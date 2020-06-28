/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

#include "base/resources/include/resource.h"
#include "base/containers/include/mutableArray.h"
#include "base/containers/include/hashSet.h"

namespace game
{
    //--

    /// node parent/child relation
    struct GAME_SCENE_API NodeTemplateContainerHierarchyEntry 
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(NodeTemplateContainerHierarchyEntry);

    public:
        NodeTemplatePtr m_data;
        int m_parentId = INDEX_NONE;
        base::Array<int> m_children; // not saved, built at runtime
    };

    /// container for nodes templates (used twice: in prefab and in layer)
    /// NOTE: add relations between nodes are managed by this container not by the node itself (similar idea to blocks in graph)
    class GAME_SCENE_API NodeTemplateContainer : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(NodeTemplateContainer, base::IObject);

    public:
        NodeTemplateContainer();

        //---

        // get all nodes in the container
        INLINE const base::Array<NodeTemplateContainerHierarchyEntry>& nodes() const { return m_nodes; }

        // get root nodes in the container
        INLINE const base::Array<int>& rootNodes() const { return m_rootNodes; }

        //---

        /// add node to container, optionally to a parent, returns ID of the node
        int addNode(const NodeTemplatePtr& node, bool clone=true, int parentNodeId = -1);

        //---

        /// compile node template structure
        void compile(int nodeId, const base::AbsoluteTransform& placement, const NodeTemplateContainerPtr& outContainer, int outContainerParentIndex, PrefabDependencies* outDependencies = nullptr) const;

    private:
        base::Array<NodeTemplateContainerHierarchyEntry> m_nodes;

        base::Array<int> m_rootNodes; // not saved
        uint32_t m_version; // data version

        virtual void onPostLoad() override final;
    };

} // game