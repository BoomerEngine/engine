/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "scene_common_glue.inl"

namespace scene
{
    /// update context for the entity
    struct SCENE_COMMON_API UpdateContext : public base::NoCopy
    {
    public:
        float m_dt; // simulation delta time for this frame
    };

    //---

    class AssetMetadataCache;

    class NodePath;

    class Scene;
    typedef base::RefPtr<Scene> ScenePtr;

    class IRuntimeSystem;
    typedef base::RefPtr<IRuntimeSystem> SystemPtr;

    class IAttachment;
    typedef base::RefPtr<IAttachment> AttachmentPtr;

    class TransformAttachment;
    typedef base::RefPtr<TransformAttachment> TransformAttachmentPtr;

    class Element;
    typedef base::RefPtr<Element> ElementPtr;
    typedef base::RefWeakPtr<Element> ElementWeakPtr;

    class Component;
    typedef base::RefPtr<Component> ComponentPtr;

    class Entity;
    typedef base::RefPtr<Entity> EntityPtr;

    /// runtime system type
    enum class SceneType : uint8_t
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

    class Layer;
    typedef base::RefPtr<Layer> LayerPtr;

    class IWorldParameters;
    typedef base::RefPtr<IWorldParameters> WorldParamsPtr;

    class WorldParameterContainer;
    typedef base::RefPtr<WorldParameterContainer> WorldParameterContainerPtr;

    //--

} // scene
