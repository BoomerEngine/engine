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
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingCommandBuffer.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingShaderLibrary.h"

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

            if (!(view.frame().filters & FilterBit::DebugGeometry))
                return;

            if (auto shader = resDebugGeometryShader.loadAndGet())
            {
                const auto vertexDataSize = geom.vertices().dataSize();
                const auto vertexExDataSize = geom.verticesEx().dataSize();
                const auto indicesDataSize = geom.indices().dataSize();
                if (!indicesDataSize)
                    return;

                command::CommandWriterBlock block(cmd, "DebugGeometry");

                // create buffers
                auto vertexData = TransientBufferView(BufferViewFlag::Vertex, TransientBufferAccess::ShaderReadOnly, vertexDataSize);
                auto vertexExData = TransientBufferView(BufferViewFlag::Vertex, TransientBufferAccess::ShaderReadOnly, vertexExDataSize);
                auto indexData = TransientBufferView(BufferViewFlag::Index, TransientBufferAccess::ShaderReadOnly, indicesDataSize);

                // upload buffers
                void* vertexDataPtr = cmd.opAllocTransientBufferWithUninitializedData(vertexData, vertexDataSize);
                geom.vertices().copy(vertexDataPtr, vertexDataSize);
                void* vertexExDataPtr = cmd.opAllocTransientBufferWithUninitializedData(vertexExData, vertexExDataSize);
                geom.verticesEx().copy(vertexExDataPtr, vertexExDataSize);
                void* indexDataPtr = cmd.opAllocTransientBufferWithUninitializedData(indexData, indicesDataSize);
                geom.indices().copy(indexDataPtr, indicesDataSize);

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

                cmd.opBindVertexBuffer("DebugVertex"_id, vertexData);
                cmd.opBindVertexBuffer("DebugVertexEx"_id, vertexExData);
                cmd.opBindIndexBuffer(indexData, ImageFormat::R32_UINT);

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

