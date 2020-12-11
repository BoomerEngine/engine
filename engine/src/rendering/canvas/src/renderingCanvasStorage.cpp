/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasStorage.h"
#include "renderingCanvasImageCache.h"
#include "renderingCanvasGlyphCache.h"
#include "renderingCanvasBatchRenderer.h"

#include "rendering/device/include/renderingResources.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingGraphicsStates.h"
#include "rendering/device/include/renderingShaderFile.h"

#include "base/image/include/imageView.h"
#include "base/canvas/include/canvasGeometry.h"

namespace rendering
{
    namespace canvas
    {

        //---

        base::ConfigProperty<bool> cvUseCanvasBatching("Render.Canvas", "UseBatching", true);
        base::ConfigProperty<bool> cvUseCanvasGlyphAtlas("Render.Canvas", "UseGlyphAtlas", true);
		base::ConfigProperty<bool> cvUseCanvasImageAtlas("Render.Canvas", "UseImageAtlas", true);

		base::ConfigProperty<uint32_t> cvCanvasPageMinSize("Render.Canvas", "AtlasMinSize", 256);
		base::ConfigProperty<uint32_t> cvCanvasPageMaxSize("Render.Canvas", "AtlasMaxSize", 4096);
		base::ConfigProperty<uint32_t> cvCanvasPageSizeGranurality("Render.Canvas", "AtlasSizeGranurality", 64);
        base::ConfigProperty<uint32_t> cvCanvasMaxPages("Rendering.Canvas", "AtlasMaxPages", 64);
		base::ConfigProperty<bool> cvCanvasForcePowerOfTwoPages("Render.Canvas", "ForcePowerOfTwoSizes", false);

        base::ConfigProperty<uint32_t> cvMaxVertices("Rendering.Canvas", "MaxVertices", 65536);
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

		RTTI_BEGIN_TYPE_NATIVE_CLASS(CanvasStorage);
		RTTI_END_TYPE();

		//--

		CanvasStorage::CanvasStorage(IDevice* dev)
			: m_device(dev)
			, m_maxBatchVetices(std::max<uint32_t>(1024, cvMaxVertices.get()))
			, m_maxAttributes(std::max<uint32_t>(128, cvMaxAttributes.get()))
			, m_allowImages(cvUseCanvasImageAtlas.get())
		{
			if (cvUseCanvasGlyphAtlas.get())
				m_glyphCache = new CanvasGlyphCache(dev);

			{
				BufferCreationInfo info;
				info.allowDynamicUpdate = true;
				info.allowVertex = true;
				info.size = sizeof(base::canvas::Vertex) * m_maxBatchVetices;
				info.label = "CanvasVertices";

				m_sharedVertexBuffer = dev->createBuffer(info);
			}

			{
				BufferCreationInfo info;
				info.allowShaderReads = true;
				info.size = sizeof(GPUCanvasImageInfo);
				info.stride = sizeof(GPUCanvasImageInfo);
				info.label = "CanvasEmptyAtlasEntries";
				m_emptyAtlasEntryBuffer = dev->createBuffer(info);
				m_emptyAtlasEntryBufferSRV = m_emptyAtlasEntryBuffer->createStructuredView();
			}

			{
				BufferCreationInfo info;
				info.allowShaderReads = true;
				info.allowDynamicUpdate = true;
				info.size = sizeof(base::canvas::Attributes) * m_maxAttributes;
				info.stride = sizeof(base::canvas::Attributes);
				info.label = "CanvasAttributesBuffer";
				m_sharedAttributesBuffer = dev->createBuffer(info);
				m_sharedAttributesBufferSRV = m_sharedAttributesBuffer->createStructuredView();
				
			}

			createRenderStates();
			createBatchRenderers();
		}

		CanvasStorage::~CanvasStorage()
		{
			delete m_glyphCache;
			m_glyphCache = nullptr;

			delete m_renderStates;
			m_renderStates = nullptr;

			m_imageCaches.clearPtr();
			m_batchRenderers.clearPtr();			
		}

		void CanvasStorage::flushDataChanges(command::CommandWriter& cmd) const
		{
			if (m_glyphCache)
				m_glyphCache->flushUpdate(cmd);

			for (auto* imageCache : m_imageCaches)
				if (imageCache)
					imageCache->flushUpdate(cmd);
		}

		void CanvasStorage::conditionalRebuild(bool* outAtlasesWereRebuilt)
		{
		}

		base::canvas::ImageAtlasIndex CanvasStorage::createAtlas(uint32_t suggestedPageSize, uint32_t numPages, base::StringView debugName)
		{
			// image atlases may be disabled
			if (!m_allowImages)
				return 0;

			// calculate page size
			const auto pageGranurality = base::NextPow2(std::clamp<uint32_t>(cvCanvasPageSizeGranurality.get(), 1, 1024));
			const auto pageSize = base::Align(std::clamp<uint32_t>(suggestedPageSize, cvCanvasPageMinSize.get(), cvCanvasPageMaxSize.get()), pageGranurality);
			
			// calculate page count
			const auto pageCount = std::clamp<uint32_t>(numPages, 1, cvCanvasMaxPages.get());

			// calculate final texture size
			const auto textureSize = cvCanvasForcePowerOfTwoPages.get() ? base::NextPow2(pageSize) : pageSize;

			// allocate atlas
			auto* atlas = new CanvasImageCache(m_device, ImageFormat::RGBA8_UNORM, textureSize, pageCount);

			// try to place in free entry
			for (auto index : m_imageCaches.indexRange())
			{
				if (m_imageCaches[index] == nullptr)
				{
					m_imageCaches[index] = atlas;
					return index + 1;
				}
			}

			// add at the end
			m_imageCaches.pushBack(atlas);
			return m_imageCaches.size(); // the ID is just the index+1
		}

		void CanvasStorage::destroyAtlas(base::canvas::ImageAtlasIndex atlasIndex)
		{
			DEBUG_CHECK_RETURN_EX(atlasIndex != 0, "Invalid atlas index");
			DEBUG_CHECK_RETURN_EX(atlasIndex <= m_imageCaches.size(), "Invalid atlas index");

			auto& entry = m_imageCaches[atlasIndex - 1];
			DEBUG_CHECK_RETURN_EX(entry != nullptr, "Atlas not allocated");

			delete entry;
			entry = nullptr;
		}

		base::canvas::ImageEntry CanvasStorage::registerImage(base::canvas::ImageAtlasIndex atlasIndex, const base::image::Image* ptr, bool supportWrapping, int additionalPixelBorder)
		{
			if (!m_allowImages)
				return base::canvas::ImageEntry();

			DEBUG_CHECK_RETURN_EX_V(atlasIndex != 0, "Invalid atlas index", base::canvas::ImageEntry());
			DEBUG_CHECK_RETURN_EX_V(atlasIndex <= m_imageCaches.size(), "Invalid atlas index", base::canvas::ImageEntry());
			DEBUG_CHECK_RETURN_EX_V(ptr != nullptr, "Invalid image", base::canvas::ImageEntry());
			DEBUG_CHECK_RETURN_EX_V(!ptr->view().empty(), "Empty image", base::canvas::ImageEntry());

			const auto& atlas = m_imageCaches[atlasIndex - 1];
			if (auto ret = atlas->registerImage(ptr, supportWrapping, additionalPixelBorder))
			{
				ret.atlasIndex = atlasIndex;
				return ret;
			}

			return base::canvas::ImageEntry();
		}

		void CanvasStorage::unregisterImage(base::canvas::ImageEntry entry)
		{
			if (entry)
			{
				DEBUG_CHECK_RETURN_EX(entry.atlasIndex != 0, "Invalid entry");
				DEBUG_CHECK_RETURN_EX(entry.atlasIndex <= m_imageCaches.size(), "Invalid entry atlas index");

				const auto& atlas = m_imageCaches[entry.atlasIndex - 1];
				atlas->unregisterImage(entry);
			}
		}

		const base::canvas::ImageAtlasEntryInfo* CanvasStorage::findRenderDataForAtlasEntry(const base::canvas::ImageEntry& entry) const
		{
			if (entry.atlasIndex > 0 && entry.atlasIndex <= m_imageCaches.size())
				if (const auto* cache = m_imageCaches[entry.atlasIndex - 1])
					return cache->fetchImageData(entry.entryIndex);

			return nullptr;
		}

		const base::canvas::ImageAtlasEntryInfo* CanvasStorage::findRenderDataForGlyph(const base::font::Glyph* glyph) const
		{
			if (m_glyphCache)
				return m_glyphCache->findRenderDataForGlyph(glyph);

			return nullptr;
		}

		//--

		void CanvasStorage::createRenderStates()
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

		void CanvasStorage::createBatchRenderers()
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
					if (handler->initialize(this, m_device))
						m_batchRenderers[cls->userIndex()] = handler;

		}

		//--

    } // canvas
} // rendering