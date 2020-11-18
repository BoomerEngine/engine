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

        void RenderForwardPass(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT, const ImageView& colorRT)
        {
            PassBracket pass(cmd, view, "Forward");
            pass.depthNoClear(depthRT);
            pass.colorClear(0, colorRT);
            pass.begin();

            FragmentRenderContext context;
            context.msaaCount = depthRT.numSamples();
            context.filterFlags = &view.frame().filters;
            context.pass = MaterialPass::Forward;
            context.depthCompare = CompareOp::Equal;

            //cmd.opSetDepthState(true, true, CompareOp::LessEqual);
            cmd.opSetDepthState(true, true, context.depthCompare);

            // render fragments only if we are allowed to
            if (view.frame().filters & FilterBit::PassForward)
            {
                // solid
                RenderViewFragments(cmd, view, context, { 
                    FragmentDrawBucket::OpaqueNotMoving, 
                    FragmentDrawBucket::Opaque, 
                    FragmentDrawBucket::DebugSolid, 
                    FragmentDrawBucket::OpaqueMasked });

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

