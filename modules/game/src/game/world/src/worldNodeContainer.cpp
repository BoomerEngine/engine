/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
*
***/

#include "build.h"
#include "worldPrefab.h"
#include "worldNodeTemplate.h"
#include "worldNodeContainer.h"

namespace game
{

    //---

    RTTI_BEGIN_TYPE_STRUCT(NodeTemplateContainerHierarchyEntry);
        RTTI_PROPERTY(m_data);
        RTTI_PROPERTY(m_parentId);
    RTTI_END_TYPE();

    //---

    RTTI_BEGIN_TYPE_CLASS(NodeTemplateContainer);
        RTTI_PROPERTY(m_nodes);
    RTTI_END_TYPE();

    NodeTemplateContainer::NodeTemplateContainer()
    {}

    int NodeTemplateContainer::addNode(const NodeTemplatePtr& node, int parentNodeId /*= -1*/)
    {
        ASSERT(node);
        ASSERT(parentNodeId == -1 || parentNodeId <= m_nodes.lastValidIndex());

        // make sure the node we are storing is parented to the container
        DEBUG_CHECK_EX(node->parent() == nullptr, "Node is parented to something, risky buisiness");
        auto storedNode = node;
        storedNode->parent(this);

        // add entry
        auto& entry = m_nodes.emplaceBack();
        entry.m_data = storedNode;
        entry.m_parentId = parentNodeId;

        // add to root node list
        auto entryId = m_nodes.lastValidIndex();
        if (parentNodeId == INDEX_NONE)
        {
            m_rootNodes.pushBack(entryId);
        }
        else
        {
            auto& parentEntry = m_nodes[parentNodeId];
            parentEntry.m_children.pushBack(entryId);
        }

        return entryId;
    }
    
    void NodeTemplateContainer::onPostLoad()
    {
        TBaseClass::onPostLoad();

        // good place for optional cleanup/conversion of nodes, etc
    }

    //---

} // game