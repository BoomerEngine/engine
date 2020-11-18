/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasRenderer.h"
#include "renderingCanvasImageCache.h"
#include "renderingCanvasRendererCustomBatchHandler.h"

#include "base/app/include/localServiceContainer.h"
#include "base/image/include/imageDynamicAtlas.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasGlyphCache.h"
#include "base/system/include/scopeLock.h"

#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingObject.h"
#include "rendering/device/include/renderingShaderLibrary.h"
#include "rendering/device/include/renderingDeviceService.h"

namespace rendering
{
    namespace canvas
    {

        //---

        base::ConfigProperty<bool> cvUseCanvasBatching("Render.Canvas", "UseBatching", true);
        base::ConfigProperty<bool> cvUseCanvasAtlas("Render.Canvas", "UseAtlas", true);
        base::ConfigProperty<uint32_t> cvNumInitialColorPages("Rendering.Canvas", "NumInitialColorPages", 1);
        base::ConfigProperty<uint32_t> cvNumInitialAlphaPages("Rendering.Canvas", "NumInitialAlphaPages", 1);
        base::ConfigProperty<uint32_t> cvColorPageSize("Rendering.Canvas", "MaxColorPageSize", 2048);
        base::ConfigProperty<uint32_t> cvAlphaPageSize("Rendering.Canvas", "MaxAlphaPageSize", 2048);

        base::ConfigProperty<uint32_t> cvMaxVertices("Rendering.Canvas", "MaxVertices", 4*1024*1024);
        base::ConfigProperty<uint32_t> cvMaxIndices("Rendering.Canvas", "MaxIndices", 6*1024*1024);
        base::ConfigProperty<uint32_t> cvMaxParams("Rendering.Canvas", "MaxObjects", 65536);

        ///---

        class CanvasImageCache;
        struct CanvasImageCacheEntry;

        ///---

        struct CanvasVertex
        {
            base::Vector2 VertexPosition;
            base::Vector2 VertexUV;
            base::Vector2 VertexPaintUV;
            base::Vector2 VertexClipUV;
            base::Color VertexColor;
            uint32_t VertexBatchID;
        };

        ///---

        struct CanvasViewportParams
        {
            struct ConstData
            {
                base::Matrix CanvasToScreen;
            };

            ConstantsView Consts;
            BufferView ParametersData; // buffer with rendering style parameters
            ImageView AlphaTextureAtlas;
            ImageView ColorTextureAtlas;
        };

        ///---

        struct CanvasShaderParams
        {
            base::Vector4 InnerCol = base::Vector4::ONE();
            base::Vector4 OuterCol = base::Vector4::ONE();

            base::Vector2 Base = base::Vector2::ZERO();
            base::Vector2 Padding0 = base::Vector2::ZERO();

            base::Vector2 Extent = base::Vector2::ONE();
            base::Vector2 ExtentInv = base::Vector2::ONE();

            base::Vector2 UVBias = base::Vector2::ZERO();
            base::Vector2 UVScale = base::Vector2::ONE();
            base::Vector2 UVMin = base::Vector2::ZERO();
            base::Vector2 UVMax = base::Vector2::ONE();

            float FeatherHalf = 0.0f;
            float FeatherInv = 0.0f;
            float Radius = 0.0f;
            float Feather = 0.0f;

            uint32_t TextureType = 0; // 0-none 1-alpha 2-color 3-custom
            uint32_t TextureSlice = 0; // slice index
            uint32_t ShaderType = 0;
            uint32_t WrapType = 0;
        };

        //---

        void CanvasRenderer::packParameters(const base::canvas::Canvas::Params& params, CanvasShaderParams& outParams) const
        {
            // TODO: more tight packing ?
            outParams.InnerCol = params.innerCol;
            outParams.OuterCol = params.outerCol;
            outParams.Base = params.base;
            outParams.Extent = params.extent;
            outParams.ExtentInv.x = params.extent.x ? 1.0f / params.extent.x : 0.0f;
            outParams.ExtentInv.y = params.extent.y ? 1.0f / params.extent.y : 0.0f;
            outParams.UVMin = params.uvMin;
            outParams.UVMax = params.uvMax;
            outParams.FeatherHalf = params.featherHalf;
            outParams.FeatherInv = params.featherInv;
            outParams.Radius = params.radius;
            outParams.Feather = params.feather;
            outParams.WrapType = params.wrapType;
            outParams.TextureType = 0;
            outParams.ShaderType = 0;
        }

        base::res::StaticResource<ShaderLibrary> resCanvasShaderMask("/engine/shaders/canvas/canvas_mask.fx");
        base::res::StaticResource<ShaderLibrary> resCanvasShaderFill("/engine/shaders/canvas/canvas_fill.fx");
        
        CanvasRenderer::CanvasRenderer()
            : m_api(base::GetService<DeviceService>()->device())
        {
            m_colorImages.reserve(1024);
            m_alphaImages.reserve(1024);
            m_paramDataStorage.reserve(1024);

            createCustomHandlers();
            createDeviceResources();
        }

        CanvasRenderer::~CanvasRenderer()
        {
            destroyDeviceResources();

            base::mem::FreeBlock(m_customHandlersFlatList);
            m_customHandlersFlatList = nullptr;
        }

        //--

        CanvasRenderer::BatchCollector::BatchCollector()
        {}

        void CanvasRenderer::BatchCollector::push(const base::canvas::Canvas::Batch& b)
        {
            if (!cvUseCanvasBatching.get() || b.type != m_currentShader || b.op != m_currentOp || b.fristIndex != (m_currentIndexCount + m_currentStartIndex) || b.customDrawer != m_currentCustomHandlerType)
            {
                finish();

                m_currentCustomHandlerType = b.customDrawer;
                m_currentShader = b.type;
                m_currentOp = b.op;
                m_currentIndexCount = 0;
                m_currentStartIndex = b.fristIndex;
                m_currentStartPayload = m_customPayloads.size();
            }

            m_currentIndexCount += b.numIndices;

            if (b.customDrawer != 0)
            {
                auto& payload = m_customPayloads.emplaceBack();
                payload.firstIndex = b.fristIndex;
                payload.numIndices = b.numIndices;
                payload.data = b.customPayload;
            }
        }

        void CanvasRenderer::BatchCollector::finish()
        {
            if (m_currentIndexCount > 0)
            {
                auto& batch = m_batches.emplaceBack();
                batch.shader = m_currentShader;
                batch.blendOp = m_currentOp;
                batch.firstIndex = m_currentStartIndex;
                batch.numIndices = m_currentIndexCount;
                batch.customDrawer = (uint16_t)m_currentCustomHandlerType;
                batch.firstPayload = m_currentStartPayload;
                batch.numPayloads = m_customPayloads.size() - m_currentStartPayload;
            }
        }

        //--

        static void SelectBlendMode(command::CommandWriter& cmd, base::canvas::CompositeOperation op)
        {
            BlendState state;

            switch (op)
            {
                case base::canvas::CompositeOperation::Copy:
                    break;

                case base::canvas::CompositeOperation::Blend:
                    state.srcAlphaBlendFactor = state.srcColorBlendFactor = BlendFactor::SrcAlpha;
                    state.destAlphaBlendFactor = state.destColorBlendFactor= BlendFactor::OneMinusSrcAlpha;
                    break;

                case base::canvas::CompositeOperation::SourceOver:
                    state.srcAlphaBlendFactor = state.srcColorBlendFactor = BlendFactor::One;
                    state.destAlphaBlendFactor = state.destColorBlendFactor = BlendFactor::OneMinusSrcAlpha;
                    break;

                case base::canvas::CompositeOperation::SourceIn:
                    state.srcAlphaBlendFactor = state.srcColorBlendFactor = BlendFactor::DestAlpha;
                    state.destAlphaBlendFactor = state.destColorBlendFactor = BlendFactor::Zero;
                    break;

                case base::canvas::CompositeOperation::SourceOut:
                    state.srcAlphaBlendFactor = state.srcColorBlendFactor = BlendFactor::OneMinusDestAlpha;
                    state.destAlphaBlendFactor = state.destColorBlendFactor = BlendFactor::Zero;
                    break;

                case base::canvas::CompositeOperation::SourceAtop:
                    state.srcAlphaBlendFactor = state.srcColorBlendFactor = BlendFactor::DestAlpha;
                    state.destAlphaBlendFactor = state.destColorBlendFactor = BlendFactor::OneMinusSrcAlpha;
                    break;

                case base::canvas::CompositeOperation::DestinationOver:
                    state.srcAlphaBlendFactor = state.srcColorBlendFactor = BlendFactor::OneMinusDestAlpha;
                    state.destAlphaBlendFactor = state.destColorBlendFactor = BlendFactor::One;
                    break;

                case base::canvas::CompositeOperation::DestinationIn:
                    state.srcAlphaBlendFactor = state.srcColorBlendFactor = BlendFactor::Zero;
                    state.destAlphaBlendFactor = state.destColorBlendFactor = BlendFactor::SrcAlpha;
                    break;

                case base::canvas::CompositeOperation::DestinationOut:
                    state.srcAlphaBlendFactor = state.srcColorBlendFactor = BlendFactor::Zero;
                    state.destAlphaBlendFactor = state.destColorBlendFactor = BlendFactor::OneMinusSrcAlpha;
                    break;

                case base::canvas::CompositeOperation::DestinationAtop:
                    state.srcAlphaBlendFactor = state.srcColorBlendFactor = BlendFactor::OneMinusDestAlpha;
                    state.destAlphaBlendFactor = state.destColorBlendFactor = BlendFactor::SrcAlpha;
                    break;

                case base::canvas::CompositeOperation::Addtive:
                    state.srcAlphaBlendFactor = state.srcColorBlendFactor = BlendFactor::One;
                    state.destAlphaBlendFactor = state.destColorBlendFactor = BlendFactor::One;
                    break;

                case base::canvas::CompositeOperation::Xor:
                    state.srcAlphaBlendFactor = state.srcColorBlendFactor = BlendFactor::OneMinusDestAlpha;
                    state.destAlphaBlendFactor = state.destColorBlendFactor = BlendFactor::OneMinusSrcAlpha;
                    break;
            }

            cmd.opSetBlendState(0, state);
        }

        static void SelectRenderMode(command::CommandWriter& cmd, base::canvas::Canvas::BatchType batchType, base::canvas::CompositeOperation op)
        {
            if (batchType == base::canvas::Canvas::BatchType::ConcaveMask)
            {
                cmd.opSetColorMask(0, 0);

                StencilState state;
                state.enabled = true;
                state.front.compareOp = CompareOp::Always;
                state.front.passOp = StencilOp::IncrementAndWrap;
                state.back.compareOp = CompareOp::Always;
                state.back.passOp = StencilOp::DecrementAndWrap;
                cmd.opSetStencilState(state);

                BlendState blendState;
                cmd.opSetBlendState(0, blendState);
            }
            else if (batchType == base::canvas::Canvas::BatchType::ConcaveFill)
            {
                cmd.opSetColorMask(0, 0xFF);

                StencilState state;
                state.enabled = true;
                state.front.compareOp = CompareOp::NotEqual;
                state.front.referenceValue = 0;
                state.front.passOp = StencilOp::Zero;
                state.back.compareOp = CompareOp::NotEqual;
                state.back.referenceValue = 0;
                state.back.passOp = StencilOp::Zero;
                cmd.opSetStencilState(state);

                SelectBlendMode(cmd, op);
            }
            else
            {
                cmd.opSetColorMask(0, 0xFF);

                StencilState state;
                state.enabled = false;
                cmd.opSetStencilState(state);

                SelectBlendMode(cmd, op);
            }
        }

        void CanvasRenderer::render(command::CommandWriter& cmd, const base::canvas::Canvas& geometry, const CanvasRenderingParams& renderingParams)
        {
            PC_SCOPE_LVL1(CanvasRender);

            // determine size of the stuff to render
            auto vertexDataSize = (uint32_t)(geometry.vertices().size() * sizeof(CanvasVertex));
            auto indexDataSize = (uint32_t)(geometry.indices().size() * sizeof(uint32_t));
            if (!vertexDataSize || !indexDataSize)
                return;

            // load shaders
            const auto shaderMask = resCanvasShaderMask.loadAndGet();
            const auto shaderFill = resCanvasShaderFill.loadAndGet();
            if (!shaderMask || !shaderFill)
                return;

            command::CommandWriterBlock block(cmd, "Canvas");

            // prepare image packing entries
            {
                PC_SCOPE_LVL2(PackImages);

                m_colorImages.reset();
                m_colorImages.reserve(geometry.images().size());
                m_alphaImages.reset();
                m_alphaImages.reserve(geometry.images().size());

                geometry.images().forEach([this](const base::canvas::Canvas::ImageRef& im)
                    {
                        if (im.imagePtr->channels() == 1)
                        {
                            im.imageType = 1;
                            im.imageIndex = m_alphaImages.size();

                            auto& entry = m_alphaImages.emplaceBack();
                            entry.m_image = im.imagePtr;
                            entry.m_needsWrapping = im.imageNeedsWrapping;
                        }
                        else
                        {
                            im.imageType = 2;
                            im.imageIndex = m_colorImages.size();

                            auto& entry = m_colorImages.emplaceBack();
                            entry.m_image = im.imagePtr;
                            entry.m_needsWrapping = im.imageNeedsWrapping;
                        }
                    });
            }

            // upload images
            {
                PC_SCOPE_LVL2(UpdateCanvasImageAltas);
                m_rgbaImageCache->cacheImages(cmd, m_colorImages.typedData(), m_colorImages.size());
                m_alphaImageCache->cacheImages(cmd, m_alphaImages.typedData(), m_alphaImages.size());
            }

            // pack render params
            {
                PC_SCOPE_LVL2(PackParams);

                m_paramDataStorage.reset();
                auto writePtr  = m_paramDataStorage.allocateUninitialized(geometry.params().size());

                geometry.params().forEach([this, &writePtr](const base::canvas::Canvas::Params& srcParams)
                    {
                        // pack general stuff
                        packParameters(srcParams, *writePtr);

                        // assign the image placement
                        if (srcParams.imageRef)
                        {
                            auto imageType = srcParams.imageRef->imageType;

                            auto& entry = (imageType == 1) ? m_alphaImages[srcParams.imageRef->imageIndex] : m_colorImages[srcParams.imageRef->imageIndex];
                            writePtr->TextureSlice = entry.m_layerIndex;
                            writePtr->TextureType = srcParams.imageRef->imageType;
                            writePtr->ShaderType = 1;
                            writePtr->UVBias = entry.m_uvOffset;
                            writePtr->UVScale = entry.m_uvScale;

                            //writePtr->UVBias.x = entry.
                        }

                        ++writePtr;
                    });
            }

            // push data we know and collect batches
            BatchCollector batchCollector;
            {
                PC_SCOPE_LVL2(CollectBatches);
                geometry.baches().forEach([&batchCollector](const base::canvas::Canvas::Batch& srcBatch) { batchCollector.push(srcBatch); });
                batchCollector.finish();
            }

            //-- upload data to GPU

            // upload vertices
            {
                PC_SCOPE_LVL2(CopyVertices);
                auto* data = cmd.opUpdateDynamicBufferPtr(m_vertexBuffer->view(), 0, vertexDataSize);
                geometry.vertices().copy(data, vertexDataSize);
            }

            // upload indices
            {
                PC_SCOPE_LVL2(CopyIndices);
                auto* data = cmd.opUpdateDynamicBufferPtr(m_indexBuffer->view(), 0, indexDataSize);
                geometry.indices().copy(data, indexDataSize);
            }

            // rendering parameters
            {
                PC_SCOPE_LVL2(CopyParams);
                auto* data = cmd.opUpdateDynamicBufferPtr(m_paramBuffer->view(), 0, m_paramDataStorage.dataSize());
                memcpy(data, m_paramDataStorage.data(), m_paramDataStorage.dataSize());
            }

            // bind viewport params
            CanvasViewportParams::ConstData constViewportParams;
            //if (canvasToScreen)
            {
                float scaleX = renderingParams.frameBufferWidth ? renderingParams.frameBufferWidth : geometry.width();
                float scaleY = renderingParams.frameBufferHeight ? renderingParams.frameBufferHeight : geometry.height();
                float offsetX = renderingParams.customViewport ? renderingParams.customViewport->left() : 0;
                float offsetY = renderingParams.customViewport ? renderingParams.customViewport->top() : 0;

                // TODO: clip!

                constViewportParams.CanvasToScreen.identity();
                constViewportParams.CanvasToScreen.m[0][0] = 2.0f / scaleX;
                constViewportParams.CanvasToScreen.m[1][1] = 2.0f / scaleY;
                constViewportParams.CanvasToScreen.m[0][3] = -1.0f + (offsetX / scaleX);
                constViewportParams.CanvasToScreen.m[1][3] = -1.0f + (offsetX / scaleX);
            }

            CanvasViewportParams viewportParams;
            viewportParams.Consts = cmd.opUploadConstants(constViewportParams);
            viewportParams.ColorTextureAtlas = m_rgbaImageCache->view();
            viewportParams.AlphaTextureAtlas = m_alphaImageCache->view();
            viewportParams.ParametersData = m_paramBuffer->view();
            cmd.opBindParametersInline("CanvasViewportParams"_id, viewportParams);

            // render the canvas geometry
            {
                PC_SCOPE_LVL2(RenderBatches);

                // bind geometry buffers
                cmd.opBindVertexBuffer("CanvasVertex"_id, m_vertexBuffer->view());
                cmd.opBindIndexBuffer(m_indexBuffer->view(), ImageFormat::R32_UINT);

                // call the batches
                for (auto& batch : batchCollector.batches())
                {
                    SelectRenderMode(cmd, batch.shader, batch.blendOp);
                    cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);

                    switch (batch.shader)
                    {
                        case base::canvas::Canvas::BatchType::ConcaveMask:
                            cmd.opDrawIndexed(shaderMask, 0, batch.firstIndex, batch.numIndices);
                            break;

                        case base::canvas::Canvas::BatchType::ConcaveFill:
                        case base::canvas::Canvas::BatchType::ConvexFill:
                            cmd.opDrawIndexed(shaderFill, 0, batch.firstIndex, batch.numIndices);
                            break;

                        case base::canvas::Canvas::BatchType::Custom:
                            DEBUG_CHECK(batch.customDrawer > 0 && batch.customDrawer <= m_customHandlers.size());
                            if (batch.customDrawer > 0 && batch.customDrawer <= m_customHandlers.size())
                                m_customHandlersFlatList[batch.customDrawer]->render(cmd, geometry, renderingParams, batch.firstIndex, batch.numIndices, batch.numPayloads, batchCollector.payloads().typedData() + batch.firstPayload);
                            break;
                    }
                }
            }

            // reset render states
            cmd.opSetColorMask(0, 0xFF);
            cmd.opSetBlendState(0, BlendState());
            cmd.opSetStencilState(StencilState());
        }

        //---

        void CanvasRenderer::createCustomHandlers()
        {
            base::Array<base::SpecificClassType<ICanvasRendererCustomBatchHandler>> batchHandlerClasses;
            RTTI::GetInstance().enumClasses(batchHandlerClasses);

            m_customHandlersFlatList = base::mem::GlobalPool<POOL_CANVAS, ICanvasRendererCustomBatchHandler*>::AllocN(batchHandlerClasses.size() + 1);
            memzero(m_customHandlersFlatList, sizeof(ICanvasRendererCustomBatchHandler*) * (batchHandlerClasses.size() + 1));

            short index = 1;
            for (const auto& cls : batchHandlerClasses)
            {
                if (auto handler = cls.create())
                {
                    m_customHandlers.pushBack(handler);
                    m_customHandlersFlatList[index] = handler.get();
                    cls->assignUserIndex(index);
                    index += 1;
                }
            }
        }

        void CanvasRenderer::handleDeviceReset()
        {

        }

        void CanvasRenderer::createDeviceResources()
        {
            m_alphaImageCache = base::CreateUniquePtr<CanvasImageCache>(ImageFormat::R8_UNORM, cvAlphaPageSize.get(), cvNumInitialAlphaPages.get());
            m_rgbaImageCache = base::CreateUniquePtr<CanvasImageCache>(ImageFormat::RGBA8_UNORM, cvColorPageSize.get(), cvNumInitialColorPages.get());

            {
                BufferCreationInfo info;
                info.allowDynamicUpdate = true;
                info.allowVertex = true;
                info.label = "CanvasVertexBuffer";
                info.size = cvMaxVertices.get() * sizeof(CanvasVertex);
                m_vertexBuffer = m_api->createBuffer(info);
            }

            {
                BufferCreationInfo info;
                info.allowDynamicUpdate = true;
                info.allowIndex = true;
                info.label = "CanvasIndexBuffer";
                info.size = cvMaxIndices.get() * sizeof(uint32_t);
                m_indexBuffer = m_api->createBuffer(info);
            }

            {
                BufferCreationInfo info;
                info.allowDynamicUpdate = true;
                info.allowShaderReads = true;
                info.allowUAV = true;
                info.allowIndex = true;
                info.label = "CanvasParamBuffer";
                info.size = cvMaxParams.get() * sizeof(CanvasShaderParams);
                info.stride = sizeof(CanvasShaderParams);
                m_paramBuffer = m_api->createBuffer(info);
            }

            for (const auto& handler : m_customHandlers)
                handler->initialize(m_api);
        }

        void CanvasRenderer::destroyDeviceResources()
        {
            m_alphaImageCache.reset();
            m_colorImages.reset();

            m_vertexBuffer.reset();
            m_indexBuffer.reset();
            m_paramBuffer.reset();

            for (const auto& handler : m_customHandlers)
                handler->deinitialize(m_api);
        }

        //---

    } // canvas
} // rendering