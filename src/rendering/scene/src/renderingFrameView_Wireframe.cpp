/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view  #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameParams.h"
#include "renderingFrameResources.h"

#include "renderingFrameView_Wireframe.h"
#include "renderingFrameHelper_Compose.h"
#include "renderingFrameHelper_Debug.h"

#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingImage.h"
#include "renderingFrameHelper_Outline.h"

namespace rendering
{
    namespace scene
    {
		//---

		FrameViewWireframeRecorder::FrameViewWireframeRecorder()
            : FrameViewRecorder(nullptr)
			, viewBegin(nullptr)
			, viewEnd(nullptr)
			, depthPrePass(nullptr)
			, mainSolid(nullptr)
			, mainTransparent(nullptr)
			, selectionOutline(nullptr)
            , sceneOverlay(nullptr)
            , screenOverlay(nullptr)
		{}

		//---

		FrameViewWireframe::FrameViewWireframe(const FrameRenderer& frame, const Setup& setup)
			: FrameViewSingleCamera(frame, setup.camera, setup.viewport)
			, m_setup(setup)
		{
        }

		void FrameViewWireframe::render(command::CommandWriter& cmd)
		{
            // initialize the scaffolding of the view
            FrameViewWireframeRecorder rec;
            initializeCommandStreams(cmd, rec);

            // start cascade rendering

            // render scene into the pre-created command buffers
			if (auto* scene = m_frame.frame().scenes.mainScenePtr)
				scene->renderWireframeView(rec, *this, m_frame);

            // TODO: render background

            // render debug fragments
            if (m_frame.frame().filters & FilterBit::DebugGeometry)
            {
                DebugGeometryViewRecorder debugRec(&rec);
                debugRec.solid.attachBuffer(rec.mainSolid.opCreateChildCommandBuffer(false));
                debugRec.transparent.attachBuffer(rec.mainTransparent.opCreateChildCommandBuffer(false));
                debugRec.overlay.attachBuffer(rec.sceneOverlay.opCreateChildCommandBuffer(false));
                debugRec.screen.attachBuffer(rec.screenOverlay.opCreateChildCommandBuffer(false));
                m_frame.helpers().debug->render(debugRec, m_frame.frame().geometry, &m_camera);
            }

            // wait for all recording jobs to finish
            rec.finishRendering();
		}

        void FrameViewWireframe::initializeCommandStreams(command::CommandWriter& cmd, FrameViewWireframeRecorder& rec)
        {
            // view start 
            rec.viewBegin.attachBuffer(cmd.opCreateChildCommandBuffer());

            // bind camera params
            bindCamera(cmd);

            // depth pre-pass
            {
                command::CommandWriterBlock block(cmd, "DepthPrePass");

                FrameBuffer fb;
                fb.depth.view(m_frame.resources().sceneFullDepthRTV).clearDepth(1.0f).clearStencil(0);
                fb.depth.loadOp = LoadOp::Clear;
                fb.depth.storeOp = StoreOp::Store;

                cmd.opBeingPass(fb, 1, m_viewport);

                rec.depthPrePass.attachBuffer(cmd.opCreateChildCommandBuffer());

                cmd.opEndPass();
            }

            // main forward pass
            {
                command::CommandWriterBlock block(cmd, "Wireframe");

                FrameBuffer fb;
                fb.depth.view(m_frame.resources().sceneFullDepthRTV);
                fb.depth.loadOp = LoadOp::Keep;
                fb.depth.storeOp = StoreOp::Store;
                fb.color[0].view(m_frame.resources().sceneFullColorRTV);
                fb.color[0].loadOp = LoadOp::Clear;
                fb.color[0].storeOp = StoreOp::Store;

                if (m_frame.frame().clear.clear)
                    fb.color[0].clear(m_frame.frame().clear.clearColor);
                else
                    fb.color[0].clear(0.1f, 0.1f, 0.1f);

                cmd.opBeingPass(fb, 1, m_viewport);

                {
                    command::CommandWriterBlock block(cmd, "Solid");
                    rec.mainSolid.attachBuffer(cmd.opCreateChildCommandBuffer());
                }

                {
                    command::CommandWriterBlock block(cmd, "Transparent");
                    rec.mainTransparent.attachBuffer(cmd.opCreateChildCommandBuffer());
                }

                cmd.opEndPass();
            }

            // view end
            rec.viewEnd.attachBuffer(cmd.opCreateChildCommandBuffer());

            // selection outline depth
            {
                command::CommandWriterBlock block(cmd, "Outline");

                FrameBuffer fb;
                fb.depth.view(m_frame.resources().sceneOutlineDepthRTV).clearDepth(1.0f).clearStencil(0);

                cmd.opBeingPass(fb, 1, m_viewport);
                
                rec.selectionOutline.attachBuffer(cmd.opCreateChildCommandBuffer());

                cmd.opEndPass();
            }

            // make selection depth readable
            cmd.opTransitionLayout(m_frame.resources().sceneOutlineDepth, ResourceLayout::DepthWrite, ResourceLayout::ShaderResource);

            // overlay
            {
                command::CommandWriterBlock block(cmd, "Overlay");

                FrameBuffer fb;
                fb.color[0].view(m_frame.resources().sceneFullColorRTV);
                fb.color[0].loadOp = LoadOp::Keep; // TODO: do not clear if we have background scene
                fb.color[0].storeOp = StoreOp::Store;

                cmd.opBeingPass(fb, 1, m_viewport);

                rec.sceneOverlay.attachBuffer(cmd.opCreateChildCommandBuffer());

                cmd.opEndPass();
            }

            // resolve color
            {
                cmd.opTransitionLayout(m_frame.resources().sceneFullColor, ResourceLayout::RenderTarget, ResourceLayout::ResolveSource);
                cmd.opTransitionLayout(m_frame.resources().sceneResolvedColor, ResourceLayout::ShaderResource, ResourceLayout::ResolveDest);

                cmd.opResolve(m_frame.resources().sceneFullColor, m_frame.resources().sceneResolvedColor);

                cmd.opTransitionLayout(m_frame.resources().sceneFullColor, ResourceLayout::ResolveSource, ResourceLayout::RenderTarget);
                cmd.opTransitionLayout(m_frame.resources().sceneResolvedColor, ResourceLayout::ResolveDest, ResourceLayout::ShaderResource);
            }

            // resolve depth
            {
                cmd.opTransitionLayout(m_frame.resources().sceneFullDepth, ResourceLayout::DepthWrite, ResourceLayout::ResolveSource);
                cmd.opTransitionLayout(m_frame.resources().sceneResolvedDepth, ResourceLayout::ShaderResource, ResourceLayout::ResolveDest);
                cmd.opResolve(m_frame.resources().sceneFullDepth, m_frame.resources().sceneResolvedDepth);
                cmd.opTransitionLayout(m_frame.resources().sceneFullDepth, ResourceLayout::ResolveSource, ResourceLayout::DepthRead);
                cmd.opTransitionLayout(m_frame.resources().sceneResolvedDepth, ResourceLayout::ResolveDest, ResourceLayout::ShaderResource);
            }

            // screen pass
            {
                command::CommandWriterBlock block(cmd, "Screen");

                FrameBuffer fb;
                fb.color[0].view(m_setup.colorTarget);
                fb.depth.view(m_setup.depthTarget);

                cmd.opBeingPass(fb, 1, m_setup.viewport);

                // present
                {
                    FrameHelperCompose::Setup setup;
                    setup.gameWidth = m_frame.width();
                    setup.gameHeight = m_frame.height();
                    setup.gameView = m_frame.resources().sceneResolvedColorSRV;
                    setup.presentTarget = m_setup.colorTarget;
                    setup.presentRect = m_setup.viewport;
                    setup.gamma = 1.0f / 2.2f;
                    m_frame.helpers().compose->finalCompose(cmd, setup);
                }

                // selection outline
                {
                    FrameHelperOutline::Setup setup;
                    setup.presentRect = m_setup.viewport;
                    setup.innerOpacity = m_frame.frame().selectionOutline.centerOpacity;
                    setup.width = m_frame.frame().selectionOutline.outlineWidth;
                    setup.primaryColor = m_frame.frame().selectionOutline.colorFront;
                    setup.backgroundColor = m_frame.frame().selectionOutline.colorBack;
                    setup.outlineDepth = m_frame.resources().sceneOutlineDepthSRV;
                    setup.sceneDepth = m_frame.resources().sceneResolvedDepthSRV;
                    setup.sceneWidth = m_frame.width();
                    setup.sceneHeight = m_frame.height();
                    m_frame.helpers().outline->drawOutlineEffect(cmd, setup);
                }

                // screen overlay
                {
                    rec.screenOverlay.attachBuffer(cmd.opCreateChildCommandBuffer());
                }

                // end screen pass
                cmd.opEndPass();
            }
        }

#if 0
        //---

		FrameViewWireframe::FrameViewWireframe(const FrameRenderer& frame, const Setup& setup)
			: m_frame(frame)
			, m_setup(setup)
		{}

		FrameViewWireframe::~FrameViewWireframe()
		{
		}

		void FrameViewWireframe::render(command::CommandWriter& cmd)
		{
			FrameViewWireframeParams params;

			// TODO: emit the light probe rendering requests
			// TODO: emit the light shadow map rendering requests
			params.globalLighting = frame.frame().globalLighting;
			//lightingData.globalShadowMaskAO = frame.surfaces().m_globalAOShadowMaskRT;

			// render cascades
			if (frame.frame().filters & FilterBit::CascadeShadows)
			{
				/*// calculate cascades
				cascades.cascadeShadowMap = frame.surfaces().m_cascadesShadowDepthRT;
				CalculateCascadeSettings(frame().globalLighting.globalLightDirection, m_camera, frame.frame().cascades, cascades);

				// render cascades
				// TODO: fiber
				if (cascades.numCascades)
				{
					FrameView_CascadeShadows cascadeView(renderer(), cascades);
					cascadeView.render(parentCmd);
				}*/
			}

			// bind all global view descriptors before branching into rendering pass command lists
			//BindSingleCamera(cmd, m_camera);
			//BindShadowsData(cmd, cascades);

			// create command buffer writers


		}

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
#endif

    } // scene
} // rendering

