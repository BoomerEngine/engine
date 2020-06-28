/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "game_scene_glue.inl"
#include "base/resources/include/resourceAsyncReference.h"

namespace game
{
    /// update context for the entity
    struct GAME_SCENE_API UpdateContext : public base::NoCopy
    {
    public:
        float m_dt; // simulation delta time for this frame
    };

    //---

    class AssetMetadataCache;

    class NodePath;

    class IAttachment;
    typedef base::RefPtr<IAttachment> AttachmentPtr;
    typedef base::RefWeakPtr<IAttachment> AttachmentWeakPtr;

    class TransformAttachment;
    typedef base::RefPtr<TransformAttachment> TransformAttachmentPtr;

    class Component;
    typedef base::RefPtr<Component> ComponentPtr;
    typedef base::RefWeakPtr<Component> ComponentWeakPtr;

    class Entity;
    typedef base::RefPtr<Entity> EntityPtr;
    typedef base::RefWeakPtr<Entity> EntityWeakPtr;

    class IEntityMovement;
    typedef base::RefPtr<IEntityMovement> EntityMovementPtr;

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

    class EntityDataTemplate;
    typedef base::RefPtr<EntityDataTemplate> EntityDataTemplatePtr;

    class ComponentDataTemplate;
    typedef base::RefPtr<ComponentDataTemplate> ComponentDataTemplatePtr;

    class NodeTemplateContainer;
    typedef base::RefPtr<NodeTemplateContainer> NodeTemplateContainerPtr;

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
