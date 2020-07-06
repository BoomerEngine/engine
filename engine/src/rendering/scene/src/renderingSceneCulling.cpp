/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "renderingSceneCulling.h"
#include "renderingSceneStats.h"

namespace rendering
{
    namespace scene
    {
        //---

        SceneObjectRegistry::SceneObjectRegistry(Scene* owner, uint32_t maxObjects)
            : m_objectInfos(POOL_TEMP)
            , m_maxObjects(std::min<uint32_t>(maxObjects, MAX_OBJECTS_ABSOLUTE_LIMIT))
        {
            m_objectInfos.resize(m_maxObjects);

            {
                auto index = m_objectInfos.emplace();
                DEBUG_CHECK(index == 0);
            }

            {
                BufferCreationInfo info;
                info.allowCopies = true;
                info.allowDynamicUpdate = true;
                info.allowShaderReads = true;
                info.allowUAV = true;
                info.label = "SceneObjectRegistry";
                info.stride = sizeof(GPUSceneObjectInfo);
                info.size = info.stride * m_maxObjects;
                m_gpuObjectInfos.create(info);
            }
        }

        SceneObjectRegistry::~SceneObjectRegistry()
        {
            m_objectInfos.clear();
            m_gpuObjectInfos.reset();
        }

        //--

        bool SceneObjectRegistry::registerObject(const SceneObjectInfo& info, ObjectRenderID& outIndex)
        {
            if (m_objectInfos.full())
                return false;

            auto index = m_objectInfos.emplace(info);
            outIndex = index;

            auto& data = m_objectInfos.typedData()[index];
            data.visibilityBox.setup(data.sceneBounds);

            GPUSceneObjectInfo gpuInfo;
            packObjectData(info, gpuInfo);
            TRACE_INFO("Object++: {}, {} total", index, m_objectInfos.occupancy());

            m_gpuObjectInfos->writeAtIndex(index, gpuInfo);
            return true;
        }

        void SceneObjectRegistry::unregisterObject(ObjectRenderID index)
        {
            m_objectInfos.free(index);

            TRACE_INFO("Object--: {}, {} total", index, m_objectInfos.occupancy());

            GPUSceneObjectInfo gpuInfo;
            memset(&gpuInfo, 0, sizeof(gpuInfo));
            m_gpuObjectInfos->writeAtIndex(index, gpuInfo);
        }

        void SceneObjectRegistry::repackObject(ObjectRenderID index)
        {
            const auto& data = m_objectInfos.typedData()[index];

            GPUSceneObjectInfo gpuInfo;
            memset(&gpuInfo, 0, sizeof(gpuInfo));
            packObjectData(data, gpuInfo);

            m_gpuObjectInfos->writeAtIndex(index, gpuInfo);
        }

        void SceneObjectRegistry::updateObject(ObjectRenderID index, const base::Matrix& localToScene, const base::Box& sceneBounds)
        {
            auto& data = m_objectInfos.typedData()[index];
            data.localToScene = localToScene;
            data.sceneBounds = sceneBounds;
            data.visibilityBox.setup(sceneBounds);

            repackObject(index);            
        }

        void SceneObjectRegistry::packObjectData(const SceneObjectInfo& info, GPUSceneObjectInfo& outObject) const
        {
            memset(&outObject, 0, sizeof(outObject));
            outObject.autoHideDistance = std::clamp<float>(info.autoHideDistance, 0.1f, 10000.0f);
            outObject.localToScene = info.localToScene.transposed();
            outObject.sceneBoundsMin = info.sceneBounds.min;
            outObject.sceneBoundsMax = info.sceneBounds.max;
            outObject.flags = info.flags;
            outObject.color = info.color.toRGBA();
            outObject.colorEx = 0xFFFFFFFF;
        }

        //--

        void SceneObjectRegistry::prepareForFrame(command::CommandWriter& cmd)
        {
            // send updates to object table
            m_gpuObjectInfos->update(cmd);

            // bind the buffers
            {
                BufferView view = m_gpuObjectInfos->bufferView();
                cmd.opBindParametersInline("ObjectBuffers"_id, view);
            }

            // TODO: compat vis table ?
        }

        //--

        void SceneObjectRegistry::cullObjects(const SceneObjectCullingSetup& setup, SceneObjectCullingResult& outResult) const
        {
            PC_SCOPE_LVL1(CullObjects);

            SceneCullingStats stats;

            m_objectInfos.enumerate([&outResult, &stats, &setup](const SceneObjectInfo& info, uint32_t index)
                {
                    stats.proxies[(int)info.proxyType].numTestedProxies += 1;

                    uint8_t furstumMask = 0;
                    for (uint32_t i = 0; i < setup.cameraFrustumCount; ++i)
                        if (info.visibilityBox.isInFrustum(setup.cameraFrustums[i]))
                            furstumMask |= (1 << i);

                    if (furstumMask)
                    {
                        stats.proxies[(int)info.proxyType].numCollectedProxies += 1;

                        auto& entry = outResult.visibleObjects[(int)info.proxyType].emplaceBack();
                        entry.frustumMask = furstumMask;
                        entry.distance = 0;
                        entry.proxy = info.proxyPtr;
                    }

                    return false; // continue iterating
                });


            if (setup.stats)
                setup.stats->merge(stats);
        }

        //--

    } // scene
} // rendering