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

        void RenderWireframePass(command::CommandWriter& cmd, const FrameView& view, const ImageView& depthRT, const ImageView& colorRT, bool solid)
        {
            PassBracket pass(cmd, view, "Wireframe");
            pass.depthClear(depthRT);
            pass.colorClear(0, colorRT);
            pass.begin();

            FragmentRenderContext context;
            context.msaaCount = depthRT.numSamples();
            context.filterFlags = &view.frame().filters;
            context.allowsCustomRenderStates = false;

            if (solid)
            {
                context.pass = MaterialPass::Wireframe;
                cmd.opSetDepthState(true, true, CompareOp::LessEqual);
                cmd.opSetFillState(PolygonMode::Fill);
            }
            else
            {
                context.pass = MaterialPass::Wireframe;

                cmd.opSetDepthState(true, true, CompareOp::LessEqual);
                cmd.opSetFillState(PolygonMode::Line);
            }

            RenderViewFragments(cmd, view, context, {
                FragmentDrawBucket::OpaqueNotMoving,
                FragmentDrawBucket::Opaque,
                FragmentDrawBucket::DebugSolid,
                FragmentDrawBucket::OpaqueMasked });

            cmd.opSetFillState(PolygonMode::Fill);
        }

        //--

    } // scene
} // rendering

