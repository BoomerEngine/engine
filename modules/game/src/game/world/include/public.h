/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "game_world_glue.inl"
#include "base/resource/include/resourceAsyncReference.h"

namespace game
{

    //---

    class NodePath;

    class ComponentGraph;

    class IComponentLink;
    typedef base::RefPtr<IComponentLink> ComponentLinkPtr;
    typedef base::RefWeakPtr<IComponentLink> ComponentWeakLinkPtr;

    class ComponentTransformLink;
    typedef base::RefPtr<ComponentTransformLink> ComponentTransformLinkPtr;

    class Component;
    typedef base::RefPtr<Component> ComponentPtr;
    typedef base::RefWeakPtr<Component> ComponentWeakPtr;

    class Entity;
    typedef base::RefPtr<Entity> EntityPtr;
    typedef base::RefWeakPtr<Entity> EntityWeakPtr;

    //--

    /// runtime system type
    enum class WorldType : uint8_t
    {
        // editor only scene, no game systems
        Editor,

        // true game scene, may contain players, replication, etc
        Game,
    };

    //--

    /// node visibility flags
    enum class NodeVisibilityFlag : uint16_t
    {
        GlobalHide, // hide everywhere, controlled by editor "eye" icon
        EditorOnly, // node is rendered only in the editor (helpers)
        HideInMainView, // do not render in main view
        HideInCascades, // do not render in global shadow cascades
        HideInShadows, // do not render in general shadows
        HideInReflections, // do not render in reflection probes
        HideInStaticShadows, // do not render in static shadows (for lightmaps)
    };

    typedef base::BitFlags<NodeVisibilityFlag> NodeVisibilityFlags;

    //--

    class Prefab;
    typedef base::RefPtr<Prefab> PrefabPtr;
    typedef base::RefWeakPtr<Prefab> PrefabWeakPtr;
    typedef base::res::Ref<Prefab> PrefabRef;

    struct PrefabDependencies;

    class CompiledWorld;
    typedef base::RefPtr<CompiledWorld> CompiledWorldPtr;

    class NodeTemplate;
    typedef base::RefPtr<NodeTemplate> NodeTemplatePtr;

    class EntityTemplate;
    typedef base::RefPtr<EntityTemplate> EntityTemplatePtr;

    class ComponentTemplate;
    typedef base::RefPtr<ComponentTemplate> ComponentTemplatePtr;

    class NodeTemplateContainer;
    typedef base::RefPtr<NodeTemplateContainer> NodeTemplateContainerPtr;

    struct NodeTemplateCompiledOutput;
    typedef base::RefPtr<NodeTemplateCompiledOutput> NodeTemplateCompiledOutputPtr;

    struct NodeTemplateCompiledData;
    typedef base::RefPtr<NodeTemplateCompiledData> NodeTemplateCompiledDataPtr;

    //--

    class World;
    typedef base::RefPtr<World> WorldPtr;

    class IWorldSystem;
    typedef base::RefPtr<IWorldSystem> WorldSystemPtr;

    //--

    class WorldDefinition;
    typedef base::RefPtr<WorldDefinition> WorldDefinitionPtr;
    typedef base::res::Ref<WorldDefinition> WorldDefinitionRef;

    class WorldLayer;
    typedef base::RefPtr<WorldLayer> WorldLayerPtr;

    class IWorldParameters;
    typedef base::RefPtr<IWorldParameters> WorldParametersPtr;

    class WorldSectorData;
    typedef base::RefPtr<WorldSectorData> WorldSectorDataPtr;

    class WorldSectorGridLevel;
    typedef base::RefPtr<WorldSectorGridLevel> WorldSectorGridLevelPtr;

    class WorldSectors;
    typedef base::RefPtr<WorldSectors> WorldSectorsPtr;
    typedef base::res::Ref<WorldSectors> WorldSectorsRef;
    typedef base::res::AsyncRef<WorldSectors> WorldSectorsAsyncRef;
    
    class WorldSectorUnpackedData;
    typedef base::RefPtr<WorldSectorUnpackedData> WorldSectorUnpackedDataPtr;

    //--

    class CameraComponent;
    class InputComponent;

    //---

} // game
