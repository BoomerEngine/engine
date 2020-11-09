/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#pragma once

#include "renderingScene.h"
#include "renderingSceneProxy.h"

#include "base/system/include/simpleStructurePool.h"
#include "rendering/material/include/renderingMaterialRuntimeService.h"

namespace rendering
{
    namespace scene
    {

        ///--

        // data for a mesh proxy
        struct ProxyMesh : public IProxy
        {
            ProxyMesh();

            struct Chunk
            {
                MeshChunkRenderID meshChunkId = 0;
                uint8_t detailMask = 1;
                uint16_t materialIndex = 0;
                MaterialCachedTemplatePtr materialTemplate = nullptr;
                MaterialDataProxyPtr materialData = nullptr;
            };

            base::Array<Chunk> chunks;
            base::Box localBounds;
            bool castsShadows = true;
            bool selected = false;
        };

        ///--

        /// proxy handler for mesh proxies
        class RENDERING_SCENE_API ProxyMeshHandler : public IProxyHandler, public IMaterialDataProxyListener
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ProxyMeshHandler, IProxyHandler);

        public:
            ProxyMeshHandler();
            virtual ~ProxyMeshHandler();

            virtual void handleInit(Scene* scene) override final;
            virtual void handleSceneLock() override final;
            virtual void handleSceneUnlock() override final;
            virtual void handlePrepare(command::CommandWriter& cmd) override final;

            virtual IProxy* handleProxyCreate(const ProxyBaseDesc& desc) override final;
            virtual void handleProxyDestroy(IProxy* proxy) override final;

            virtual void handleProxyFragments(command::CommandWriter& cmd, FrameView& view, const SceneObjectCullingEntry* proxies, uint32_t numProxies, FragmentDrawList& outFragmentList) const override;

        private:
            base::SimpleStructurePool<ProxyMesh> m_pool;
            base::HashSet<ProxyMesh*> m_allProxies;

            MaterialCache* m_materialCache = nullptr;

            //--

            virtual void runMoveProxy(Scene* scene, IProxy* proxy, const CommandMoveProxy& cmd) override final;
            virtual void runEffectSelectionHighlight(Scene* scene, IProxy* proxy, const CommandEffectSelectionHighlight& cmd) override final;

            virtual void handleMaterialProxyChanges(const MaterialDataProxyChangesRegistry& changedProxies) override final;
        };

        ///---
        
    } // scene
} // rendering

