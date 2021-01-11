/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\structure #]
***/

#include "build.h"
#include "sceneContentStructure.h"
#include "sceneContentNodes.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneContentStructure);
    RTTI_END_TYPE();

    SceneContentStructure::SceneContentStructure(SceneContentNodeType rootNodeType)
    { 
        if (rootNodeType == SceneContentNodeType::PrefabRoot)
            m_root = RefNew<SceneContentPrefabRoot>();
        else if (rootNodeType == SceneContentNodeType::WorldRoot)
            m_root = RefNew<SceneContentWorldRoot>();

        m_root->bindRootStructure(this);
    }

    SceneContentStructure::~SceneContentStructure()
    {}

    void SceneContentStructure::handleDebugRender(rendering::scene::FrameParams& frame) const
    {
        for (const auto& ent : m_nodes)
            ent->handleDebugRender(frame);
    }

    void SceneContentStructure::nodeAdded(SceneContentNode* node)
    {
        ASSERT(node);
        ASSERT(node->structure() == this);
        DEBUG_CHECK_RETURN(!m_nodes.contains(node));

        m_nodes.pushBack(AddRef(node));

        auto& sync = m_nodeChanges.emplaceBack();
        sync.type = SceneContentSyncOpType::Added;
        sync.node = AddRef(node);        

        if (auto entity = rtti_cast<SceneContentEntityNode>(node))
            m_entities.pushBack(AddRef(entity));

        if (!node->dirtyFlags().empty())
            m_dirtyNodes.insert(node);
    }

    void SceneContentStructure::nodeRemoved(SceneContentNode* node)
    {
        ASSERT(node);
        ASSERT(node->structure() == this);
        DEBUG_CHECK_RETURN(m_nodes.contains(node));

        auto& sync = m_nodeChanges.emplaceBack();
        sync.type = SceneContentSyncOpType::Removed;
        sync.node = AddRef(node);

        m_nodes.remove(node);        
        m_dirtyNodes.remove(node);

        if (auto entity = rtti_cast<SceneContentEntityNode>(node))
            m_entities.remove(entity);
    }

    void SceneContentStructure::nodeDirtyContent(SceneContentNode* node)
    {
        DEBUG_CHECK_RETURN(node);
        m_dirtyNodes.insert(node);
    }

    void SceneContentStructure::syncNodeChanges(Array<SceneContentSyncOp>& outOps)
    {
        outOps = std::move(m_nodeChanges);
    }

    void SceneContentStructure::visitDirtyNodes(const std::function<void(const SceneContentNode*, SceneContentNodeDirtyFlags&)>& func)
    {
        PC_SCOPE_LVL0(VisitDirtyNodes);

        uint32_t numNodesVisited = 0;
        uint32_t numNodesStillDirty = 0;

        auto nodes = m_dirtyNodes.keys();
        m_dirtyNodes.reset();

        for (const auto& nodeRef : nodes)
        {
            if (auto node = nodeRef.lock())
            {
                auto flags = node->dirtyFlags();
                func(node, flags);
                numNodesVisited += 1;

                if (flags != node->dirtyFlags())
                {
                    node->resetDirtyStatus(flags);

                    if (!flags.empty())
                    {
                        m_dirtyNodes.insert(nodeRef);
                        numNodesStillDirty += 1;
                    }
                }
            }
        }

        if (numNodesVisited)
        {
            TRACE_INFO("Visited {} dirty nodes ({} still dirty)", numNodesVisited, numNodesStillDirty);
        }
    }

    //---

} // ed