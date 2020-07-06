/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame  #]
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
#include "renderingFrameView_Cascades.h"

namespace rendering
{
    namespace scene
    {
        //---

        RTTI_BEGIN_TYPE_ENUM(FrameViewType);
            RTTI_ENUM_OPTION(MainColor);
            RTTI_ENUM_OPTION(GlobalCascades);
        RTTI_END_TYPE();

        RTTI_BEGIN_TYPE_ENUM(ProxyType);
            RTTI_ENUM_OPTION(None);
            RTTI_ENUM_OPTION(Mesh);
        RTTI_END_TYPE();

        RTTI_BEGIN_TYPE_ENUM(FragmentHandlerType);
            RTTI_ENUM_OPTION(None);
            RTTI_ENUM_OPTION(Mesh);
        RTTI_END_TYPE();
        
        //---

        RTTI_BEGIN_TYPE_ENUM(FragmentDrawBucket);
            RTTI_ENUM_OPTION(OpaqueNotMoving);
            RTTI_ENUM_OPTION(Opaque);
            RTTI_ENUM_OPTION(OpaqueMasked);
            RTTI_ENUM_OPTION(Transparent);
            RTTI_ENUM_OPTION(DebugSolid);
            RTTI_ENUM_OPTION(ShadowDepth0);
            RTTI_ENUM_OPTION(ShadowDepth1);
            RTTI_ENUM_OPTION(ShadowDepth2);
            RTTI_ENUM_OPTION(ShadowDepth3);
            RTTI_ENUM_OPTION(SelectionOutline);
        RTTI_END_TYPE();

        //---

        FrameView::FrameView(const FrameRenderer& frame, FrameViewType type, uint32_t width, uint32_t height)
            : m_renderer(frame)
            , m_frame(frame.m_frame)
            , m_allocator(frame.m_allocator.pageAllocator())
            , m_width(width)
            , m_height(height)
            , m_type(type)
        {
            const auto numScenes = m_renderer.m_scenes.size();
            m_scenes.reserve(numScenes);

            m_stats.numViews += 1;

            // create per scene data
            for (uint32_t i = 0; i < numScenes; ++i)
            {
                auto* data = m_allocator.createNoCleanup<PerSceneData>();
                data->frameStats = const_cast<SceneViewStats*>(&m_renderer.m_scenes[i].stats.views[(int)m_type]);
                data->stats.numViews = 1;
                data->scene = m_renderer.m_scenes[i].scene;
                data->params = m_renderer.m_scenes[i].params;
                data->drawList = m_allocator.createNoCleanup<FragmentDrawList>(m_allocator);
                m_scenes.pushBack(data);
            }
        }

        FrameView::~FrameView()
        {
            for (auto* scene : m_scenes)
            {
                if (scene->frameStats)
                    scene->frameStats->merge(scene->stats);

                scene->drawList->~FragmentDrawList();
                scene->~PerSceneData();
            }

            auto& viewStats = m_renderer.frameStats().views[(int)m_type];
            const_cast<FrameViewStats&>(viewStats).merge(m_stats);
        }

        void FrameView::collectSingleCamera(const Camera& camera)
        {
            PC_SCOPE_LVL1(CollectSingleCamera);
            for (auto* scene : m_scenes)
            {
                base::ScopeTimer timer;

                // TODO: transform camera to scene space

                SceneObjectCullingSetup sceneLocalCullingContext;
                sceneLocalCullingContext.cameraPosition = camera.position();
                sceneLocalCullingContext.stats = &scene->stats.culling;

                // single frustum
                VisibilityFrustum frustum;
                frustum.setup(camera);
                sceneLocalCullingContext.cameraFrustumCount = 1;
                sceneLocalCullingContext.cameraFrustums = &frustum;

                scene->scene->objects().cullObjects(sceneLocalCullingContext, scene->collectedObjects);

                scene->stats.cullingTime += timer.timeElapsed();
            }
        }

        void FrameView::collectCascadeCameras(const Camera& viewCamera, const CascadeData& cascades)
        {
            PC_SCOPE_LVL1(collectCascadeCameras);

            if (cascades.numCascades)
            {
                for (auto* scene : m_scenes)
                {
                    base::ScopeTimer timer;

                    // TODO: transform camera to scene space

                    SceneObjectCullingSetup sceneLocalCullingContext;
                    sceneLocalCullingContext.cameraPosition = viewCamera.position();
                    sceneLocalCullingContext.stats = &scene->stats.culling;

                    // single frustum
                    VisibilityFrustum frustums[MAX_CASCADES];
                    for (uint32_t i=0; i<cascades.numCascades; ++i)
                        frustums[i].setup(cascades.cascades[i].camera);

                    sceneLocalCullingContext.cameraFrustumCount = cascades.numCascades;
                    sceneLocalCullingContext.cameraFrustums = frustums;
                    scene->scene->objects().cullObjects(sceneLocalCullingContext, scene->collectedObjects);

                    scene->stats.cullingTime += timer.timeElapsed();
                }
            }
        }

        void FrameView::generateFragments(command::CommandWriter& cmd)
        {
            PC_SCOPE_LVL1(GenerateFragments);

            for (auto* scene : m_scenes)
            {
                base::ScopeTimer timer;

                for (uint32_t j = 1; j < (uint32_t)ProxyType::MAX; ++j)
                {
                    const auto& visibleObjectOfType = scene->collectedObjects.visibleObjects[j];
                    if (!visibleObjectOfType.empty())
                    {
                        auto* handler = scene->scene->proxyHandlers()[j];
                        handler->handleProxyFragments(cmd, *this, visibleObjectOfType.typedData(), visibleObjectOfType.size(), *scene->drawList);
                    }
                }

                for (uint32_t j = 1; j < ARRAY_COUNT(scene->stats.buckets); ++j)
                    scene->stats.buckets[j].merge(scene->drawList->stats((FragmentDrawBucket)j));

                scene->stats.fragmentsTime += timer.timeElapsed();
            }
        }

        //--

        struct GPUShadowsInfo
        {
            GPUCascadeInfo cascades;
        };

        struct ShadowParams
        {
            ConstantsView Constants;
            ImageView CascadeShadowMap;
        };


        void BindShadowsData(command::CommandWriter& cmd, const CascadeData& cascades)
        {
            GPUShadowsInfo packedData;
            PackCascadeData(cascades, packedData.cascades);

            ShadowParams params;
            params.Constants = cmd.opUploadConstants(packedData);
            params.CascadeShadowMap = cascades.cascadeShadowMap.createSampledView(ObjectID::DefaultDepthBilinearSampler());

            cmd.opBindParametersInline("ShadowParams"_id, params);
        }

        //--

        /// lighting
#pragma pack(push)
#pragma pack(4)
        struct GPUGlobalLightingParams
        {
            base::Vector4 LightDirection; // normal vector towards the global light
            base::Vector4 LightColor; // color of the global light, LINEAR
            base::Vector4 AmbientColorZenith; // color of the global ambient light, LINEAR
            base::Vector4 AmbientColorHorizon; // color of the global light, LINEAR
        };

        struct GPUightingParams
        {
            GPUGlobalLightingParams GlobalLighting;
        };
#pragma pack(pop)

        struct LightingParams
        {
            ConstantsView Constants;
            ImageView GlobalShadowMaskAO;
        };


        void BindLightingData(command::CommandWriter& cmd, const LightingData& lighting)
        {
            GPUightingParams params;
            params.GlobalLighting.LightColor = lighting.globalLighting.globalLightColor.xyzw();
            params.GlobalLighting.LightDirection = lighting.globalLighting.globalLightDirection.xyzw();
            params.GlobalLighting.AmbientColorHorizon = lighting.globalLighting.globalAmbientColorHorizon.xyzw();
            params.GlobalLighting.AmbientColorZenith = lighting.globalLighting.globalAmbientColorZenith.xyzw();

            LightingParams desc;
            desc.Constants = cmd.opUploadConstants(params);
            desc.GlobalShadowMaskAO = lighting.globalShadowMaskAO;

            cmd.opBindParametersInline("LightingParams"_id, desc);
        }

        //--

    } // scene
} // rendering

