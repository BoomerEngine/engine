/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

#include "rendering/driver/include/renderingManagedBuffer.h"
#include "base/containers/include/staticStructurePool.h"

namespace rendering
{
    namespace scene
    {

        ///--

        struct SceneObjectInfo
        {
            base::Matrix localToScene;
            base::Box sceneBounds;
            uint16_t flags = 0;
            base::Color color = base::Color::WHITE;
            ProxyType proxyType = ProxyType::None;
            const IProxy* proxyPtr = nullptr;
            float autoHideDistance = 100.0f;
        };

#pragma pack(push)
#pragma pack(4)
        struct GPUSceneObjectInfo
        {
            float autoHideDistance = 0.0f; // 0 if element is not used
            uint32_t flags = 0;
            uint32_t color = 0;
            uint32_t colorEx = 0;
            base::Vector4 sceneBoundsMin; // unpacked bounds, W can be used for something
            base::Vector4 sceneBoundsMax; // unpacked bounds, W can be used for something
            base::Matrix localToScene; // transformation matrix
        };
#pragma pack(pop)

        ///--

        struct SceneObjectCullingSetup : public base::NoCopy
        {
            base::Vector3 cameraPosition;
            base::Matrix cameraFrustumMatrix;
        };

        struct SceneObjectCullingEntry
        {
            const IProxy* proxy = nullptr;
            uint8_t cameraMask = 0;
            uint16_t distance = 0;
        };

        struct SceneObjectCullingResult : public base::NoCopy
        {
            base::Array<SceneObjectCullingEntry> visibleObjects[(int)ProxyType::MAX];
        };

        ///--

        /// scene global object's registry
        class RENDERING_SCENE_API SceneObjectRegistry : public base::NoCopy
        {
        public:
            SceneObjectRegistry(Scene* owner, uint32_t maxObjects);
            virtual ~SceneObjectRegistry();

            // get object information
            INLINE const SceneObjectInfo* objectInfos() const { return m_objectInfos.typedData(); }

            // get object information
            INLINE const SceneObjectInfo& objectInfo(uint32_t index) const { return m_objectInfos.typedData()[index]; }

            //--

            // register an object
            bool registerObject(const SceneObjectInfo& info, ObjectRenderID& outIndex);

            // unregister an object
            void unregisterObject(ObjectRenderID index);

            // update object placement/bounds
            void updateObject(ObjectRenderID index, const base::Matrix& localToScene, const base::Box& sceneBounds);

            //--

            // prepare for rendering, update all data on the GPU side
            void prepareForFrame(command::CommandWriter& cmd);

            //--

            // cull objects
            void cullObjects(const SceneObjectCullingSetup& setup, SceneObjectCullingResult& outResult) const;

        private:
            static const uint32_t MAX_OBJECTS_ABSOLUTE_LIMIT = 1024*1024;

            Scene* m_scene;

            //--

            uint32_t m_maxObjects = 0;

            base::UniquePtr<ManagedBuffer> m_gpuObjectInfos;

            base::StaticStructurePool<SceneObjectInfo> m_objectInfos;

            //--
            
            void packObjectData(const SceneObjectInfo& info, GPUSceneObjectInfo& outObject) const;

            //--
        };

        ///--

    } // scene
} // rendering

