/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasStorage.h"
#include "renderingCanvasRenderer.h"
#include "renderingCanvasImageCache.h"
#include "renderingCanvasGlyphCache.h"

#include "rendering/device/include/renderingResources.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingDeviceGlobalObjects.h"
#include "base/image/include/imageView.h"

namespace rendering
{
    namespace canvas
    {

		//--

		RTTI_BEGIN_TYPE_NATIVE_CLASS(CanvasRenderer);
		RTTI_END_TYPE();

		//--

		CanvasRenderer::CanvasRenderer(const Setup& setup, const CanvasStorage* storage)
			: base::canvas::Canvas(setup, storage)
			, m_backBufferColorRTV(setup.backBufferColorRTV)
			, m_backBufferDepthRTV(setup.backBufferDepthRTV)
			, m_backBufferLayout(setup.backBufferLayout)
			, m_storage(storage)
		{
			m_commandBufferWriter = new command::CommandWriter("Canvas");
			m_storage->flushDataChanges(*m_commandBufferWriter);

			for (auto* renderer : m_storage->m_batchRenderers)
				if (renderer)
					renderer->prepareForLayout(*m_storage->m_renderStates, setup.backBufferLayout);

			{
				//float scaleX = setup.width;
				//float scaleY = setup.height;
				float scaleX = setup.backBufferColorRTV->width();
				float scaleY = setup.backBufferColorRTV->height();
				float offsetX = 0;
				float offsetY = 0;

				// TODO: clip!

				m_canvasToScreen.identity();
				m_canvasToScreen.m[0][0] = 2.0f / scaleX;
				m_canvasToScreen.m[1][1] = 2.0f / scaleY;
				m_canvasToScreen.m[0][3] = -1.0f + (offsetX / scaleX) - 0.5f * m_canvasToScreen.m[0][0];
				m_canvasToScreen.m[1][3] = -1.0f + (offsetX / scaleX) - 0.5f * m_canvasToScreen.m[1][1];
			}
		}

		CanvasRenderer::~CanvasRenderer()
		{
			if (m_inPass)
				m_commandBufferWriter->opEndPass();

			delete m_commandBufferWriter;
			m_commandBufferWriter = nullptr;
		}

		command::CommandBuffer* CanvasRenderer::finishRecording()
		{
			flush();
			finishPass();
			return m_commandBufferWriter->release();
		}

		//--

		void CanvasRenderer::startPass()
		{
			if (!m_inPass)
			{
				FrameBuffer fb;
				fb.color[0].view(m_backBufferColorRTV);
				fb.depth.view(m_backBufferDepthRTV);

				m_commandBufferWriter->opBeingPass(m_backBufferLayout, fb);
				m_inPass = true;
			}
		}

		void CanvasRenderer::finishPass()
		{
			if (m_inPass)
			{
				m_commandBufferWriter->opEndPass();
				m_inPass = false;
			}
		}

		void CanvasRenderer::flushInternal(const base::canvas::Vertex* vertices, uint32_t numVertices, 
			const base::canvas::Attributes* attributes, uint32_t numAttributes,
			const void* customData, uint32_t customDataSize,
			const base::canvas::Batch* batches, uint32_t numBatches)
		{
			PC_SCOPE_LVL1(CanvasRendererFlush);

			// upload vertices as is
			if (const auto vertexDataSize = sizeof(base::canvas::Vertex) * numVertices)
			{				
				m_commandBufferWriter->opTransitionLayout(m_storage->m_sharedVertexBuffer, ResourceLayout::VertexBuffer, ResourceLayout::CopyDest);
				m_commandBufferWriter->opUpdateDynamicBuffer(m_storage->m_sharedVertexBuffer, 0, vertexDataSize, vertices);
				m_commandBufferWriter->opTransitionLayout(m_storage->m_sharedVertexBuffer, ResourceLayout::CopyDest, ResourceLayout::VertexBuffer);
			}

			// upload attributes as is
			if (const auto attributesDataSize = sizeof(base::canvas::Attributes) * numAttributes)
			{
				m_commandBufferWriter->opTransitionLayout(m_storage->m_sharedAttributesBuffer, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);
				m_commandBufferWriter->opUpdateDynamicBuffer(m_storage->m_sharedAttributesBuffer, 0, attributesDataSize, attributes);
				m_commandBufferWriter->opTransitionLayout(m_storage->m_sharedAttributesBuffer, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);
			}

			// bind vertices
			m_commandBufferWriter->opBindVertexBuffer("CanvasVertex"_id, m_storage->m_sharedVertexBuffer);

			// draw batches, as many as we can in one go
			bool firstBatch = true;
			const auto* batchPtr = batches;
			const auto* batchEnd = batches + numBatches;
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
				if (const auto* renderer = m_storage->m_batchRenderers[curBatchStart->rendererIndex])
				{
					ASSERT(firstDrawVertex + numDrawVertices <= numVertices);
					if (numDrawVertices)
					{
						// prepare render states
						ICanvasBatchRenderer::RenderData data;
						if (curBatchStart->atlasIndex > 0)
						{
							if (const auto* atlas = m_storage->m_imageCaches[curBatchStart->atlasIndex - 1])
							{
								data.atlasData = atlas->atlasInfo();
								data.atlasImage = atlas->atlasView();
							}
						}

						data.blendOp = curBatchStart->op;
						data.batchType = curBatchStart->type;
						data.customData = base::OffsetPtr(customData, curBatchStart->renderDataOffset);
						data.vertexBuffer = m_storage->m_sharedVertexBuffer;
						data.vertices = vertices + firstDrawVertex;
						data.glyphImage = m_storage->m_glyphCache->atlasView();

						// rebind atlas data
						if ((curBatchStart->atlasIndex && m_currentAtlasIndex != curBatchStart->atlasIndex) || firstBatch)
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
							consts.width = width();
							consts.height = height();
							consts.CanvasToScreen = m_canvasToScreen;

							// bind data
							DescriptorEntry desc[5];
							desc[0].constants(consts);
							desc[1] = m_storage->m_sharedAttributesBufferSRV;
							desc[2] = data.atlasData ? data.atlasData : m_storage->m_emptyAtlasEntryBufferSRV;
							desc[3] = data.atlasImage ? data.atlasImage : Globals().TextureArrayWhite;
							desc[4] = data.glyphImage ? data.glyphImage : Globals().TextureArrayWhite;
							m_commandBufferWriter->opBindDescriptor("CanvasDesc"_id, desc);

							firstBatch = false;
						}

						// make sure we are in pass before rendering (previous renderer might have exited)
						startPass();

						// render the vertices from all the batches in one go
						renderer->render(*m_commandBufferWriter, data, firstDrawVertex, numDrawVertices);
					}
				}
			}
		}

		//--

    } // canvas
} // rendering