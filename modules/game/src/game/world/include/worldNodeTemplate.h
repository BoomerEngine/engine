/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\content #]
***/

#pragma once

#include "worldNodePlacement.h"

#include "base/resource/include/resource.h"
#include "base/containers/include/mutableArray.h"
#include "base/containers/include/hashSet.h"
#include "base/script/include/scriptObject.h"
#include "base/object/include/objectTemplate.h"

namespace game
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

    // a data for prefab in a node
    struct GAME_WORLD_API NodeTemplatePrefabSetup
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(NodeTemplatePrefabSetup);

        bool enabled = true;
        PrefabRef prefab;

        NodeTemplatePrefabSetup();
        NodeTemplatePrefabSetup(const PrefabRef& prefab, bool enabled = true);
        NodeTemplatePrefabSetup(const PrefabPtr& prefab, bool enabled = true);
    };

    //--

    // a component for the entity
    struct GAME_WORLD_API NodeTemplateComponentEntry
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(NodeTemplateComponentEntry);

        base::StringID name;
        ComponentTemplatePtr data;

        NodeTemplateComponentEntry();
        NodeTemplateComponentEntry(base::StringID name, ComponentTemplate* data);
    };

    //--

    // general node type
    enum class NodeTemplateType : uint8_t
    {
        Content, // genuine content node,
        Override, // override node - usually overrides stuff created in a prefab
    };

    //--

    /// node template construction info
    struct GAME_WORLD_API NodeTemplateConstructionInfo
    {
        base::StringID name;
        NodeTemplateType type = NodeTemplateType::Content;
        NodeTemplatePlacement placement;

        StreamingModel streamingModel = StreamingModel::Auto;
        float streamingDistanceOverride = 0.0f;

        base::Array<NodeTemplatePrefabSetup> prefabAssets; // prefab to use

        EntityTemplatePtr entityData;
        base::Array<NodeTemplateComponentEntry> componentData;

        //--

        NodeTemplateConstructionInfo();
    };
   
    //--

    /// a basic template of a node, runtime nodes are spawned from templates
    /// node templates form a tree that is transformed once the nodes are instantiated
    class GAME_WORLD_API NodeTemplate : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(NodeTemplate, base::IObject);

    public:
        NodeTemplate();
        NodeTemplate(const NodeTemplateConstructionInfo& data);
        virtual ~NodeTemplate();

        //--

        /// get node name
        INLINE base::StringID name() const { return m_name; }

        /// get type of the node
        INLINE NodeTemplateType type() const { return m_type; }

        /// get placement of the node
        INLINE const NodeTemplatePlacement& placement() const { return m_placement; }

        /// get visibility flags
        INLINE StreamingModel streamingModel() const { return m_streamingModel; }

        /// get the override for streaming distance
        INLINE uint16_t streamingDistanceOverride() const { return m_streamingDistanceOverride; }

        /// prefabs to instance into the node
        INLINE const base::Array<NodeTemplatePrefabSetup>& prefabAssets() const { return m_prefabAssets; }

        /// get template for the entity - may be empty if we don't override it
        INLINE const EntityTemplatePtr& entityTemplate() const { return m_entityTemplate; }

        /// get template for components
        INLINE const base::Array<NodeTemplateComponentEntry>& componentTemplates() const { return m_componentTemplates; }

        //--

        /// find data for component
        ComponentTemplatePtr findComponentTemplate(base::StringID name) const;

        //--

    private:
        base::StringID m_name; // name of the node, can't be empty
        NodeTemplatePlacement m_placement; // placement in relation to parent node
        NodeTemplateType m_type = NodeTemplateType::Content; // type of the node

        //--

        StreamingModel m_streamingModel = StreamingModel::Auto;  // streaming mode for this node
        float m_streamingDistanceOverride = 0.0f; // override for streaming distance
        // TODO: object category, streaming distance multiplier, etc

        //--

        base::Array<NodeTemplatePrefabSetup> m_prefabAssets; // prefab to use

        EntityTemplatePtr m_entityTemplate; // entity data - NULL only if we are an override node
        base::Array<NodeTemplateComponentEntry> m_componentTemplates; // component data - empty
    };

    //--

} // game