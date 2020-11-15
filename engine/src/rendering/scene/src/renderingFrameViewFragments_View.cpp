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

        void RenderViewFragments(command::CommandWriter& cmd, const FrameView& view, const FragmentRenderContext& context, const std::initializer_list<FragmentDrawBucket>& buckets)
        {
            PC_SCOPE_LVL1(RenderViewFragments);

            for (const auto bucket : buckets)
            {
                command::CommandWriterBlock block(cmd, base::TempString("{}", bucket));

                FrameFragmentBucketStats localBucketStat;

                if (bucket == FragmentDrawBucket::DebugSolid)
                {
                    if (view.frame().filters & FilterBit::DebugGeometrySolid)
                        RenderDebugFragments(cmd, view, view.frame().geometry.solid);
                }
                else if (bucket == FragmentDrawBucket::DebugOverlay)
                {
                    if (view.frame().filters & FilterBit::DebugGeometryOverlay)
                        RenderDebugFragments(cmd, view, view.frame().geometry.overlay);
                }
                else
                {
                    for (auto* scene : view.scenes())
                    {
                        scene->drawList->iterateFragmentRanges(bucket, [&scene, &localBucketStat, &cmd, &context, &view](const Fragment* const* fragments, uint32_t count)
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
                                        handler->handleRender(cmd, view, context, fragments + firstFragmentIndex, index - firstFragmentIndex, localBucketStat.types[(uint8_t)firstHandlerType]);
                                }
                            });
                    }
                }

                // merge stats
                view.stats().buckets[(int)bucket].merge(localBucketStat);
            }
        }

        //---

    } // scene
} // rendering

