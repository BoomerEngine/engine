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
#include "renderingFrameParams.h"
#include "renderingFrameView.h"
#include "renderingSceneCulling.h"
#include "renderingSceneProxy.h"
#include "renderingSceneFragmentList.h"
#include "renderingFrameView_Main.h"
#include "renderingFrameView_Cascades.h"

namespace rendering
{
    namespace scene
    {

        //---

        FrameView_Main::FrameView_Main(const FrameRenderer& frame, const Camera& camera, ImageView colorTarget, ImageView depthTarget, FrameRenderMode mode /*= FrameRenderMode::Default*/)
            : FrameView(frame, FrameViewType::MainColor, frame.frame().resolution.width, frame.frame().resolution.height)
            , m_camera(camera)
            , m_colorTarget(colorTarget)
            , m_depthTarget(depthTarget)
            , m_mode(mode)
        {}

        void FrameView_Main::render(command::CommandWriter& cmd)
        {
            PC_SCOPE_LVL1(MainView);

            // collect fragments from the main camera
            collectSingleCamera(m_camera);

            // TODO: emit the light probe rendering requests
            // TODO: emit the light shadow map rendering requests

            // prepare fragments for the visible objects
            {
                //command::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer(), "MainFragments");
                generateFragments(cmd);
            }

            //--

            // render cascades
            CascadeData cascades;
            if (frame().filters & FilterBit::CascadeShadows)
            {
                // calculate cascades
                cascades.cascadeShadowMap = renderer().surfaces().m_cascadesShadowDepthRT;
                CalculateCascadeSettings(frame().globalLighting.globalLightDirection, m_camera, frame().cascades, cascades);

                // render cascades
                // TODO: fiber
                FrameView_CascadeShadows cascadeView(renderer(), cascades);
                cascadeView.render(cmd);
            }

            //--

            // render passes
            {
                //command::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer(), "MainView");

                // bind global settings
                BindSingleCamera(cmd, m_camera);
                BindLightingData(cmd, cascades);

                // render passes
                if (m_mode == FrameRenderMode::WireframeSolid)
                {
                    RenderWireframePass(cmd, *this, m_depthTarget, m_colorTarget, true);
                }
                else if (m_mode == FrameRenderMode::WireframePassThrough)
                {
                    RenderWireframePass(cmd, *this, m_depthTarget, m_colorTarget, false);
                }
                else
                {
                    // depth pre pass for the scene
                    RenderDepthPrepass(cmd, *this, m_depthTarget);

                    // TODO: generate "horizon mask" texture for shadow cascade rendering

                    // compute global shadow mask
                    const auto& shadowMask = renderer().surfaces().m_globalAOShadowMaskRT;
                    GlobalShadowMask(cmd, width(), height(), m_depthTarget, shadowMask);

                    // render forward pass
                    RenderForwardPass(cmd, *this, m_depthTarget, m_colorTarget);

                    // visualize special content
                    if (m_mode == FrameRenderMode::DebugDepth)
                    {
                        VisualizeDepthBuffer(cmd, width(), height(), m_depthTarget, m_colorTarget);
                    }
                    else if (m_mode == FrameRenderMode::DebugLuminance)
                    {
                        //VisualizeLuminance(cmd, width(), height(), m_colorTarget, m_colorTarget);
                    }
                    else if (m_mode == FrameRenderMode::DebugShadowMask)
                    {
                        VisualizeTexture(cmd, width(), height(), shadowMask, m_colorTarget, base::Vector4(1, 0, 0, 0));
                    }
                }
            }
        }

        //--

    } // scene
} // rendering

