/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameSurfaceCache.h"
#include "renderingFrameParams.h"
#include "renderingFrameView.h"
#include "renderingFrameViewCamera.h"

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

        RTTI_BEGIN_TYPE_ENUM(FragmentDrawBucket);
        RTTI_ENUM_OPTION(OpaqueNotMoving);
        RTTI_ENUM_OPTION(Opaque);
        RTTI_ENUM_OPTION(OpaqueMasked);
        RTTI_ENUM_OPTION(Transparent);
        RTTI_ENUM_OPTION(SelectionOutline);
        RTTI_END_TYPE();

        //---

        static base::res::StaticResource<ShaderLibrary> resDebugGeometryShader("engine/shaders/debug.fx");

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

        void RenderDebugFragments(command::CommandWriter& cmd, const FrameView& view, const FrameViewCamera& camera, const DebugGeometry& geom)
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
                    consts.WorldToScreen = camera.renderCameras()[0].data.WorldToScreen; // TODO: jitter or not ?
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

        static bool FilterFragmentType(const FrameView& view, FragmentDrawBucket bucket)
        {
            switch (bucket)
            {
                case FragmentDrawBucket::OpaqueNotMoving: return view.frame().filters & FilterBit::FragOpaqueNonMovable;
                case FragmentDrawBucket::OpaqueMasked: return view.frame().filters & FilterBit::FragOpaqueMasked;
                case FragmentDrawBucket::Opaque: return view.frame().filters & FilterBit::FragOpaqueSolid;
            }

            return true;
        }

        void RenderViewFragments(command::CommandWriter& cmd, const FrameView& view, const FrameViewCamera& camera, const FragmentRenderContext& context, const std::initializer_list<FragmentDrawBucket>& buckets)
        {
            PC_SCOPE_LVL1(RenderViewFragments);

            for (const auto bucket : buckets)
            {
                command::CommandWriterBlock block(cmd, base::TempString("{}", bucket));

                if (bucket == FragmentDrawBucket::DebugSolid)
                {
                    if (view.frame().filters & FilterBit::DebugGeometrySolid)
                        RenderDebugFragments(cmd, view, camera, view.frame().geometry.solid);
                }
                else
                {
                    for (auto* scene : view.scenes())
                    {
                        scene->drawList->iterateFragmentRanges(bucket, [&scene, &cmd, &context, &view](const Fragment* const* fragments, uint32_t count)
                            {
                                uint32_t index = 0;
                                while (index < count)
                                {
                                    auto firstHandlerType = fragments[index]->type;
                                    auto firstFragmentIndex = index;
                                    while (++index < count)
                                        if (fragments[index]->type != firstHandlerType)
                                            break;


                                    if (const auto* handler = scene->scene->fragmentHandlers()[(uint8_t)firstHandlerType])
                                        handler->handleRender(cmd, view, context, fragments + firstFragmentIndex, index - firstFragmentIndex);
                                }
                            });
                    }
                }
            }
        }

        //---

    } // scene
} // rendering

