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
#include "renderingFrameViewCamera.h"
#include "renderingFrameViewPass.h"
#include "renderingFrameViewPostFX.h"

namespace rendering
{
    namespace scene
    {
        //---

        FrameView::FrameView(const FrameRenderer& frame, const FrameViewCamera& camera, base::StringView<char> name)
            : m_renderer(frame)
            , m_camera(camera)
            , m_frame(frame.m_frame)
            , m_allocator(frame.m_allocator.pageAllocator())
            , m_name(name)
        {           
        }

        FrameView::~FrameView()
        {
            for (auto* scene : m_scenes)
            {
                scene->drawList->~FragmentDrawList();
                scene->~PerSceneData();
            }
        }

        void FrameView::collect()
        {
            PC_SCOPE_LVL0(CollectView);

            const auto numScenes = m_renderer.m_scenes.size();
            m_scenes.reserve(numScenes);

            // create per scene data
            for (uint32_t i = 0; i < numScenes; ++i)
            {
                auto* data = m_allocator.createNoCleanup<PerSceneData>();
                data->scene = m_renderer.m_scenes[i].scene;
                data->params = m_renderer.m_scenes[i].params;
                data->drawList = m_allocator.createNoCleanup<FragmentDrawList>(m_allocator);
                m_scenes.pushBack(data);
            }

            // collect
            {
                PC_SCOPE_LVL1(CollectSingleCamera);
                for (auto* scene : m_scenes)
                {
                    // TODO: transform to scene space
                    SceneObjectCullingSetup sceneLocalCullingContext;
                    sceneLocalCullingContext.cameraPosition = m_camera.mainCamera().position();
                    //sceneLocalCullingContext.cameraFrustumMatrix = 
                    scene->scene->objects().cullObjects(sceneLocalCullingContext, scene->collectedObjects);
                }
            }
        }

        void FrameView::generateFragments(command::CommandWriter& cmd)
        {
            PC_SCOPE_LVL1(GenerateFragments);

            for (auto* scene : m_scenes)
            {
                for (uint32_t j = 1; j < (uint32_t)ProxyType::MAX; ++j)
                {
                    const auto& visibleObjectOfType = scene->collectedObjects.visibleObjects[j];
                    if (!visibleObjectOfType.empty())
                    {
                        auto* handler = scene->scene->proxyHandlers()[j];
                        handler->handleProxyFragments(cmd, *this, visibleObjectOfType.typedData(), visibleObjectOfType.size(), *scene->drawList);
                    }
                }
            }
        }

        //---

        void RenderLitView(command::CommandWriter& cmd, const FrameRenderer& frame, const FrameViewCamera& camera, const ImageView& resolvedColor, const ImageView& resolvedDepth)
        {
            PC_SCOPE_LVL1(RenderLitView);

            FrameView view(frame, camera, "LitView");
            command::CommandWriterBlock block(cmd, "LitView");

            // collect visible objects in the view
            view.collect();

            // TODO: start fiber with cascades
            // TODO: start fiber with point light shadows

            // prepare renderable fragments for the visible objects
            view.generateFragments(cmd);

            // bind the camera setup
            camera.bind(cmd);

            // get MSAA target surfaces
            const auto& tempColor = frame.fetchImage(FrameResource::HDRLinearMainColorRT);
            const auto& tempDepth = frame.fetchImage(FrameResource::HDRLinearMainDepthRT);

            // render passes
            RenderDepthPrepass(cmd, view, camera, tempDepth);
            RenderForwardPass(cmd, view, camera, tempDepth, tempColor);

            // capture depth as it's not going to change from now on (at least not legally...)
            ResolveMSAADepth(cmd, view, tempDepth, resolvedDepth);

            // render transparencies

            // resolve to targets
            ResolveMSAAColor(cmd, view, tempColor, resolvedColor);
        }

        //---

        void RenderWireframeView(command::CommandWriter& cmd, const FrameRenderer& frame, const FrameViewCamera& camera, const ImageView& resolvedColor, bool solid)
        {
            FrameView view(frame, camera, "WireframeView");
            command::CommandWriterBlock block(cmd, "WireframeView");

            // collect and prepare
            view.collect();
            view.generateFragments(cmd);

            // bind the camera setup
            camera.bind(cmd);

            // get MSAA target surfaces
            const auto& tempColor = frame.fetchImage(FrameResource::HDRLinearMainColorRT);
            const auto& tempDepth = frame.fetchImage(FrameResource::HDRLinearMainDepthRT);

            // render only the wire frame
            RenderWireframePass(cmd, view, camera, tempDepth, tempColor, solid);

            // resolve to targets
            ResolveMSAAColor(cmd, view, tempColor, resolvedColor);
        }

        //---

        void RenderDepthDebug(command::CommandWriter& cmd, const FrameRenderer& frame, const FrameViewCamera& camera, const ImageView& targetColor)
        {
            FrameView view(frame, camera, "DebugDepth");
            command::CommandWriterBlock block(cmd, "DebugDepth");

            // collect and prepare
            view.collect();
            view.generateFragments(cmd);

            // bind the camera setup
            camera.bind(cmd);

            // render depth pass only
            const auto& tempDepth = frame.fetchImage(FrameResource::HDRLinearMainDepthRT);
            RenderDepthPrepass(cmd, view, camera, tempDepth);

            // render the depth visualization
            VisualizeDepthBuffer(cmd, frame, tempDepth, targetColor);
        }

        //--

        void RenderLuminanceDebug(command::CommandWriter& cmd, const FrameRenderer& frame, const FrameViewCamera& camera, const ImageView& targetColor)
        {
            const auto& resolvedColor = frame.fetchImage(FrameResource::HDRResolvedColor);
            const auto& resolvedDepth = frame.fetchImage(FrameResource::HDRResolvedDepth);

            RenderLitView(cmd, frame, camera, resolvedColor, resolvedDepth);

            VisualizeLuminance(cmd, frame, resolvedColor, targetColor);
        }

        //--

    } // scene
} // rendering

