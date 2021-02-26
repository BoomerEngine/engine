/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

#include "core/memory/include/structurePool.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

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
    EntityPtr entity;

    bool visible = true;
    bool effectSelection = false;
};

//--

/// visualization handler for scene
class SceneNodeVisualizationHandler : public IReferencable
{
public:
    SceneNodeVisualizationHandler(World* targetWorld);
    ~SceneNodeVisualizationHandler();

    void clearAllProxies();
    void updateAllProxies();

    void createProxy(const SceneContentNode* node);
    void removeProxy(const SceneContentNode* node);
    void updateProxy(const SceneContentNode* node, SceneContentNodeDirtyFlags& flags);

    bool retrieveBoundsForProxy(const SceneContentNode* node, Box& outBounds) const;

    SceneContentNodePtr resolveSelectable(const Selectable& selectable) const;

private:
    World* m_world;

    mem::StructurePool<SceneNodeVisualization> m_proxyPool;

    Array<SceneNodeVisualization*> m_proxies;
    Array<uint32_t> m_freeProxyIndices;

    Mutex m_proxyLock;
    Mutex m_renderLock;

    uint32_t m_proxyGenerationCounter = 1;

    HashMap<const SceneContentNode*, SceneNodeVisualization*> m_nodeToProxyMap;

    struct ProxyToReattach
    {
        uint32_t index;
        uint32_t generation;
        EntityPtr newEntity;
    };

    Array<ProxyToReattach> m_reattachList;

    uint32_t allocProxyIndex();

    void updateProxySelection(SceneNodeVisualization* proxy, const SceneContentEntityNode* node);
    void updateProxyData(SceneNodeVisualization* proxy, const SceneContentEntityNode* node);
    void updateProxyVisibility(SceneNodeVisualization* proxy, const SceneContentEntityNode* node);
    void reattachProxies();

    static bool CheckProxy(const RefWeakPtr<SceneNodeVisualizationHandler>& self, uint32_t proxyIndex, uint32_t proxyGeneration, uint32_t versionIndex);
    static void ApplyProxy(const RefWeakPtr<SceneNodeVisualizationHandler>& self, uint32_t proxyIndex, uint32_t proxyGeneration, uint32_t versionIndex, const EntityPtr& entity);
    static CAN_YIELD void CompileEntityData(const RefWeakPtr<SceneNodeVisualizationHandler>& self, const NodeTemplatePtr& data, uint32_t versionIndex, uint32_t proxyIndex, uint32_t proxyGeneration);
};

//--

END_BOOMER_NAMESPACE_EX(ed)
