/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneUtils.h"
#include "renderingSceneFragment.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameSurfaceCache.h"
#include "renderingFrameViewPass.h"
#include "renderingFrameParams.h"
#include "renderingFrameView.h"
#include "renderingFrameViewFragments.h"

#include "base/containers/include/stringBuilder.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingCommandBuffer.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/mesh/include/renderingMeshService.h"
#include "renderingFrameViewCamera.h"

namespace rendering
{
    namespace scene
    {
        //---

        base::ConfigProperty<base::Color> cvRenderingDefaultClearColor("Rendering.Scene", "DefaultClearColor", base::Color(50, 50, 50, 255));

        //---

        PassBracket::PassBracket(command::CommandWriter& cmd, const FrameView& view, base::StringView<char> name)
            : m_cmd(cmd)
            , m_view(view)
        {
            cmd.opBeginBlock(name);

            m_viewport.minDepthRange = 0.0f;
            m_viewport.maxDepthRange = 1.0f;
            m_viewport.viewportRect = base::Rect(0, 0, view.frame().resolution.width, view.frame().resolution.height);
        }

        void PassBracket::depthClear(const ImageView& rt)
        {
            DEBUG_CHECK(!rt.empty());
            DEBUG_CHECK(rt.renderTargetDepth());

            m_fb.depth.rt = rt;
            m_fb.depth.loadOp = LoadOp::Clear;
            m_fb.depth.clearDepth(1.0f);
            m_fb.depth.clearStencil(0);
        }

        void PassBracket::colorClear(uint8_t index, const ImageView& rt, const base::Vector4& clearValues)
        {
            DEBUG_CHECK(!rt.empty());
            DEBUG_CHECK(rt.renderTarget());

            m_fb.color[index].rt = rt;
            m_fb.color[index].loadOp = LoadOp::Clear;
            m_fb.color[index].clearColorValues[0] = clearValues.x;
            m_fb.color[index].clearColorValues[1] = clearValues.y;
            m_fb.color[index].clearColorValues[2] = clearValues.z;
            m_fb.color[index].clearColorValues[3] = clearValues.w;
        }

        void PassBracket::begin()
        {
            DEBUG_CHECK(!m_hasStartedPass);
            m_cmd.opBeingPass(m_fb, 1, &m_viewport);
            m_hasStartedPass = true;
        }

        PassBracket::~PassBracket()
        {
            if (m_hasStartedPass)
                m_cmd.opEndPass();

            m_cmd.opEndBlock();
        }

        //---

        void RenderDepthPrepass(command::CommandWriter& cmd, const FrameView& view, const FrameViewCamera& camera, const ImageView& depthRT)
        {
            PassBracket pass(cmd, view, "DepthPrepass");
            pass.depthClear(depthRT);
            pass.begin();

            FragmentRenderContext context;
            context.msaaCount = depthRT.numSamples();
            context.filterFlags = &view.frame().filters;
            context.pass = MaterialPass::DepthPrepass;

            cmd.opSetDepthState(true, true, CompareOp::LessEqual);

            // render fragments only if we are allowed to
            if (view.frame().filters & FilterBit::PassDepthPrepass)
            {
                // TODO: re project previous frame Z-buffer from static-immovable fragments as occlusion for current frame 

                RenderViewFragments(cmd, view, camera, context, { FragmentDrawBucket::OpaqueNotMoving });

                // TODO: capture the Z buffer 
                // TODO: merge with previous frame Z 

                RenderViewFragments(cmd, view, camera, context, { FragmentDrawBucket::Opaque, FragmentDrawBucket::DebugSolid, FragmentDrawBucket::OpaqueMasked });
            }
            else
            {
                // still need to capture the Z buffer even if it's empty
            }
        }

        //---

        void RenderForwardPass(command::CommandWriter& cmd, const FrameView& view, const FrameViewCamera& camera, const ImageView& depthRT, const ImageView& colorRT)
        {
            PassBracket pass(cmd, view, "Forward");
            pass.depthClear(depthRT);
            pass.colorClear(0, colorRT, cvRenderingDefaultClearColor.get().toVectorSRGB());
            pass.begin();

            FragmentRenderContext context;
            context.msaaCount = depthRT.numSamples();
            context.filterFlags = &view.frame().filters;
            context.pass = MaterialPass::Forward;

            cmd.opSetDepthState(true, true, CompareOp::LessEqual);

            // render fragments only if we are allowed to
            if (view.frame().filters & FilterBit::PassForward)
            {
                // solid
                RenderViewFragments(cmd, view, camera, context, { 
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

        void RenderWireframePass(command::CommandWriter& cmd, const FrameView& view, const FrameViewCamera& camera, const ImageView& depthRT, const ImageView& colorRT, bool solid)
        {
            PassBracket pass(cmd, view, "Wireframe");
            pass.depthClear(depthRT);
            pass.colorClear(0, colorRT, cvRenderingDefaultClearColor.get().toVectorSRGB());
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

            RenderViewFragments(cmd, view, camera, context, {
                FragmentDrawBucket::OpaqueNotMoving,
                FragmentDrawBucket::Opaque,
                FragmentDrawBucket::DebugSolid,
                FragmentDrawBucket::OpaqueMasked });

            cmd.opSetFillState(PolygonMode::Fill);
        }

        //--

    } // scene
} // rendering

