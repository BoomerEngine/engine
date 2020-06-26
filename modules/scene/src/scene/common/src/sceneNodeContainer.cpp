/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: content #]
*
***/

#include "build.h"
#include "scenePrefab.h"
#include "sceneNodeTemplate.h"
#include "sceneNodeContainer.h"

namespace scene
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

    int NodeTemplateContainer::addNode(const NodeTemplatePtr& node, bool clone /*= true*/, int parentNodeId /*= -1*/)
    {
        ASSERT(node);
        ASSERT(parentNodeId == -1 || parentNodeId <= m_nodes.lastValidIndex());

        // clone the node
        auto storedNode = node;
        if (clone)
        {
            storedNode = base::CloneObject<NodeTemplate>(node, sharedFromThis());
            if (!storedNode)
                return -1;
        }

        // make sure the node we are storing is parented to the container
        storedNode->parent(sharedFromThis());

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

        for (int i=m_nodes.lastValidIndex(); i >= 0; --i)
            if (!m_nodes[i].m_data)
                m_nodes.erase(i);

        for (uint32_t i = 0; i < m_nodes.size(); ++i)
        {
            auto& entry = m_nodes[i];

            // convert node class
            if (entry.m_data && entry.m_data->cls() != NodeTemplate::GetStaticClass())
                entry.m_data = entry.m_data->convertLegacyContent();

            // no node data, create empty one
            if (!entry.m_data)
            {
                entry.m_data = base::CreateSharedPtr<NodeTemplate>();
                entry.m_data->parent(sharedFromThis());
            }

            // repair broken parent indices
            if (entry.m_parentId != INDEX_NONE)
            {
                if (entry.m_parentId < 0 || entry.m_parentId >= (int)i)
                    entry.m_parentId = INDEX_NONE;
            }

            // add to root list or child list
            if (entry.m_parentId != INDEX_NONE)
            {
                auto& parentEntry = m_nodes[entry.m_parentId];
                parentEntry.m_children.pushBack(i);
            }
            else
            {
                m_rootNodes.pushBack(i);
            }
        }
    }

    //---

} // scene