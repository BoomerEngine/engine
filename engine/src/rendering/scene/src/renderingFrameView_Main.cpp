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

        void FrameView_Main::render(command::CommandWriter& parentCmd)
        {
            PC_SCOPE_LVL1(MainView);

            // collect fragments from the main camera
            collectSingleCamera(m_camera);

            // TODO: emit the light probe rendering requests
            // TODO: emit the light shadow map rendering requests
            LightingData lightingData;
            lightingData.globalLighting = frame().globalLighting;
            lightingData.globalShadowMaskAO = renderer().surfaces().m_globalAOShadowMaskRT;

            // prepare fragments for the visible objects
            {
                command::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer(), "MainFragments");
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
                if (cascades.numCascades)
                {
                    FrameView_CascadeShadows cascadeView(renderer(), cascades);
                    cascadeView.render(parentCmd);
                }
            }

            //--

            // render passes
            {
                command::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer(), "MainView");
                base::ScopeTimer timer;

                // bind global settings
                BindSingleCamera(cmd, m_camera);
                BindShadowsData(cmd, cascades);

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
                    const auto& veloicityBuffer = renderer().surfaces().m_velocityBufferRT;
                    RenderDepthPrepass(cmd, *this, m_depthTarget, veloicityBuffer);

                    // TODO: generate "horizon mask" texture for shadow cascade rendering

                    // linearize depth
                    const auto& linearizedDepth = renderer().surfaces().m_linarizedDepthRT;
                    LinearizeDepth(cmd, width(), height(), m_camera, m_depthTarget, linearizedDepth);

                    // reconstruct view space normals from the linearized depth
                    const auto& reconstructedNormals = renderer().surfaces().m_viewNormalRT;
                    ReconstructViewNormals(cmd, width(), height(), m_camera, linearizedDepth, reconstructedNormals);

                    // compute global shadow mask and finally bind lighting parameters so the forward pass (and following passes) may use it
                    GlobalShadowMask(cmd, width(), height(), m_depthTarget, lightingData.globalShadowMaskAO);

                    // compute ambient occlusion
                    HBAOIntoShadowMask(cmd, width(), height(), linearizedDepth, lightingData.globalShadowMaskAO, m_camera, frame().ao);

                    // render forward SOLID pass
                    BindLightingData(cmd, lightingData); // make lighting data available for forward rendering
                    RenderForwardPass(cmd, *this, m_depthTarget, m_colorTarget);

                    // capture screen color for any transparencies

                    // render forward TRANSPARENT pass

                    // visualize content
                    if (m_mode == FrameRenderMode::DebugDepth)
                        VisualizeDepthBuffer(cmd, width(), height(), m_depthTarget, m_colorTarget);
                    else if (m_mode == FrameRenderMode::DebugLuminance)
                        VisualizeLuminance(cmd, width(), height(), m_colorTarget, m_colorTarget);
                    else if (m_mode == FrameRenderMode::DebugShadowMask)
                        VisualizeTexture(cmd, width(), height(), lightingData.globalShadowMaskAO, m_colorTarget, base::Vector4(1, 0, 0, 0));
                    else if (m_mode == FrameRenderMode::DebugAmbientOcclusion)
                        VisualizeTexture(cmd, width(), height(), lightingData.globalShadowMaskAO, m_colorTarget, base::Vector4(0, 1, 0, 0));
                    else if (m_mode == FrameRenderMode::DebugLinearizedDepth)
                        VisualizeLinearDepthBuffer(cmd, width(), height(), linearizedDepth, m_colorTarget);
                    else if (m_mode == FrameRenderMode::DebugReconstructedViewNormals)
                        VisualizeTexture(cmd, width(), height(), reconstructedNormals, m_colorTarget, base::Vector4(0, 0, 0, 0), base::Vector4(1,1,1,0));
                }

                // stats
                m_stats.recordTime += timer.timeElapsed();
            }
        }

        //--

    } // scene
} // rendering

