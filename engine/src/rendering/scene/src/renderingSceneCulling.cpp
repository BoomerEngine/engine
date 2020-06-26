/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "renderingSceneCulling.h"

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

        void SceneObjectRegistry::updateObject(ObjectRenderID index, const base::Matrix& localToScene, const base::Box& sceneBounds)
        {
            // TODO
        }

        void SceneObjectRegistry::packObjectData(const SceneObjectInfo& info, GPUSceneObjectInfo& outObject) const
        {
            memset(&outObject, 0, sizeof(outObject));
            outObject.autoHideDistance = std::clamp<float>(info.autoHideDistance, 0.1f, 10000.0f);
            outObject.localToScene = info.localToScene;
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

            m_objectInfos.enumerate([&outResult, &setup](const SceneObjectInfo& info, uint32_t index)
                {
                    // TODO: test bounding box

                    auto& entry = outResult.visibleObjects[(int)info.proxyType].emplaceBack();
                    entry.cameraMask = 1;
                    entry.distance = 0;
                    entry.proxy = info.proxyPtr;
                    return false;
                });
        }

        //--

    } // scene
} // rendering