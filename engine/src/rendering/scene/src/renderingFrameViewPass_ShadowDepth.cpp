/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\pass  #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneUtils.h"
#include "renderingSceneFragment.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameSurfaceCache.h"
#include "renderingFrameParams.h"
#include "renderingFrameView.h"

#include "base/containers/include/stringBuilder.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingCommandBuffer.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/mesh/include/renderingMeshService.h"

namespace rendering
{
    namespace scene
    {

        //---

        void RenderShadowDepthPass(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT, uint32_t index)
        {
            PassBracket pass(cmd, view, "ShadowDepthPass");
            pass.depthClear(depthRT);
            pass.begin();

            FragmentRenderContext context;
            context.msaaCount = depthRT.numSamples();
            context.filterFlags = &view.frame().filters;
            context.pass = MaterialPass::ShadowDepth;

            cmd.opSetDepthState(true, true, CompareOp::LessEqual);

            // render fragments only if we are allowed to
            if (view.frame().filters & FilterBit::PassShadowDepth)
            {
                const FragmentDrawBucket buckets[4] = { FragmentDrawBucket::ShadowDepth0, FragmentDrawBucket::ShadowDepth1, FragmentDrawBucket::ShadowDepth2, FragmentDrawBucket::ShadowDepth3 };
                RenderViewFragments(cmd, view, context, { buckets[index] });
            }
        }

        //---

    } // scene
} // rendering

