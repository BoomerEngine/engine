/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_world_glue.inl"

#include "core/resource/include/asyncReference.h"

namespace boomer::rendering
{
    struct FrameParams;
}

BEGIN_BOOMER_NAMESPACE()

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
typedef ResourceRef<Prefab> PrefabRef;

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

class StreamingTask;

class StreamingIslandInstance;
typedef RefPtr<StreamingIslandInstance> StreamingIslandInstancePtr;

class StreamingIsland;
typedef RefPtr<StreamingIsland> StreamingIslandPtr;

//---

class CompiledScene;
typedef RefPtr<CompiledScene> CompiledScenePtr;
typedef ResourceRef<CompiledScene> CompiledSceneRef;
typedef ResourceAsyncRef<CompiledScene> CompiledSceneAsyncRef;

//---

// information about streaming observer
struct ENGINE_WORLD_API StreamingObserverInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(StreamingObserverInfo);

    Vector3 position;
    Vector3 velocity;

    StreamingObserverInfo();
};

//--

END_BOOMER_NAMESPACE()
