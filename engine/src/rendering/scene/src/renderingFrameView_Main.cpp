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
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameParams.h"
#include "renderingFrameResources.h"

#include "renderingFrameView_Main.h"
#include "renderingFrameView_Cascades.h"
#include "renderingFrameHelper_Compose.h"
#include "renderingFrameHelper_Debug.h"

#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingImage.h"

namespace rendering
{
    namespace scene
    {
		//---

		FrameViewMainRecorder::FrameViewMainRecorder()
            : FrameViewRecorder(nullptr)
			, viewBegin(nullptr)
			, viewEnd(nullptr)
			, depthPrePassStatic(nullptr)
			, depthPrePassOther(nullptr)
			, forwardSolid(nullptr)
			, forwardMasked(nullptr)
			, forwardTransparent(nullptr)
			, selectionOutline(nullptr)
            , sceneOverlay(nullptr)
		{}

		//---

		FrameViewMain::FrameViewMain(const FrameRenderer& frame, const Setup& setup)
			: m_frame(frame)
			, m_setup(setup)
		{}

		FrameViewMain::~FrameViewMain()
		{

		}

		void FrameViewMain::render(command::CommandWriter& cmd)
		{
            // initialize the scaffolding of the view
            FrameViewMainRecorder rec;
            initializeCommandStreams(cmd, rec);

            // render main scene into the pre-created command buffers
			if (auto* scene = m_frame.frame().scenes.mainScenePtr)
				scene->renderMainView(rec, *this, m_frame);

            // TODO: render background

            // render debug fragments
            if (m_frame.frame().filters & FilterBit::DebugGeometry)
            {


            }

            // wait for all recording jobs to finish
            rec.finishRendering();
		}

        void FrameViewMain::initializeCommandStreams(command::CommandWriter& cmd, FrameViewMainRecorder& rec)
        {
            // view start 
            rec.viewBegin.attachBuffer(cmd.opCreateChildCommandBuffer());

            // TODO: reproject previous frame depth for occlusion culling

            // bind camera params
            bindCamera(cmd);

            // depth pre-pass
            {
                FrameBuffer fb;
                fb.depth.view(m_frame.resources().sceneFullDepthRTV).clearDepth(1.0f).clearStencil(0);
                fb.depth.loadOp = LoadOp::Clear;
                fb.depth.storeOp = StoreOp::Store;

                cmd.opBeingPass(fb);

                rec.depthPrePassStatic.attachBuffer(cmd.opCreateChildCommandBuffer());

                // TODO: capture depth

                rec.depthPrePassOther.attachBuffer(cmd.opCreateChildCommandBuffer());

                cmd.opEndPass();
            }

            // TODO: capture depth for this-frame occlusion culling

            // wait for cascades

            // bind lighting params

            // main forward pass
            {
                FrameBuffer fb;
                fb.depth.view(m_frame.resources().sceneFullDepthRTV);
                fb.depth.loadOp = LoadOp::Keep;
                fb.depth.storeOp = StoreOp::Store;
                fb.color[0].view(m_frame.resources().sceneFullColorRTV);
                fb.color[0].storeOp = StoreOp::Store;

                if (m_frame.frame().clear.clear)
                {
                    fb.color[0].loadOp = LoadOp::Clear; // TODO: do not clear if we have background scene
                    fb.color[0].clear(m_frame.frame().clear.clearColor);
                }
                else
                {
                    fb.color[0].loadOp = LoadOp::Keep;
                }

                //fb.color[0].clear(0.5f, 0.5f, 0.5f, 1.0f);

                cmd.opBeingPass(fb);

                rec.forwardSolid.attachBuffer(cmd.opCreateChildCommandBuffer());

                rec.forwardMasked.attachBuffer(cmd.opCreateChildCommandBuffer());

                // TODO: capture color buffer for refractions

                rec.forwardTransparent.attachBuffer(cmd.opCreateChildCommandBuffer());

                cmd.opEndPass();
            }

            // view end
            rec.viewEnd.attachBuffer(cmd.opCreateChildCommandBuffer());

            // selection outline depth
            {
                FrameBuffer fb;
                fb.depth.view(m_frame.resources().sceneOutlineDepthRTV).clearDepth(1.0f).clearStencil(0);

                cmd.opBeingPass(fb);

                rec.selectionOutline.attachBuffer(cmd.opCreateChildCommandBuffer());

                cmd.opEndPass();
            }

            // overlay
            {
                FrameBuffer fb;
                fb.color[0].view(m_frame.resources().sceneFullColorRTV);
                fb.color[0].loadOp = LoadOp::Keep; // TODO: do not clear if we have background scene
                fb.color[0].storeOp = StoreOp::Store;

                cmd.opBeingPass(fb);

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
                //cmd.opTransitionLayout(m_frame.resources().sceneFullDepth, ResourceLayout::DepthWrite, ResourceLayout::ResolveSource);
                //cmd.opTransitionLayout(m_frame.resources().sceneResolvedDepth, ResourceLayout::ShaderResource, ResourceLayout::ResolveDest);
                //cmd.opResolve(m_frame.resources().sceneFullDepth, m_frame.resources().sceneResolvedDepth);
                //cmd.opTransitionLayout(m_frame.resources().sceneFullDepth, ResourceLayout::ResolveSource, ResourceLayout::DepthWrite);
                //cmd.opTransitionLayout(m_frame.resources().sceneResolvedDepth, ResourceLayout::ResolveDest, ResourceLayout::ShaderResource);
            }

            // present
            {
                FrameHelperCompose::Setup setup;
                setup.gameWidth = m_frame.width();
                setup.gameHeight = m_frame.height();
                setup.gameView = m_frame.resources().sceneResolvedColorSRV;
                setup.presentTarget = m_setup.colorTarget;
                setup.presentRect = m_setup.viewport;
                m_frame.helpers().compose->finalCompose(cmd, setup);
            }
        }

        void FrameViewMain::bindCamera(command::CommandWriter& cmd)
        {
            GPUCameraInfo cameraParams;
            CalcGPUSingleCamera(cameraParams, m_frame.frame().camera.camera);

            DescriptorEntry desc[1];
            desc[0].constants(cameraParams);
            cmd.opBindDescriptor("CameraParams"_id, desc);
        }

		void FrameViewMain::renderFragments(Scene* scene, FrameViewMainRecorder& rec)
		{
		}

		/*{
			struct
			{
				GPUCameraInfo camera;
				GPUFrameParameters frame;
			} viewConsts;

			GPUCameraInfo data;
			CalcGPUCameraInfo(data, camera, prevFrameCamera);



		}*/

#if 0
        //---

		FrameViewMain::FrameViewMain(const FrameRenderer& frame, const Setup& setup)
			: m_frame(frame)
			, m_setup(setup)
		{}

		FrameViewMain::~FrameViewMain()
		{
		}

		void FrameViewMain::render(command::CommandWriter& cmd)
		{
			FrameViewMainParams params;

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

