/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: content #]
***/

#pragma once

#include "base/resources/include/resource.h"
#include "base/containers/include/mutableArray.h"
#include "base/containers/include/hashSet.h"
#include "base/resources/include/objectTemplate.h"
#include "base/script/include/scriptObject.h"

#include "sceneNodePlacement.h"
#include "sceneNodeContainer.h"


namespace scene
{
    //--

    /// Streaming model for the node
    enum class StreamingModel : uint8_t
    {
        // Auto mode
        Auto,

        // Stream with parent node, this allows nodes to be grouped together
        // If there's no streamable parent the node is streamed on it's own using the streaming distane
        // The streaming distance of the parent node is calculated based on the maximum streaming distance of the child nodes
        StreamWithParent,

        // Node will be streamed separately based on it's own position and streaming distance
        // NOTE: this node will not be accessible as target of bindings (since we cannot guarantee it's loaded)
        HierarchicalGrid,

        // Node will be always loaded when the world is loaded
        // NOTE: this node will always be accessible for bindings
        AlwaysLoaded,

        // Node will be placed in a separate sector when baking
        // Good only for heavy data like navmesh
        SeparateSector,

        // Node will be discarded during world baking
        // Good for temporary data
        Discard,
    };

    //--

    /// actual component template data holder
    class SCENE_COMMON_API ComponentDataTemplate : public base::ObjectTemplate
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ComponentDataTemplate, base::ObjectTemplate);

    public:
        ComponentDataTemplate();

        // name of the component to create from this template
        INLINE base::StringID name() const { return m_name; }

        // is this template enabled ?
        INLINE bool isEnabled() const { return m_enabled; }

        //--

        virtual base::ClassType rootTemplateClass() const override final;
        virtual base::ClassType objectClass() const override final;

        //--

        ComponentPtr createComponent() const;

        void componentClass(base::SpecificClassType<Component> componentClass);
        void name(base::StringID name);

        static ComponentDataTemplatePtr Merge(const ComponentDataTemplate** templatesToMerge, uint32_t count);

    protected:
        bool m_enabled;
        base::StringID m_name;
        base::SpecificClassType<Component> m_componentClass;
    };

    //--

    /// actual entity template data holder
    class SCENE_COMMON_API EntityDataTemplate : public base::ObjectTemplate
    {
        RTTI_DECLARE_VIRTUAL_CLASS(EntityDataTemplate, base::ObjectTemplate);

    public:
        EntityDataTemplate();

        INLINE bool isEnabled() const { return m_enabled; }
        
        virtual base::ClassType rootTemplateClass() const override final;
        virtual base::ClassType objectClass() const override final;

        static EntityDataTemplatePtr Merge(const EntityDataTemplate** templatesToMerge, uint32_t count);

        void entityClass(base::SpecificClassType<Entity> entityClass);

        EntityPtr compile() const;

    protected:
        bool m_enabled;
        base::SpecificClassType<Entity> m_entityClass;
    };

    //--

    // a data for prefab in a node
    struct SCENE_COMMON_API NodeTemplatePrefabSetup
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(NodeTemplatePrefabSetup);

    public:
        bool m_enabled;
        PrefabRef m_prefab;
    };
   
    //--

    /// a basic template of a node, runtime nodes are spawned from templates
    /// node templates form a tree that is transformed once the nodes are instantiated
    class SCENE_COMMON_API NodeTemplate : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(NodeTemplate, base::IObject);

    public:
        NodeTemplate();
        virtual ~NodeTemplate();

        //--

        /// get node name
        INLINE base::StringID name() const { return m_name; }

        /// get placement of the node
        INLINE const NodeTemplatePlacement& placement() const { return m_placement; }

        /// get visibility flags
        INLINE StreamingModel streamingModel() const { return m_streamingModel; }

        /// get the override for streaming distance
        INLINE uint16_t streamingDistanceOverride() const { return m_streamingDistanceOverride; }

        /// prefabs to instance into the node
        INLINE const base::Array<NodeTemplatePrefabSetup>& prefabAssets() const { return m_prefabAssets; }

        /// get template for the entity
        INLINE const base::RefPtr<EntityDataTemplate>& entityTemplate() const { return m_entityTemplate; }

        /// get template for components
        INLINE const base::Array<base::RefPtr<ComponentDataTemplate>>& componentTemplates() const { return m_componentTemplates; }

        //--

        /// set name of the node
        void name(base::StringID name);

        /// set new placement of the node
        /// NOTE: this by definition does not affect the
        void placement(const NodeTemplatePlacement& placement);

        /// set new entity template data
        void entityTemplate(const EntityDataTemplatePtr& data);

        /// add component template data
        void addComponentTemplate(const ComponentDataTemplatePtr& data);

        /// remove component template data
        void removeComponentTemplate(const ComponentDataTemplatePtr& data);

        //--

        /// merge templates, creates a new template with merged data from all provided templates
        /// NOTE: original data objects are NOT modified, and a new object is always created
        static NodeTemplatePtr Merge(const NodeTemplate** templatesToMerge, uint32_t numTemplates);

        //--

        /// compile entity
        EntityPtr compile() const;

        //--

        /// LEGACY CONTENT CONVERSION
        virtual NodeTemplatePtr convertLegacyContent() const;


    private:
        // naming
        base::StringID m_name;

        // placement
        NodeTemplatePlacement m_placement;

        //--

        // streaming mode for this node
        StreamingModel m_streamingModel;

        // override for streaming distance
        float m_streamingDistanceOverride;

        //--

        // prefab to use
        base::Array<NodeTemplatePrefabSetup> m_prefabAssets;

        // entity template
        base::RefPtr<EntityDataTemplate> m_entityTemplate;

        // component templates
        base::Array<base::RefPtr<ComponentDataTemplate>> m_componentTemplates;

        //--

        // LEGACY placement, TO REMOVE
        base::Vector3 m_relativePosition;
        base::Angles m_relativeRotation;
        base::Vector3 m_relativeScale;

    protected:
        virtual void onPostLoad() override;
        virtual void onPropertyChanged(base::StringView<char> path) override;
    };

    //--

} // scene