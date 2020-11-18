/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\fragments #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameSurfaceCache.h"
#include "renderingFrameParams.h"
#include "renderingFrameView.h"

#include "renderingSelectable.h"
#include "renderingSceneFragmentList.h"

#include "base/containers/include/stringBuilder.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingShaderLibrary.h"
#include "../../device/include/renderingDeviceService.h"

namespace rendering
{
    namespace scene
    {

        //---

        static base::res::StaticResource<ShaderLibrary> resDebugGeometryShader("/engine/shaders/debug.fx");

        //---

        struct DebugFragmentData
        {
            base::Matrix LocalToWorld;
            base::Vector4 ConstColor;
            EncodedSelectable Selectable;
        };

        bool DebugFragmentBuffers::ensureSize(uint64_t vertexDataSize, uint64_t vertexExDataSize, uint64_t indicesDataSize)
        {
            if (vertexDataSize >= INDEX_MAX || vertexExDataSize >= INDEX_MAX || indicesDataSize >= INDEX_MAX)
                return false;

            auto dev = base::GetService<DeviceService>()->device();

            if (!vertexBuffer || vertexBuffer->view().size() < vertexDataSize)
            {
                BufferCreationInfo info;
                info.allowDynamicUpdate = true;
                info.allowCopies = true;
                info.allowVertex = true;
                info.label = "DebugFragmentVertices";
                info.size = base::Align<uint32_t>(vertexDataSize, 65536);
                vertexBuffer = dev->createBuffer(info);
            }

            if (!vertexBufferEx || vertexBufferEx->view().size() < vertexExDataSize)
            {
                BufferCreationInfo info;
                info.allowDynamicUpdate = true;
                info.allowCopies = true;
                info.allowVertex = true;
                info.label = "DebugFragmentVerticesEx";
                info.size = base::Align<uint32_t>(vertexExDataSize, 65536);
                vertexBufferEx = dev->createBuffer(info);
            }

            if (!indexBuffer || indexBuffer->view().size() < indicesDataSize)
            {
                BufferCreationInfo info;
                info.allowDynamicUpdate = true;
                info.allowCopies = true;
                info.allowIndex = true;
                info.label = "DebugFragmentIndices";
                info.size = base::Align<uint32_t>(indicesDataSize, 65536);
                indexBuffer = dev->createBuffer(info);
            }

            return vertexBuffer && vertexBufferEx && indexBuffer;
        }

        struct DebugFragmentParams
        {
            struct Constants
            {
                base::Matrix WorldToScreen;
            };

            ConstantsView Data;
            //ImageView Texture;
            //ImageView TextureArray;
        };

        void RenderDebugFragments(command::CommandWriter& cmd, const FrameView& view, const DebugGeometry& geom)
        {
            PC_SCOPE_LVL0(RenderDebugFragments);

            auto& buffers = view.renderer().surfaces().debugBuffers();

            if (!(view.frame().filters & FilterBit::DebugGeometry))
                return;

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
        }

        //---

    } // scene
} // rendering

