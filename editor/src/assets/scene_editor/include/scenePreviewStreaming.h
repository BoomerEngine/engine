/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

#include "base/memory/include/structurePool.h"

namespace ed
{
    //--

    // visualization of node
    class SceneNodeVisualization : public NoCopy
    {
    public:
        uint32_t index = INDEX_MAX;
        uint32_t generation = INDEX_MAX;

        ObjectID contentNodeObjectId = 0;

        AbsoluteTransform placement;

        std::atomic<uint32_t> version = 0;
        world::EntityPtr entity;

        bool effectSelection = false;
        Array<StringID> selectedComponents;
    };

    //--

    /// visualization handler for scene
    class SceneNodeVisualizationHandler : public IReferencable
    {
    public:
        SceneNodeVisualizationHandler(world::World* targetWorld);
        ~SceneNodeVisualizationHandler();

        void clearAllProxies();

        void createProxy(const SceneContentNode* node);
        void removeProxy(const SceneContentNode* node);
        void updateProxy(const SceneContentNode* node, SceneContentNodeDirtyFlags& flags);

        SceneContentNodePtr resolveSelectable(const rendering::scene::Selectable& selectable) const;

    private:
        world::World* m_world;

        mem::StructurePool<SceneNodeVisualization> m_proxyPool;

        Array<SceneNodeVisualization*> m_proxies;
        Array<uint32_t> m_freeProxyIndices;

        Mutex m_proxyLock;
        Mutex m_renderLock;

        uint32_t m_proxyGenerationCounter = 1;

        HashMap<const SceneContentNode*, SceneNodeVisualization*> m_nodeToProxyMap;

        uint32_t allocProxyIndex();

        void updateProxySelection(SceneNodeVisualization* proxy, const SceneContentEntityNode* node);
        void updateProxyData(SceneNodeVisualization* proxy, const SceneContentEntityNode* node);

        static bool CheckProxy(const RefWeakPtr<SceneNodeVisualizationHandler>& self, uint32_t proxyIndex, uint32_t proxyGeneration, uint32_t versionIndex);
        static void ApplyProxy(const RefWeakPtr<SceneNodeVisualizationHandler>& self, uint32_t proxyIndex, uint32_t proxyGeneration, uint32_t versionIndex, const world::EntityPtr& entity);
        static CAN_YIELD void CompileEntityData(const RefWeakPtr<SceneNodeVisualizationHandler>& self, const world::NodeTemplatePtr& data, uint32_t versionIndex, uint32_t proxyIndex, uint32_t proxyGeneration);
    };

    //--

} // ed