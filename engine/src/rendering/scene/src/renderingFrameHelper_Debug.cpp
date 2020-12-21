/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\helpers #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameParams.h"
#include "renderingSelectable.h"

#include "renderingFrameHelper_Debug.h"

#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingShaderFile.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingShaderData.h"

#include "base/containers/include/stringBuilder.h"
#include "base/resource/include/resourceStaticResource.h"
#include "../../device/include/renderingGraphicsStates.h"

namespace rendering
{
    namespace scene
    {

        //---

        static base::res::StaticResource<ShaderFile> resDebugGeometryShader("/engine/shaders/debug.fx");

        //---

        struct DebugFragmentData
        {
            base::Matrix LocalToWorld;
            base::Vector4 ConstColor;
            EncodedSelectable Selectable;
        };

		FrameHelperDebug::FrameHelperDebug(IDevice* api)
            : m_device(api)
		{
            m_drawShader = resDebugGeometryShader.loadAndGet()->rootShader()->deviceShader();

            {
                GraphicsPassLayoutSetup setup;
                setup.color[0].format = ImageFormat::RGBA16F;
                setup.depth.format = ImageFormat::D24S8;
                m_passStandard = api->createGraphicsPassLayout(setup);
            }

            {
                GraphicsPassLayoutSetup setup;
                setup.color[0].format = ImageFormat::RGBA16F;
                m_passOverlay = api->createGraphicsPassLayout(setup);
            }

            {
                GraphicsRenderStatesSetup setup;
                setup.depth(true);
                setup.depthWrite(true);
                setup.depthFunc(CompareOp::LessEqual);

                setup.primitiveTopology(PrimitiveTopology::TriangleList);
                m_renderStatesSolidTriangles = api->createGraphicsRenderStates(setup);

                setup.primitiveTopology(PrimitiveTopology::LineList);
                m_renderStatesSolidLines = api->createGraphicsRenderStates(setup);
            }

            {
                GraphicsRenderStatesSetup setup;
                setup.depth(true);
                setup.depthWrite(false);
                setup.depthFunc(CompareOp::LessEqual);
                setup.blend(true);
                setup.blendFactor(0, BlendFactor::One, BlendFactor::OneMinusSrcAlpha);

                setup.primitiveTopology(PrimitiveTopology::TriangleList);
                m_renderStatesTransparentTriangles = api->createGraphicsRenderStates(setup);

                setup.primitiveTopology(PrimitiveTopology::LineList);
                m_renderStatesTransparentLines = api->createGraphicsRenderStates(setup);
            }

            {
                GraphicsRenderStatesSetup setup;
                setup.depth(false);
                setup.blend(true);
                setup.blendFactor(0, BlendFactor::One, BlendFactor::OneMinusSrcAlpha);

                setup.primitiveTopology(PrimitiveTopology::TriangleList);
                m_renderStatesOverlayTriangles = api->createGraphicsRenderStates(setup);

                setup.primitiveTopology(PrimitiveTopology::LineList);
                m_renderStatesOverlayLines = api->createGraphicsRenderStates(setup);
            }

            {
               
            }

          
		}

		FrameHelperDebug::~FrameHelperDebug()
		{
		}

        void FrameHelperDebug::ensureBufferSize(const FrameParams_DebugGeometry& geom)
        {
            uint32_t maxVertexSizeThisFrame = 0;
            maxVertexSizeThisFrame = std::max<uint32_t>(maxVertexSizeThisFrame, geom.solid.vertices().dataSize());
            maxVertexSizeThisFrame = std::max<uint32_t>(maxVertexSizeThisFrame, geom.transparent.vertices().dataSize());
            maxVertexSizeThisFrame = std::max<uint32_t>(maxVertexSizeThisFrame, geom.overlay.vertices().dataSize());
            maxVertexSizeThisFrame = std::max<uint32_t>(maxVertexSizeThisFrame, geom.screen.vertices().dataSize());

            if (maxVertexSizeThisFrame > m_maxVertexDataSize)
            {
                m_maxVertexDataSize = base::Align<uint32_t>(maxVertexSizeThisFrame, 65536);

                BufferCreationInfo info;
                info.allowVertex = true;
                info.allowDynamicUpdate = true;
                info.label = "DebugVertexBuffer";
                info.size = m_maxVertexDataSize;
                m_vertexBuffer = m_device->createBuffer(info);
            }

            uint32_t maxIndexSizeThisFrame = 0;
            maxIndexSizeThisFrame = std::max<uint32_t>(maxIndexSizeThisFrame, geom.solid.indices().dataSize());
            maxIndexSizeThisFrame = std::max<uint32_t>(maxIndexSizeThisFrame, geom.transparent.indices().dataSize());
            maxIndexSizeThisFrame = std::max<uint32_t>(maxIndexSizeThisFrame, geom.overlay.indices().dataSize());
            maxIndexSizeThisFrame = std::max<uint32_t>(maxIndexSizeThisFrame, geom.screen.indices().dataSize());

            if (maxIndexSizeThisFrame > m_maxIndexDataSize)
            {
                m_maxIndexDataSize = base::Align<uint32_t>(maxIndexSizeThisFrame, 65536);

                BufferCreationInfo info;
                info.allowIndex = true;
                info.allowDynamicUpdate = true;
                info.label = "DebugIndexBuffer";
                info.size = m_maxIndexDataSize;
                m_indexBuffer = m_device->createBuffer(info);
            }
        }

        void FrameHelperDebug::renderInternal(command::CommandWriter& cmd, const DebugGeometry& geom) const
        {
            command::CommandWriterBlock block(cmd, "DebugGeometry");

            // upload vertices
            {
                cmd.opTransitionLayout(m_vertexBuffer, ResourceLayout::VertexBuffer, ResourceLayout::CopyDest);
                auto* writePtr = cmd.opUpdateDynamicBufferPtrN<uint8_t>(m_vertexBuffer, 0, geom.vertices().dataSize());
                geom.vertices().copy(writePtr, geom.vertices().dataSize());
                cmd.opTransitionLayout(m_vertexBuffer, ResourceLayout::CopyDest, ResourceLayout::VertexBuffer);
            }

            // upload indices
            {
                cmd.opTransitionLayout(m_indexBuffer, ResourceLayout::IndexBuffer, ResourceLayout::CopyDest);
                auto* writePtr = cmd.opUpdateDynamicBufferPtrN<uint8_t>(m_indexBuffer, 0, geom.indices().dataSize());
                geom.indices().copy(writePtr, geom.indices().dataSize());
                cmd.opTransitionLayout(m_indexBuffer, ResourceLayout::CopyDest, ResourceLayout::IndexBuffer);
            }

            // bind buffers
            cmd.opBindVertexBuffer("DebugVertex"_id, m_vertexBuffer);
            cmd.opBindIndexBuffer(m_indexBuffer, ImageFormat::R32_UINT);

            /*// draw batches
            if (auto shader = resDebugGeometryShader.loadAndGet())
            {
                const auto vertexDataSize = geom.vertices().dataSize();
                const auto vertexExDataSize = geom.verticesEx().dataSize();
                const auto indicesDataSize = geom.indices().dataSize();
                if (!indicesDataSize)
                    return;

                if (!buffers.ensureSize(vertexDataSize, vertexExDataSize, indicesDataSize))
                    return;

                command::CommandWriterBlock block(cmd, "DebugGeometry");

                // upload buffers
                {
                    PC_SCOPE_LVL1(DebugFragmentsCopyData);
                    void* vertexDataPtr = cmd.opUpdateDynamicBufferPtr(buffers.vertexBuffer->view(), 0, vertexDataSize);
                    geom.vertices().copy(vertexDataPtr, vertexDataSize);
                    void* vertexExDataPtr = cmd.opUpdateDynamicBufferPtr(buffers.vertexBufferEx->view(), 0, vertexExDataSize);
                    geom.verticesEx().copy(vertexExDataPtr, vertexExDataSize);
                    void* indexDataPtr = cmd.opUpdateDynamicBufferPtr(buffers.indexBuffer->view(), 0, indicesDataSize);
                    geom.indices().copy(indexDataPtr, indicesDataSize);
                }

                // global params
                {
                    DebugFragmentParams::Constants consts;
                    consts.WorldToScreen = view.frame().camera.camera.worldToScreen().transposed(); // TODO: jitter or not ?
                    view.frame().camera.camera.worldToScreen();

                    DebugFragmentParams params;
                    params.Data = cmd.opUploadConstants(consts);
                    //params.Texture = ImageView::DefaultWhite();
                    //params.TextureArray = ImageView::DefaultWhite();
                    cmd.opBindParametersInline("DebugFragmentPass"_id, params);
                }

                // bind geometry


                // setup initial batch state
                auto lastType = DebugGeometryType::Solid;
                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);

                // process fragments one by one
                geom.elements().forEach([&cmd, &shader, &lastType](const DebugGeometryElement& entry)
                    {
                        // switch primitive type
                        if (entry.type != lastType)
                        {
                            lastType = entry.type;

                            switch (entry.type)
                            {
                            case scene::DebugGeometryType::Solid:
                                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
                                break;

                            case scene::DebugGeometryType::Lines:
                                cmd.opSetPrimitiveType(PrimitiveTopology::LineList);
                                break;

                            case scene::DebugGeometryType::Sprite:
                                cmd.opSetPrimitiveType(PrimitiveTopology::PointList);
                                break;
                            }
                        }

                        // draw
                        cmd.opDrawIndexed(shader, 0, entry.firstIndex, entry.numIndices);
                    });

                // reset state
                cmd.opSetDepthBias(0.0f, 0.0f, 0.0f);
                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
            }*/
        }

		void FrameHelperDebug::render(FrameViewRecorder& rec, const FrameParams_DebugGeometry& geom, const Setup& setup) const
		{
            PC_SCOPE_LVL0(RenderDebugFragments);

#if 0
            command::CommandWriter* solid = nullptr;
            command::CommandWriter* transparent = nullptr;
            command::CommandWriter* overlay = nullptr;



            if (auto shader = resDebugGeometryShader.loadAndGet())
            {
                const auto vertexDataSize = geom.vertices().dataSize();
                const auto vertexExDataSize = geom.verticesEx().dataSize();
                const auto indicesDataSize = geom.indices().dataSize();
                if (!indicesDataSize)
                    return;

                if (!buffers.ensureSize(vertexDataSize, vertexExDataSize, indicesDataSize))
                    return;

                command::CommandWriterBlock block(cmd, "DebugGeometry");

                // upload buffers
                {
                    PC_SCOPE_LVL1(DebugFragmentsCopyData);
                    void* vertexDataPtr = cmd.opUpdateDynamicBufferPtr(buffers.vertexBuffer->view(), 0, vertexDataSize);
                    geom.vertices().copy(vertexDataPtr, vertexDataSize);
                    void* vertexExDataPtr = cmd.opUpdateDynamicBufferPtr(buffers.vertexBufferEx->view(), 0, vertexExDataSize);
                    geom.verticesEx().copy(vertexExDataPtr, vertexExDataSize);
                    void* indexDataPtr = cmd.opUpdateDynamicBufferPtr(buffers.indexBuffer->view(), 0, indicesDataSize);
                    geom.indices().copy(indexDataPtr, indicesDataSize);
                }

                // global params
                {
                    DebugFragmentParams::Constants consts;
                    consts.WorldToScreen = view.frame().camera.camera.worldToScreen().transposed(); // TODO: jitter or not ?
                    view.frame().camera.camera.worldToScreen();

                    DebugFragmentParams params;
                    params.Data = cmd.opUploadConstants(consts);
                    //params.Texture = ImageView::DefaultWhite();
                    //params.TextureArray = ImageView::DefaultWhite();
                    cmd.opBindParametersInline("DebugFragmentPass"_id, params);
                }

                // bind geometry

                cmd.opBindVertexBuffer("DebugVertex"_id, buffers.vertexBuffer->view());
                cmd.opBindVertexBuffer("DebugVertexEx"_id, buffers.vertexBufferEx->view());
                cmd.opBindIndexBuffer(buffers.indexBuffer->view(), ImageFormat::R32_UINT);

                // setup initial batch state
                auto lastType = DebugGeometryType::Solid;
                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);

                // process fragments one by one
                geom.elements().forEach([&cmd, &shader, &lastType](const DebugGeometryElement& entry)
                    {
                        // switch primitive type
                        if (entry.type != lastType)
                        {
                            lastType = entry.type;

                            switch (entry.type)
                            {
                            case scene::DebugGeometryType::Solid:
                                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
                                break;

                            case scene::DebugGeometryType::Lines:
                                cmd.opSetPrimitiveType(PrimitiveTopology::LineList);
                                break;

                            case scene::DebugGeometryType::Sprite:
                                cmd.opSetPrimitiveType(PrimitiveTopology::PointList);
                                break;
                            }
                        }

                        // draw
                        cmd.opDrawIndexed(shader, 0, entry.firstIndex, entry.numIndices);
                    });

                // reset state
                cmd.opSetDepthBias(0.0f, 0.0f, 0.0f);
                cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
            }
#endif
		}

        //---

    } // scene
} // rendering

