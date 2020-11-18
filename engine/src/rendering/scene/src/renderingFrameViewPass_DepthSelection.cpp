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
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/mesh/include/renderingMeshService.h"

namespace rendering
{
    namespace scene
    {
        
        //---

        void RenderDepthSelection(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT)
        {
            PassBracket pass(cmd, view, "DepthSelection");
            pass.depthClear(depthRT);
            pass.begin();

            FragmentRenderContext context;
            context.msaaCount = depthRT.numSamples();
            context.filterFlags = &view.frame().filters;
            context.allowsCustomRenderStates = false;
            context.depthCompare = CompareOp::LessEqual;
            context.pass = MaterialPass::DepthPrepass;

            cmd.opSetDepthState(true, true, context.depthCompare);

            // render fragments only if we are allowed to
            if (view.frame().filters & FilterBit::PassDepthSelection)
                RenderViewFragments(cmd, view, context, { FragmentDrawBucket::SelectionOutline });
        }

        //---

    } // scene
} // rendering

