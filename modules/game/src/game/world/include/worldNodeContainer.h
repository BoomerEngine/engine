/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/containers/include/hashSet.h"
#include "worldNodePlacement.h"

namespace game
{
    //--

    /// node parent/child relation
    struct GAME_WORLD_API NodeTemplateContainerHierarchyEntry 
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(NodeTemplateContainerHierarchyEntry);

    public:
        NodeTemplatePtr m_data;
        int m_parentId = INDEX_NONE;
        base::Array<int> m_children; // not saved, built at runtime
    };

    //--

    /// created entities
    struct GAME_WORLD_API NodeTemplateCreatedEntities
    {
        base::Array<EntityPtr> rootEntities; // root entities (entities that did not have a parent node in the container)
        base::Array<EntityPtr> allEntities; // all created entities, ordered parent first
    };

    //--

    /// "compiled" node template
    struct GAME_WORLD_API NodeTemplateCompiledData : public base::IReferencable
    {
    public:
        base::StringID name; // assigned name of the node (NOTE: may be different than name of the node in the prefab)
        base::Array<NodeTemplatePtr> templates; // all collected templates from all prefabs that matched the node's path, NOTE: NOT OWNED and NOT modified
        
        base::Transform localToParent; // placement of the actual node with respect to parent node, 99% of time matches the one source data
        base::Transform localToReference; // concatenated transform using the "scale-safe" transform rules, used during instantiation

        base::RefWeakPtr<NodeTemplateCompiledData> parent;
        base::Array<base::RefPtr<NodeTemplateCompiledData>> children; // collected children of this node

        //--

        NodeTemplateCompiledData();

        //--

        // compile an entity from the source data
        // NOTE: this function may load content
        CAN_YIELD EntityPtr createEntity(const base::AbsoluteTransform& parentTransform) const;

        //--
    };

    //--

    /// compilation result data holder
    struct GAME_WORLD_API NodeTemplateCompiledOutput : public base::IReferencable
    {
        //--

        base::Array<NodeTemplateCompiledDataPtr> roots; // root nodes of the compiled stuff
        base::Array<NodeTemplateCompiledDataPtr> allNodes; // all gathered nodes (for non-recursive inspection)

        base::HashSet<PrefabPtr> allUsedPrefabs; // all prefabs that were instantiated

        //--

        NodeTemplateCompiledOutput();

        //--

        /// create the single root entity from the collected set 
        // NOTE: this function may load content
        CAN_YIELD void createSingleRoot(int rootIndex, const NodePath& rootPath, const base::AbsoluteTransform& rootTransform, NodeTemplateCreatedEntities& outEntities) const;

        //-

    private:
        EntityPtr createEntity(const NodeTemplateCompiledData* nodeData, const NodePath& nodePath, const base::AbsoluteTransform& parentTransform, NodeTemplateCreatedEntities& outEntities) const;
    };

    //--

    /// container for nodes templates (used twice: in prefab and in layer)
    /// NOTE: add relations between nodes are managed by this container not by the node itself (similar idea to blocks in graph)
    class GAME_WORLD_API NodeTemplateContainer : public base::IObject
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
        int addNode(const NodeTemplatePtr& node, int parentNodeId = -1);

        //---

        /// Compile node template structure into something that is easier to instantiate
        /// This function resolves all prefabs and gathers a list of content for each node
        void compileNode(int nodeId, base::StringID nodeName, NodeTemplateCompiledOutput& outContainer, NodeTemplateCompiledData* compiledParent) const;

        // Compile all nodes in this node container, returns list a hierarchy of gathered nodes than can be than converted into actual entities
        void compile(NodeTemplateCompiledOutput& outContainer, NodeTemplateCompiledData* compiledParent) const;

        //---

    private:
        base::Array<NodeTemplateContainerHierarchyEntry> m_nodes;

        base::Array<int> m_rootNodes; // not saved
        uint32_t m_version; // data version

        virtual void onPostLoad() override final;
    };

} // game