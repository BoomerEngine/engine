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
#include "base/script/include/scriptObject.h"
#include "base/resource/include/objectDirectTemplate.h"

namespace base
{
    namespace world
    {   

        //--

        // a data for prefab in a node
        struct BASE_WORLD_API NodeTemplatePrefabSetup
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(NodeTemplatePrefabSetup);

            bool enabled = true;
            PrefabRef prefab;
            StringID appearance;

            NodeTemplatePrefabSetup();
            NodeTemplatePrefabSetup(const PrefabRef& prefab, bool enabled = true);
            NodeTemplatePrefabSetup(const PrefabPtr& prefab, bool enabled = true);
        };

        //--

        // a component for the entity
        struct BASE_WORLD_API NodeTemplateComponentEntry
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(NodeTemplateComponentEntry);

            StringID name;
            ObjectIndirectTemplatePtr data;

            NodeTemplateComponentEntry();
        };
   
        //--

        /// a basic template of a node, runtime nodes are spawned from templates
        /// node templates form a tree that is transformed once the nodes are instantiated
        class BASE_WORLD_API NodeTemplate : public IObject
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
            RTTI_DECLARE_VIRTUAL_CLASS(NodeTemplate, IObject);

        public:
            NodeTemplate();

            //--

            // name of the node, can't be empty
            StringID m_name; 

            // placement of the node (global)
            //Transform m_localToParent;

            // list of prefabs to instance AT THIS NODE (some may be disabled)
            Array<NodeTemplatePrefabSetup> m_prefabAssets;

            // entity data - can be empty if node did not carry any data
            ObjectIndirectTemplatePtr m_entityTemplate;

            // component data - can be empty if node does not have any components
            // NOTE: we may have component entries without entity entry
            Array<NodeTemplateComponentEntry> m_componentTemplates; 

            // child nodes
            Array<NodeTemplatePtr> m_children;

            //--

            /// find data for component
            const ObjectIndirectTemplate* findComponent(StringID name) const;

            /// find child by name
            const NodeTemplate* findChild(StringID name) const;

            //--

        protected:
            virtual void onPostLoad() override;
        };

        //--

        /// "compiled" node template
        struct BASE_WORLD_API NodeTemplateCompiledData : public IReferencable
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)

        public:
            StringID name; // assigned name of the node (NOTE: may be different than name of the node in the prefab)
            Array<NodeTemplatePtr> templates; // all collected templates from all prefabs that matched the node's path, NOTE: NOT OWNED and NOT modified

            Transform localToParent; // placement of the actual node with respect to parent node, 99% of time matches the one source data
            Transform localToReference; // concatenated transform using the "scale-safe" transform rules, used during instantiation

            RefWeakPtr<NodeTemplateCompiledData> parent;
            Array<RefPtr<NodeTemplateCompiledData>> children; // collected children of this node

            //--

            NodeTemplateCompiledData();

            //--

            // compile an entity from the source data
            // NOTE: this function may load content

            //--
        };

        //--

        // helper class used in node template compilation
        class BASE_WORLD_API NodeCompilationStack : public NoCopy
        {
        public:
            NodeCompilationStack();
            NodeCompilationStack(const NodeCompilationStack& other);

            //--

            // current template list
            INLINE const Array<const NodeTemplate*>& templates() { return m_templates; }

            //--

            // reset
            void clear();

            // push node template on stack
            void pushBack(const NodeTemplate* ptr);

            // collect list of child node names referenced in any of the template
            void collectChildNodeNames(HashSet<StringID>& outNodeNames) const;

            // collect list of active prefab references
            void collectPrefabs(Array<PrefabRef>& outPrefabs) const;

            // enter child of given name, returns new iterator
            void enterChild(StringID childNodeName, NodeCompilationStack& outIt) const;

            //--

        private:
            // note templates to compile, in order
            InplaceArray<const NodeTemplate*, 10> m_templates;
        };

        //--

        // compile flattened node (resolve and embed prefabs referenced in the node, NOTE: only prefabs from root node are embedded, use "Explode" to embed all prefabs)
        // NOTE: may return NULL if node contains NO DATA and could be discarded for all practical purposes
        BASE_WORLD_API NodeTemplatePtr UnpackTopLevelPrefabs(const NodeTemplate* rootNode);

        // compile flattened node (resolve and embed prefabs referenced in the node, NOTE: only prefabs from root node are embedded, use "Explode" to embed all prefabs)
        // NOTE: may return NULL if node contains NO DATA and could be discarded for all practical purposes
        BASE_WORLD_API NodeTemplatePtr CompileWithInjectedBaseNodes(const NodeTemplate* rootNode, const Array<const NodeTemplate*>& additionalBaseNodes);

        // compile an entity from list of templates, may return NULL if no entity data is present in any template
        // NOTE: it's not recursive
        BASE_WORLD_API CAN_YIELD EntityPtr CompileEntity(const Array<const NodeTemplate*>& templates, Transform* outEntityLocalTransform = nullptr);

        //--

        struct BASE_WORLD_API HierarchyEntity : public IReferencable
        {
            NodeGlobalID id;
            EntityPtr entity;
            
            bool streamingGroupChildren = true;
            bool streamingBreakFromGroup = false;
            float streamingDistanceOverride = 0.0f;

            Array<RefPtr<HierarchyEntity>> children;

            //--

            uint32_t countTotalEntities() const;
            void collectEntities(Array<EntityPtr>& outEntites) const; // TEMPSHIT
        };

        // compile entity (prefab-style), returns the root entity directly and other all entities via array
        BASE_WORLD_API CAN_YIELD RefPtr<HierarchyEntity> CompileEntityHierarchy(const NodePathBuilder& path, const NodeTemplate* rootTemplateNode, const AbsoluteTransform* forceInitialPlacement, res::IResourceLoader* loader);

        //--

    } // world
} // base