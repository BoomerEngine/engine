/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_world_glue.inl"
#include "base/resource/include/resourceAsyncReference.h"

namespace rendering
{
    namespace scene
    {
        struct FrameParams;
    } // scene
} // rendering

namespace base
{
    namespace world
    {

        //---

        class IEntityBehavior;
        typedef RefPtr<IEntityBehavior> EntityBehaviorPtr;
        typedef RefWeakPtr<IEntityBehavior> EntityBehaviorWeakPtr;

        class Entity;
        typedef RefPtr<Entity> EntityPtr;
        typedef RefWeakPtr<Entity> EntityWeakPtr;

        class EntityInputEvent;
        typedef RefPtr<EntityInputEvent> EntityInputEventPtr;

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

        typedef BitFlags<NodeVisibilityFlag> NodeVisibilityFlags;

        //--

        class Prefab;
        typedef RefPtr<Prefab> PrefabPtr;
        typedef RefWeakPtr<Prefab> PrefabWeakPtr;
        typedef res::Ref<Prefab> PrefabRef;

        struct PrefabDependencies;

        class CompiledWorld;
        typedef RefPtr<CompiledWorld> CompiledWorldPtr;

        class NodeTemplate;
        typedef RefPtr<NodeTemplate> NodeTemplatePtr;

        typedef uint64_t NodeGlobalID;

        class NodePathBuilder;

        //--

        class World;
        typedef RefPtr<World> WorldPtr;

        class IWorldSystem;
        typedef RefPtr<IWorldSystem> WorldSystemPtr;

        //--

        class RawLayer;
        typedef RefPtr<RawLayer> RawLayerPtr;

        class RawScene;
        typedef RefPtr<RawScene> RawScenePtr;

        //--

        class StreamingIslandInstance;
        typedef RefPtr<StreamingIslandInstance> StreamingIslandInstancePtr;

        class StreamingIsland;
        typedef RefPtr<StreamingIsland> StreamingIslandPtr;

        class StreamingSector;
        typedef RefPtr<StreamingSector> StreamingSectorPtr;
        typedef res::Ref<StreamingSector> StreamingSectorRef;
        typedef res::AsyncRef<StreamingSector> StreamingSectorAsyncRef;

        class StreamingGridSetup;
        typedef RefPtr<StreamingGridSetup> StreamingGridSetupPtr;

        //---

        class CompiledScene;
        typedef RefPtr<CompiledScene> CompiledScenePtr;
        typedef res::Ref<CompiledScene> CompiledSceneRef;
        typedef res::AsyncRef<CompiledScene> CompiledSceneAsyncRef;

        //---

    }  // world
} // base
