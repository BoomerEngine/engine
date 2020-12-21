/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasBatchRenderer.h"
#include "renderingCanvasService.h"

#include "rendering/device/include/renderingResources.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingGraphicsStates.h"
#include "rendering/device/include/renderingShaderFile.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingDeviceGlobalObjects.h"

#include "base/image/include/imageView.h"
#include "base/canvas/include/canvasService.h"
#include "base/canvas/include/canvas.h"

namespace rendering
{
    namespace canvas
    {

        //---

        base::ConfigProperty<bool> cvUseCanvasBatching("Render.Canvas", "UseBatching", true);
        base::ConfigProperty<bool> cvUseCanvasGlyphAtlas("Render.Canvas", "UseGlyphAtlas", true);
		base::ConfigProperty<bool> cvUseCanvasImageAtlas("Render.Canvas", "UseImageAtlas", true);

        base::ConfigProperty<uint32_t> cvMaxVertices("Rendering.Canvas", "MaxVertices", 200000);
		base::ConfigProperty<uint32_t> cvMaxAttributes("Rendering.Canvas", "MaxAttributes", 4096);

		//---

		base::res::StaticResource<ShaderFile> resCanvasShaderFill("/engine/shaders/canvas/canvas_fill.fx");

		//---

		/// rendering handler for custom canvas batches
		class CanvasDefaultBatchRenderer : public ICanvasSimpleBatchRenderer
		{
			RTTI_DECLARE_VIRTUAL_CLASS(CanvasDefaultBatchRenderer, ICanvasSimpleBatchRenderer);

		public:
			virtual ShaderFilePtr loadMainShaderFile() override final
			{
				return resCanvasShaderFill.loadAndGet();
			}
		};

		RTTI_BEGIN_TYPE_CLASS(CanvasDefaultBatchRenderer);
		RTTI_END_TYPE();

		//--

		CanvasRenderAtlasImage::CanvasRenderAtlasImage(ImageFormat format, uint32_t size, uint32_t numPages)
		{
			auto* dev = base::GetService<DeviceService>()->device();

			// create image
			{
				ImageCreationInfo info;
				info.allowCopies = true;
				info.allowDynamicUpdate = true;
				info.allowShaderReads = true;
				info.width = size;
				info.height = size;
				info.numSlices = numPages;
				info.format = format;
				info.view = ImageViewType::View2DArray;
				info.label = "CanvasImageAtlas";
				atlasImage = dev->createImage(info);
				atlasImageSRV = atlasImage->createSampledView();
			}

			// create image entry buffer
			{
				BufferCreationInfo info;
				info.allowCopies = true;
				info.allowDynamicUpdate = true;
				info.allowShaderReads = true;
				info.stride = sizeof(GPUCanvasImageInfo);
				info.size = 2048 * info.stride;
				info.label = "CanvasImageEntries";
				atlasInfoBuffer = dev->createBuffer(info);
				atlasInfoBufferSRV = atlasInfoBuffer->createStructuredView();
			}
		}

		void CanvasRenderAtlasImage::updateImage(command::CommandWriter& cmd, const base::image::ImageView& sourceImage, uint8_t pageIndex, const base::Rect& dirtyRect)
		{
			auto dirtySubImage = sourceImage.subView(
				dirtyRect.min.x, dirtyRect.min.y,
				dirtyRect.width(), dirtyRect.height());

			cmd.opTransitionLayout(atlasImage, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);
			cmd.opUpdateDynamicImage(atlasImage, dirtySubImage, 0, pageIndex, dirtyRect.min.x, dirtyRect.min.y);
			cmd.opTransitionLayout(atlasImage, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
		}

		void CanvasRenderAtlasImage::updateEntries(command::CommandWriter& cmd, uint32_t firstEntry, uint32_t numEntries, const base::canvas::ImageAtlasEntryInfo* entiresData)
		{
			cmd.opTransitionLayout(atlasInfoBuffer, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);

			auto* writePtr = cmd.opUpdateDynamicBufferPtrN< GPUCanvasImageInfo>(atlasInfoBuffer, firstEntry, numEntries);
			const auto* readPtr = entiresData;
			const auto* readEndPtr = entiresData + numEntries;
			while (readPtr < readEndPtr)
			{
				writePtr->uvMin = readPtr->uvOffset;
				writePtr->uvMax = readPtr->uvMax;
				writePtr->uvScale = (readPtr->uvMax - readPtr->uvOffset);
				writePtr->uvInvScale.x = (writePtr->uvScale.x > 0.0f) ? (1.0f / writePtr->uvScale.x) : 0.0f;
				writePtr->uvInvScale.y = (writePtr->uvScale.y > 0.0f) ? (1.0f / writePtr->uvScale.y) : 0.0f;
				++writePtr;
				++readPtr;
			}

			cmd.opTransitionLayout(atlasInfoBuffer, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
		}

		//--

		RTTI_BEGIN_TYPE_CLASS(CanvasRenderService);
			RTTI_METADATA(base::app::DependsOnServiceMetadata).dependsOn<rendering::DeviceService>();
			RTTI_METADATA(base::app::DependsOnServiceMetadata).dependsOn<base::canvas::CanvasService>();
		RTTI_END_TYPE();

		//--

		CanvasRenderService::CanvasRenderService()
		{}

		CanvasRenderService::~CanvasRenderService()
		{}

		base::app::ServiceInitializationResult CanvasRenderService::onInitializeService(const base::app::CommandLine& cmdLine)
		{
			m_device = base::GetService<DeviceService>()->device();

			m_maxBatchVetices = std::max<uint32_t>(4096, cvMaxVertices.get());
			m_maxAttributes = std::max<uint32_t>(1024, cvMaxAttributes.get());

			{
				BufferCreationInfo info;
				info.allowDynamicUpdate = true;
				info.allowVertex = true;
				info.size = sizeof(base::canvas::Vertex) * m_maxBatchVetices;
				info.label = "CanvasVertices";

				m_sharedVertexBuffer = m_device->createBuffer(info);
			}

			{
				BufferCreationInfo info;
				info.allowShaderReads = true;
				info.size = sizeof(GPUCanvasImageInfo);
				info.stride = sizeof(GPUCanvasImageInfo);
				info.label = "CanvasEmptyAtlasEntries";
				m_emptyAtlasEntryBuffer = m_device->createBuffer(info);
				m_emptyAtlasEntryBufferSRV = m_emptyAtlasEntryBuffer->createStructuredView();
			}

			{
				BufferCreationInfo info;
				info.allowShaderReads = true;
				info.allowDynamicUpdate = true;
				info.size = sizeof(base::canvas::Attributes) * m_maxAttributes;
				info.stride = sizeof(base::canvas::Attributes);
				info.label = "CanvasAttributesBuffer";
				m_sharedAttributesBuffer = m_device->createBuffer(info);
				m_sharedAttributesBufferSRV = m_sharedAttributesBuffer->createStructuredView();

			}

			createRenderStates();
			createBatchRenderers();

			return base::app::ServiceInitializationResult::Finished;
		}

		void CanvasRenderService::onShutdownService()
		{
			delete m_renderStates;
			m_renderStates = nullptr;

			delete m_glyphCache;
			m_glyphCache = nullptr;

			m_imageCaches.clearPtr();
			m_batchRenderers.clearPtr();
		}

		void CanvasRenderService::onSyncUpdate()
		{

		}

		//--

		void CanvasRenderService::updateGlyphCache(rendering::command::CommandWriter& cmd, const base::canvas::ICanvasAtlasSync::AtlasUpdate& update)
		{
			if (!m_glyphCache)
				m_glyphCache = new CanvasRenderAtlasImage(ImageFormat::R8_UNORM, update.size, update.numPages);

			for (const auto& page : update.pageUpdate)
				m_glyphCache->updateImage(cmd, page.data, page.pageIndex, page.rect);
		}

		void CanvasRenderService::updateAtlas(rendering::command::CommandWriter& cmd, base::canvas::ImageAtlasIndex atlasIndex, const base::canvas::ICanvasAtlasSync::AtlasUpdate& update)
		{
			m_imageCaches.prepareWith(atlasIndex + 1, nullptr);

			auto& atlasRef = m_imageCaches[atlasIndex];
			if (!atlasRef)
				atlasRef = new CanvasRenderAtlasImage(ImageFormat::RGBA8_UNORM, update.size, update.numPages);

			if (update.entryUpdate.data)
				atlasRef->updateEntries(cmd, update.entryUpdate.firstEntry, update.entryUpdate.numEntries, update.entryUpdate.data);

			for (const auto& page : update.pageUpdate)
				atlasRef->updateImage(cmd, page.data, page.pageIndex, page.rect);
		}

		//--

		void CanvasRenderService::render(command::CommandWriter& cmd, const base::canvas::Canvas& canvas)
		{
			cmd.opBeginBlock("RenderCanvas");

			{
				cmd.opBeginBlock("SyncCanvasAtlas");
				static auto* service = base::GetService<base::canvas::CanvasService>();
				service->syncAssetChanges(cmd, canvas.m_usedAtlasMask, canvas.m_usedGlyphPagesMask, *this);
				cmd.opEndBlock();
			}

			for (auto* renderer : m_batchRenderers)
				if (renderer)
					renderer->prepareForLayout(*m_renderStates, cmd.currentPassLayout());

			float scaleX = canvas.width();
			float scaleY = canvas.height();
			float offsetX = 0;
			float offsetY = 0;

			// TODO: clip!

			base::Matrix canvasToScreen;
			canvasToScreen.identity();
			canvasToScreen.m[0][0] = 2.0f / scaleX;
			canvasToScreen.m[1][1] = 2.0f / scaleY;
			canvasToScreen.m[0][3] = -1.0f + (offsetX / scaleX) - 0.5f * canvasToScreen.m[0][0];
			canvasToScreen.m[1][3] = -1.0f + (offsetX / scaleX) - 0.5f * canvasToScreen.m[1][1];

			//--

			// upload vertices as is
			const auto numVertices = canvas.m_gatheredVertices.size();
			if (const auto vertexDataSize = sizeof(base::canvas::Vertex) * numVertices)
			{
				cmd.opTransitionLayout(m_sharedVertexBuffer, ResourceLayout::VertexBuffer, ResourceLayout::CopyDest);

				auto* writePtr = cmd.opUpdateDynamicBufferPtrN<base::canvas::Vertex>(m_sharedVertexBuffer, 0, numVertices);
				canvas.m_gatheredVertices.copy(writePtr, vertexDataSize);

				cmd.opTransitionLayout(m_sharedVertexBuffer, ResourceLayout::CopyDest, ResourceLayout::VertexBuffer);
			}

			// upload attributes as is
			const auto numAttributes = canvas.m_gatheredAttributes.size();
			if (const auto attributesDataSize = sizeof(base::canvas::Attributes) * numAttributes)
			{
				cmd.opTransitionLayout(m_sharedAttributesBuffer, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);
				cmd.opUpdateDynamicBuffer(m_sharedAttributesBuffer, 0, attributesDataSize, canvas.m_gatheredAttributes.typedData());
				cmd.opTransitionLayout(m_sharedAttributesBuffer, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
			}

			// bind vertices
			cmd.opBindVertexBuffer("CanvasVertex"_id, m_sharedVertexBuffer);

			// bound atlas
			int currentBoundAtlas = -1;

			// draw batches, as many as we can in one go
			bool firstBatch = true;
			const auto* batchPtr = canvas.m_gatheredBatches.typedData();
			const auto* batchEnd = batchPtr + canvas.m_gatheredBatches.size();
			while (batchPtr < batchEnd)
			{
				const auto* curBatchStart = batchPtr;

				// find end of the range
				uint32_t firstDrawVertex = batchPtr->vertexOffset;
				uint32_t numDrawVertices = batchPtr->vertexCount;
				while (++batchPtr < batchEnd)
				{
					if ((batchPtr->atlasIndex && batchPtr->atlasIndex != curBatchStart->atlasIndex)
						|| batchPtr->rendererIndex != curBatchStart->rendererIndex
						|| batchPtr->renderDataOffset != curBatchStart->renderDataOffset
						|| batchPtr->op != curBatchStart->op
						|| batchPtr->type != curBatchStart->type)
						break;

					numDrawVertices += batchPtr->vertexCount;
				}

				// render with selected renderer
				if (const auto* renderer = m_batchRenderers[curBatchStart->rendererIndex])
				{
					ASSERT(firstDrawVertex + numDrawVertices <= numVertices);
					if (numDrawVertices)
					{
						// prepare render states
						ICanvasBatchRenderer::RenderData data;
						if (curBatchStart->atlasIndex > 0)
						{
							if (const auto* atlas = m_imageCaches[curBatchStart->atlasIndex])
							{
								data.atlasData = atlas->atlasInfoBufferSRV;
								data.atlasImage = atlas->atlasImageSRV;
							}
						}

						data.blendOp = curBatchStart->op;
						data.batchType = curBatchStart->type;
						data.customData = base::OffsetPtr(canvas.m_gatheredData.typedData(), curBatchStart->renderDataOffset);
						data.vertexBuffer = m_sharedVertexBuffer;
						data.glyphImage = m_glyphCache ? m_glyphCache->atlasImageSRV : Globals().TextureArrayWhite;

						// rebind atlas data
						if ((curBatchStart->atlasIndex && currentBoundAtlas != curBatchStart->atlasIndex) || firstBatch)
						{
							struct
							{
								base::Matrix CanvasToScreen;

								uint32_t width;
								uint32_t height;
								uint32_t padding0;
								uint32_t padding1;
							} consts;

							// setup constants
							consts.width = canvas.width();
							consts.height = canvas.height();
							consts.CanvasToScreen = canvasToScreen;

							// bind data
							DescriptorEntry desc[5];
							desc[0].constants(consts);
							desc[1] = m_sharedAttributesBufferSRV;
							desc[2] = data.atlasData ? data.atlasData : m_emptyAtlasEntryBufferSRV;
							desc[3] = data.atlasImage ? data.atlasImage : Globals().TextureArrayWhite;
							desc[4] = data.glyphImage ? data.glyphImage : Globals().TextureArrayWhite;
							cmd.opBindDescriptor("CanvasDesc"_id, desc);

							firstBatch = false;
						}

						// render the vertices from all the batches in one go
						renderer->render(cmd, data, firstDrawVertex, numDrawVertices);
					}
				}
			}

			cmd.opEndBlock();
		}

		//--

		void CanvasRenderService::createRenderStates()
		{
			m_renderStates = new CanvasRenderStates();

			// mask generation
			{
				GraphicsRenderStatesSetup setup;
				setup.stencil(true);
				setup.blend(true);
				setup.blendFactor(0, BlendFactor::Zero, BlendFactor::One);
				setup.stencilFront(CompareOp::Always, StencilOp::Keep, StencilOp::Keep, StencilOp::IncrementAndWrap);
				setup.stencilBack(CompareOp::Always, StencilOp::Keep, StencilOp::Keep, StencilOp::DecrementAndWrap);
				m_renderStates->m_mask = m_device->createGraphicsRenderStates(setup);
			}

			// drawing - with mask or without
			for (int i = 0; i < CanvasRenderStates::MAX_BLEND_OPS; ++i)
			{
				const auto op = (base::canvas::BlendOp)i;

				GraphicsRenderStatesSetup setup;

				switch (op)
				{
					case base::canvas::BlendOp::Copy:
						break;

					case base::canvas::BlendOp::Addtive:
						setup.blend(true);
						setup.blendFactor(0, BlendFactor::One, BlendFactor::One);
						break;

					case base::canvas::BlendOp::AlphaBlend:
						setup.blend(true);
						setup.blendFactor(0, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
						break;

					case base::canvas::BlendOp::AlphaPremultiplied:
						setup.blend(true);
						setup.blendFactor(0, BlendFactor::One, BlendFactor::OneMinusSrcAlpha);
						break;
				}

				m_renderStates->m_standardFill[i] = m_device->createGraphicsRenderStates(setup);

				setup.stencil(true);
				setup.stencilFront(CompareOp::Equal, StencilOp::Zero, StencilOp::Zero, StencilOp::Zero);
				setup.stencilBack(CompareOp::Equal, StencilOp::Zero, StencilOp::Zero, StencilOp::Zero);

				m_renderStates->m_maskedFill[i] = m_device->createGraphicsRenderStates(setup);
			}
		}

		void CanvasRenderService::createBatchRenderers()
		{
			base::Array<base::SpecificClassType<ICanvasBatchRenderer>> batchHandlerClasses;
			RTTI::GetInstance().enumClasses(batchHandlerClasses);

			static bool assignIndices = true;
			if (assignIndices)
			{
				short index = 1;

				for (const auto& cls : batchHandlerClasses)
					if (cls == CanvasDefaultBatchRenderer::GetStaticClass())
						cls->assignUserIndex(0);
					else
						cls->assignUserIndex(index++);

				assignIndices = false;
			}

			m_batchRenderers.resizeWith(batchHandlerClasses.size() + 1, nullptr);

			for (const auto& cls : batchHandlerClasses)
				if (auto handler = cls->createPointer<ICanvasBatchRenderer>())
					if (handler->initialize(m_device))
						m_batchRenderers[cls->userIndex()] = handler;
		}

		//--

    } // canvas
} // rendering