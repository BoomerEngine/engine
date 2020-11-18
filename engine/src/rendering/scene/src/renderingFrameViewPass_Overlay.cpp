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

        void RenderOverlay(command::CommandWriter& cmd, const FrameView& view, const ImageView& colorRT)
        {
            PassBracket pass(cmd, view, "Overlay");
            pass.colorNoClear(0, colorRT);
            pass.begin();

            FragmentRenderContext context;
            context.msaaCount = colorRT.numSamples();
            context.filterFlags = &view.frame().filters;
            context.pass = MaterialPass::Forward;
            context.depthCompare = CompareOp::Always;

            cmd.opSetDepthState(false, false, context.depthCompare);

            // render fragments only if we are allowed to
            if (view.frame().filters & FilterBit::PassOverlay)
            {
                // solid
                RenderViewFragments(cmd, view, context, { 
                    FragmentDrawBucket::DebugOverlay,
                    });

                // transparent
            }
            else
            {
                // ?
            }
        }

        //---

    } // scene
} // rendering

