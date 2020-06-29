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

        RTTI_BEGIN_TYPE_ENUM(FragmentDrawBucket);
        RTTI_ENUM_OPTION(OpaqueNotMoving);
        RTTI_ENUM_OPTION(Opaque);
        RTTI_ENUM_OPTION(OpaqueMasked);
        RTTI_ENUM_OPTION(Transparent);
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

            // create per scene data
            for (uint32_t i = 0; i < numScenes; ++i)
            {
                auto* data = m_allocator.createNoCleanup<PerSceneData>();
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
                scene->drawList->~FragmentDrawList();
                scene->~PerSceneData();
            }
        }

        void FrameView::collectSingleCamera(const Camera& camera)
        {
            PC_SCOPE_LVL1(CollectSingleCamera);
            for (auto* scene : m_scenes)
            {
                // TODO: transform to scene space
                SceneObjectCullingSetup sceneLocalCullingContext;
                sceneLocalCullingContext.cameraPosition = camera.position();
                //sceneLocalCullingContext.cameraFrustumMatrix = 
                scene->scene->objects().cullObjects(sceneLocalCullingContext, scene->collectedObjects);
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

        //--

        struct GPULightingInfo
        {
            GPUCascadeInfo cascades;
        };

        struct LightingParams
        {
            ConstantsView Constants;
            ImageView CascadeShadowMap;
        };


        void BindLightingData(command::CommandWriter& cmd, const CascadeData& cascades)
        {
            GPULightingInfo packedData;
            PackCascadeData(cascades, packedData.cascades);

            LightingParams params;
            params.Constants = cmd.opUploadConstants(packedData);
            params.CascadeShadowMap = cascades.cascadeShadowMap;

            cmd.opBindParametersInline("LightingParams"_id, params);
        }

        //--

    } // scene
} // rendering

