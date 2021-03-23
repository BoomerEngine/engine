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

class CompiledWorldData;
typedef RefPtr<CompiledWorldData> CompiledWorldPtr;

class RawEntity;
typedef RefPtr<RawEntity> RawEntityPtr;

typedef uint64_t EntityStaticID;
typedef uint32_t EntityRuntimeID;

class EntityStaticIDBuilder;

//--

struct WorldRenderingContext;

class World;
typedef RefPtr<World> WorldPtr;

class IWorldParameters;
typedef RefPtr<IWorldParameters> WorldParametersPtr;

class IWorldSystem;
typedef RefPtr<IWorldSystem> WorldSystemPtr;

class IWorldViewEntity;
typedef RefPtr<IWorldViewEntity> WorldViewEntityPtr;

class IWorldStreaming;
typedef RefPtr<IWorldStreaming> WorldStreamingPtr;

class IWorldStreamingTask;
typedef RefPtr<IWorldStreamingTask> WorldStreamingTaskPtr;

class WorldPersistentObserver;
typedef RefPtr<WorldPersistentObserver> WorldPersistentObserverPtr;

/// source of parameters
class ENGINE_WORLD_API IWorldParametersSource : public NoCopy
{
public:
    virtual ~IWorldParametersSource();

    virtual Array<WorldParametersPtr> compileWorldParameters() const = 0;
};

enum class WorldUpdatePhase : uint8_t
{
    None, // not in world update, entities can be changed at will
    PreTick,
    PostTick,
};

typedef BitFlags<WorldUpdatePhase> WorldUpdateMask;

//--

class RawLayer;
typedef RefPtr<RawLayer> RawLayerPtr;

class RawWorldData;
typedef RefPtr<RawWorldData> RawWorldDataPtr;

//--

class CompiledStreamingIslandInstance;
typedef RefPtr<CompiledStreamingIslandInstance> CompiledStreamingIslandInstancePtr;

class CompiledStreamingIsland;
typedef RefPtr<CompiledStreamingIsland> CompiledStreamingIslandPtr;

class CompiledStreamingTask;
class CompiledWorldStreaming;

//---

class CompiledWorldData;
typedef RefPtr<CompiledWorldData> CompiledWorldDataPtr;
typedef ResourceRef<CompiledWorldData> CompiledWorldDataRef;
typedef ResourceAsyncRef<CompiledWorldData> CompiledWorldDataAsyncRef;

//---

// information about streaming observer
struct ENGINE_WORLD_API WorldObserverInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(WorldObserverInfo);

    Vector3 position;
    Vector3 velocity;

    WorldObserverInfo();
};

//--

END_BOOMER_NAMESPACE()
