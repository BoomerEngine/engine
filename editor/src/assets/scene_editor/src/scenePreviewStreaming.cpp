/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#include "build.h"
#include "scenePreviewStreaming.h"
#include "sceneContentNodes.h"

#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingFrameParams.h"
#include "rendering/world/include/renderingVisualComponent.h"

#include "base/world/include/worldNodeTemplate.h"
#include "base/world/include/world.h"
#include "base/world/include/worldEntity.h"
#include "base/world/include/worldComponent.h"

namespace ed
{

    //--
     
    SceneNodeVisualizationHandler::SceneNodeVisualizationHandler(world::World* targetWorld)
        : m_world(targetWorld)
    {}

    SceneNodeVisualizationHandler::~SceneNodeVisualizationHandler()
    {}

    void SceneNodeVisualizationHandler::clearAllProxies()
    {
        const auto proxies = m_proxies;

        for (auto* proxy : proxies)
        {
            if (proxy)
            {
                if (proxy->entity)
                {
                    m_world->detachEntity(proxy->entity);
                    proxy->entity.reset();
                }

                m_proxyPool.free(proxy);
            }
        }

        m_freeProxyIndices.clear();
        m_proxies.clear();
    }

    static uint32_t ProxyIndicesAllocationBatchSize(uint32_t currentSize)
    {
        const auto roundedSize = NextPow2(currentSize);
        return std::clamp<uint32_t>(roundedSize, 64, 1024);
    }

    uint32_t SceneNodeVisualizationHandler::allocProxyIndex()
    {
        if (m_freeProxyIndices.empty())
        {
            auto firstIndex = m_proxies.size();
            auto allocationSize = ProxyIndicesAllocationBatchSize(firstIndex);
            m_proxies.allocateWith(allocationSize, nullptr);

            m_freeProxyIndices.reserve(allocationSize);
            for (uint32_t i = m_proxies.size(); i > firstIndex; --i)
                m_freeProxyIndices.pushBack(i-1);

            TRACE_INFO("Added visualization node slots {} -> {}", firstIndex, m_proxies.size());
        }

        auto index = m_freeProxyIndices.back();
        m_freeProxyIndices.popBack();
        return index;
    }

    void SceneNodeVisualizationHandler::createProxy(const SceneContentNode* node)
    {
        DEBUG_CHECK_RETURN(node);

        if (const auto* dataNode = rtti_cast<SceneContentEntityNode>(node))
        {
            auto lock = CreateLock(m_proxyLock);
            auto index = allocProxyIndex();

            auto* proxy = m_proxyPool.create();
            proxy->index = index;
            proxy->contentNodeObjectId = node->id();
            proxy->generation = m_proxyGenerationCounter++;
            proxy->placement = dataNode->cachedLocalToWorldTransform();
            proxy->version = 0;
            
            DEBUG_CHECK(!proxy->entity);

            DEBUG_CHECK(m_proxies[index] == nullptr);
            m_proxies[index] = proxy;
            m_nodeToProxyMap[node] = proxy;

            updateProxySelection(proxy, dataNode);
            updateProxyData(proxy, dataNode);
        }
    }

    void SceneNodeVisualizationHandler::removeProxy(const SceneContentNode* node)
    {
        auto lock = CreateLock(m_proxyLock);

        if (auto* proxy = m_nodeToProxyMap.findSafe(node, nullptr))
        {
            if (proxy->entity)
            {
                m_world->detachEntity(proxy->entity);
                proxy->entity.reset();
            }

            DEBUG_CHECK(m_proxies[proxy->index] == proxy);
            m_proxies[proxy->index] = nullptr;

            m_freeProxyIndices.pushBack(proxy->index);

            m_proxyPool.free(proxy);
        }
    }

    bool SceneNodeVisualizationHandler::CheckProxy(const RefWeakPtr<SceneNodeVisualizationHandler>& self, uint32_t proxyIndex, uint32_t proxyGeneration, uint32_t versionIndex)
    {
        if (auto handler = self.lock())
        {
            auto lock = CreateLock(handler->m_proxyLock);

            if (proxyIndex < handler->m_proxies.size())
            {
                if (auto* proxy = handler->m_proxies[proxyIndex])
                {
                    DEBUG_CHECK(proxy->index == proxyIndex);
                    if (proxy->generation == proxyGeneration)
                        if (proxy->version.load() == versionIndex)
                            return true;
                }
            }
        }

        return false;
    }

    static void ApplySelectionFlagToEntity(world::Entity* entity, bool flag, const Array<StringID>& namedComponents)
    {
        if (entity)
        {
            for (const auto& comp : entity->components())
                comp->toggleSelectionHighlight(flag || namedComponents.contains(comp->name()));
        }            
    }

    static void ApplySelectables(world::Entity* entity, uint32_t nodeObjectID)
    {
        if (entity)
        {
            for (const auto& comp : entity->components())
            {
                if (auto visualComp = base::rtti_cast<rendering::IVisualComponent>(comp.get()))
                {
                    const auto subObjectIndex = comp->name().index();
                    visualComp->bindSelectable(rendering::scene::Selectable(nodeObjectID, subObjectIndex));
                }
            }
        }
    }

    void SceneNodeVisualizationHandler::ApplyProxy(const RefWeakPtr<SceneNodeVisualizationHandler>& self, uint32_t proxyIndex, uint32_t proxyGeneration, uint32_t versionIndex, const world::EntityPtr& entity)
    {
        if (auto handler = self.lock())
        {
            auto lock = CreateLock(handler->m_proxyLock);

            if (proxyIndex < handler->m_proxies.size())
            {
                if (auto* proxy = handler->m_proxies[proxyIndex])
                {
                    DEBUG_CHECK(proxy->index == proxyIndex);
                    if (proxy->generation == proxyGeneration)
                    {
                        if (proxy->version.load() == versionIndex)
                        {
                            if (proxy->entity)
                                handler->m_world->detachEntity(proxy->entity);

                            proxy->entity = entity;

                            ApplySelectables(entity, proxy->contentNodeObjectId);
                            ApplySelectionFlagToEntity(entity, proxy->effectSelection, proxy->selectedComponents);

                            if (proxy->entity)
                            {
                                auto lock2 = CreateLock(handler->m_renderLock);
                                handler->m_world->attachEntity(proxy->entity);
                            }
                        }
                    }
                }
            }
        }
    }

    CAN_YIELD void SceneNodeVisualizationHandler::CompileEntityData(const RefWeakPtr<SceneNodeVisualizationHandler>& self, const world::NodeTemplatePtr& data, uint32_t versionIndex, uint32_t proxyIndex, uint32_t proxyGeneration)
    {
        DEBUG_CHECK_RETURN(data);

        // check if should even start
        if (CheckProxy(self, proxyIndex, proxyGeneration, versionIndex))
        {
            InplaceArray<const world::NodeTemplate*, 1> sourceTemplates;
            sourceTemplates.pushBack(data);

            // compile entity - this may load resources
            if (auto entity = world::CompileEntity(sourceTemplates))
            {
                ApplyProxy(self, proxyIndex, proxyGeneration, versionIndex, entity);
            }
        }
    }

    static bool MergeSelectionFlag(const SceneContentEntityNode* node)
    {
        while (node)
        {
            if (node->visualFlags().test(SceneContentNodeVisualBit::SelectedNode))
                return true;

            node = rtti_cast<SceneContentEntityNode>(node->parent());
        }

        return false;
    }
   
    SceneContentNodePtr SceneNodeVisualizationHandler::resolveSelectable(const rendering::scene::Selectable& selectable) const
    {
        if (auto object = IObject::FindUniqueObjectById(selectable.objectID()))
        {
            if (auto node = rtti_cast<SceneContentEntityNode>(object))
            {
                for (const auto& comp : node->components())
                {
                    const auto name = StringID::Find(comp->name().view());
                    if (name.index() == selectable.subObjectID())
                        return comp;
                }

                return node;
            }
        }

        return nullptr;
    }

    void SceneNodeVisualizationHandler::updateProxySelection(SceneNodeVisualization* proxy, const SceneContentEntityNode* node)
    {
        bool changed = false;

        bool useSelectionEffect = MergeSelectionFlag(node);
        if (useSelectionEffect != proxy->effectSelection)
        {
            proxy->effectSelection = useSelectionEffect;
            changed = true;
        }

        if (!proxy->selectedComponents.empty())
            changed = true;

        proxy->selectedComponents.reset();

        if (!proxy->effectSelection)
        {
            for (const auto& comp : node->components())
            {
                if (comp->visualFlags().test(SceneContentNodeVisualBit::SelectedNode))
                {
                    proxy->selectedComponents.pushBack(StringID(comp->name()));
                    changed = true;
                }
            }

            // TODO: better comparison of selected state
        }

        if (changed)
            ApplySelectionFlagToEntity(proxy->entity, useSelectionEffect, proxy->selectedComponents);

    }

    void SceneNodeVisualizationHandler::updateProxyData(SceneNodeVisualization* proxy, const SceneContentEntityNode* entityNode)
    {
        DEBUG_CHECK_RETURN(entityNode);

        if (const auto snapshot = entityNode->compileSnapshot())
        {
            auto versionIndex = ++proxy->version;
            auto proxyIndex = proxy->index;
            auto proxyGeneration = proxy->generation;

            auto weakSelfRef = RefWeakPtr<SceneNodeVisualizationHandler>(this);
            RunFiber("CreateEntity") << [weakSelfRef, snapshot, versionIndex, proxyIndex, proxyGeneration](FIBER_FUNC)
            {
                CompileEntityData(weakSelfRef, snapshot, versionIndex, proxyIndex, proxyGeneration);
            };
        }
    }

    void SceneNodeVisualizationHandler::updateProxy(const SceneContentNode* node, SceneContentNodeDirtyFlags& flags)
    {
        auto* proxy = m_nodeToProxyMap.findSafe(node, nullptr);

        if (const auto* dataNode = rtti_cast<SceneContentEntityNode>(node))
        {
            auto lock = CreateLock(m_proxyLock);

            if (flags.test(SceneContentNodeDirtyBit::Transform))
            {
                proxy->placement = dataNode->cachedLocalToWorldTransform();

                if (proxy->entity)
                    proxy->entity->requestTransform(proxy->placement);

                flags -= SceneContentNodeDirtyBit::Transform;
            }

            if (flags.test(SceneContentNodeDirtyBit::Content))
            {
                updateProxyData(proxy, dataNode);
                flags -= SceneContentNodeDirtyBit::Content;
            }

            if (flags.test(SceneContentNodeDirtyBit::Selection))
            {
                updateProxySelection(proxy, dataNode);
                flags -= SceneContentNodeDirtyBit::Selection;
            }
        }
    }

    //--
    
} // ed
