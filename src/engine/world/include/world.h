/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "core/memory/include/structurePool.h"
#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

///----

/// scene content observer
struct ENGINE_WORLD_API WorldObserver
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(WorldObserver);

    ExactPosition position;
    Vector3 velocity;
    float maxStreamingRange = 0.0f;
};

///----

/// world ticking
struct ENGINE_WORLD_API WorldTickStats
{
    uint32_t numTransformRootJobs = 0;
    uint32_t numTransformTotalJobs = 0;
    double transformTime = 0.0;

    uint32_t numUpdateRootJobs = 0;
    uint32_t numUpdateTotalJobs = 0;
    uint32_t numUpdateFixupJobs = 0;
    uint32_t numUpdateFixupPasses = 0;
    double updateTime = 0.0;

    double systemTine = 0.0;
};

/// world stats
struct ENGINE_WORLD_API WorldStats
{
    double totalTime = 0.0;

    double systemTime = 0.0f;
    double behaviorTime = 0.0f;
    double preTickTime = 0.0;
    double postTickTime = 0.0;

    uint32_t numEntites = 0;
    uint32_t numEntitiesAttached = 0;
    uint32_t numEntitiesDetached = 0;

    WorldTickStats preTick;
    WorldTickStats postTick;
};

///----

/// world rendering context
struct ENGINE_WORLD_API WorldRenderingContext
{
    CameraSetup cameraSetup;
    CameraContextPtr cameraContext = nullptr;

    typedef std::function<void(FrameParams& frame, DebugGeometryCollector& debug)> TFrameRenderCallback;
    TFrameRenderCallback callback;
};

///----

/// world creation setup
struct ENGINE_WORLD_API WorldCreationSetup
{
    bool hasRendering = true;
    bool hasPhysics = true;
    bool hasAudio = true;
    bool hasStreaming = true;
    bool editor = false;

    const RenderingScene* existingRenderingScene = nullptr;
};

///----

/// type of world
enum class WorldType : uint8_t
{
    SimplePreview, // simple preview, no real game
    EditorPreview, // raw scene preview
    Compiled, // runtime world with compiled data
};

///----

/// create persistent world observer around which we should stream the content
class ENGINE_WORLD_API WorldPersistentObserver : public IObject
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(WorldPersistentObserver, IObject);

public:
    WorldPersistentObserver();

    void move(Vector3 position, Vector3 velocity = Vector3::ZERO());

    bool snapshot(WorldObserverInfo& outInfo) const;

public:
    SpinLock m_lock;

    bool m_valid = false;
    Vector3 m_position;
    Vector3 m_velocitiy;
};

///----

/// abstract world streaming task - created by streaming system and run on fiber to load content and create 
class ENGINE_WORLD_API IWorldStreamingTask : public IObject
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS);
    RTTI_DECLARE_VIRTUAL_CLASS(IWorldStreamingTask, IObject);

public:
    virtual ~IWorldStreamingTask();

    /// did we finish ?
    virtual bool finished() const = 0;

    /// request streaming task to be canceled
    virtual void requestCancel() = 0;

    /// process the task, can be called directly (blocking) 
    /// but it should be called on job
    CAN_YIELD virtual void process() = 0;
};

///----

/// world streaming 
class ENGINE_WORLD_API IWorldStreaming : public IObject
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS);
    RTTI_DECLARE_VIRTUAL_CLASS(IWorldStreaming, IObject);

public:
    virtual ~IWorldStreaming();

    /// create streaming task for loading content around given observers
    virtual WorldStreamingTaskPtr createStreamingTask(const Array<WorldObserverInfo>& observers) const = 0;

    /// apply finished streaming update task
    /// first outgoing entities are detached then new entities are attached
    /// TODO: partial "budgeted" attach to minimize hitching
    virtual void applyStreamingTask(World* world, const IWorldStreamingTask* task) = 0;
};

///----

/// setup for entity attachment
struct ENGINE_WORLD_API EntityAttachmentSetup
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(EntityAttachmentSetup);

public:
    EntityAttachmentSetup();

    // transform parent entity to follow with movement
    EntityPtr transformParent;

    // custom absolute position to ensure on attachment, will override whatever we would have from the local position
    Transform customLocalToWorld;
    bool ensureCustomLocalToWorld = false;

    // update parent entity to attach to for updates, usually the creator or null
    EntityPtr updateParent;

    // entity ID (global) under which it can be found
    EntityStaticID staticID = 0;

    // custom entity name to give to the entity, if none is given then a automatic one is assigned
    StringID name;
};

///----
    
/// runtime game world class
/// this is the THE place where everything is simulated, rendered, etc
/// NOTE: we follow classical Entity-Component-System model here: entities groups components, most of the work is done in systems
/// NOTE: this object is never saved neither are the components inside
class ENGINE_WORLD_API World final : public IObject
{
    RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
    RTTI_DECLARE_VIRTUAL_CLASS(World, IObject);

public:
    World(WorldType type, const IWorldParametersSource* parameters, IWorldStreaming* streaming);
    virtual ~World();

    //---

    /// get world type
    INLINE WorldType type() const { return m_type; }

    /// get last tick stats
    INLINE const WorldStats& stats() const { return m_stats; }

    /// streaming system, valid only for compiled world
    INLINE const IWorldStreaming* streaming() const { return m_streaming; }

    //---

    /// get world system
    /// NOTE: requesting system that does not exist if a fatal error, please check scene flags first
    template< typename T >
    INLINE T* system() const
    {
        static auto userIndex = ClassID<T>()->userIndex();
        ASSERT_EX(userIndex != -1, "Trying to access unregistered scene system");
        auto system  = (T*) m_systemMap[userIndex];
        ASSERT_EX(!system || system->cls()->is(T::GetStaticClass()), "Invalid system registered");
        return system;
    }

    /// get system pointer
    INLINE IWorldSystem* systemPtr(short index) const
    {
        return m_systemMap[index];
    }

    //---

    /// get parameters
    template< typename T >
    INLINE const T& params() const
    {
        static auto userIndex = ClassID<T>()->userIndex();
        ASSERT_EX(userIndex != -1, "Trying to access unregistered scene system");
        auto params = (T*)m_parametersMap[userIndex];
        ASSERT_EX(!params || params->cls()->is(T::GetStaticClass()), "Invalid system registered");
        return *params;
    }

    /// get system pointer
    INLINE IWorldParameters* paramsPtr(short index) const
    {
        return m_parametersMap[index];
    }

    //---

    /// update scene and all the runtime systems
    /// NOTE: this function will wait for the internal tasks to finish
    CAN_YIELD void update(double dt);

    //--

    /// render the debug fragments
    void renderDebugFragments(DebugGeometryCollector& debug);

    /// render special debug GUI with ImGui
    void renderDebugGui();

    /// render viewport
    CAN_YIELD void renderViewport(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, const WorldRenderingContext& context);

    ///--

    /// attach entity to the world
    /// NOTE: entity cannot be already attached
    /// NOTE: world will add a reference to the entity object
    /// NOTE: must be called on main thread
    void attachEntity(Entity* entity, const EntityAttachmentSetup& setup = EntityAttachmentSetup());

    /// detach previously attached entity from the world
    /// NOTE: world will remove a reference to the entity object (the entity object may get destroyed)
    /// NOTE: must be called on main thread
    void detachEntity(Entity* entity);

    ///---

    /// attach persistent observer, content will be streamed around it
    void attachPersistentObserver(WorldPersistentObserver* observer);

    /// detach persistent observer
    void dettachPersistentObserver(WorldPersistentObserver* observer);

    ///---

    /// "spawn" prefab on the world, shorthand for prefab compilation, instantiation and attachment of the entities to the world
    //EntityPtr createPrefabInstance(const Transform& placement, const Prefab* prefab, StringID appearance = "default"_id);

    ///---

    /// create world with no backing scene, typical case for editor preview panels, some systems may be disabled
    static WorldPtr CreatePreviewWorld();

    /// create world based on the existing loaded raw world, we will share parameters, no content streaming
    static WorldPtr CreateEditorWorld(const RawWorldData* editableWorld);

    /// create world based on a compiled scene
    static WorldPtr CreateCompiledWorld(const CompiledWorldData* compiledScene);

    ///--

    /// enable/disable automatic streaming based on the observers
    void enableStreaming(bool flag);

    /// start stream content and wait until it's done and everything was attached
    CAN_YIELD void syncStreamingLoad();

    ///--

    /// find entity by static (editor) ID
    EntityPtr findEntityByStaticID(EntityStaticID id) const;

    /// find entity by static (editor) path
    EntityPtr findEntityByStaticPath(StringView staticPath) const;

    /// find entity by runtime ID (globally unique, mostly used when sending stuff over network)
    EntityPtr findEntityByRuntimeID(EntityRuntimeID id) const;

    //---

    /// find entity by a relative path:
    /// child: "child01"
    /// sibling: "../sibling"
    /// parent: ".."
    /// parent or parent: "../.."
    /// NOTE: fuckin slow
    EntityPtr findEntityByPath(const Entity* relativeTo, StringView path) const;

    ///--

private:
    WorldType m_type;

    //--

    static const uint32_t MAX_SYSTEM = 64;
    IWorldSystem* m_systemMap[MAX_SYSTEM];
    Array<WorldSystemPtr> m_systems; // all systems registered in the scene

    void initializeSystems();
    void destroySystems();

    //--

    static const uint32_t MAX_PARAMETERS = 64;
    IWorldParameters* m_parametersMap[MAX_PARAMETERS];
    Array<WorldParametersPtr> m_parameters;

    void initializeParameters();

    //--

    struct EntityNode;

    struct EntityHierarchy
    {
        EntityNode* parent = nullptr;
        EntityNode* children = nullptr;
        EntityNode* next = nullptr;
        EntityNode** prev = nullptr;
    };

    struct EntityNode
    {
        EntityPtr ptr;

        StringID name; // NOTE: may be duplicated
        EntityStaticID staticId = 0;
        EntityRuntimeID runtimeId = 0;

        WorldUpdateMask localUpdateMask; // just this node
        uint32_t numUpdatableChildren = 0;

        mutable char transformUpdateDepth = -1;

        EntityHierarchy transformHierarchy;
        EntityHierarchy updateHierarchy;
    };

    Mutex m_entitiesLock; // fat lock
    StructurePool<EntityNode> m_entitiesPool; // pool for the entities
    HashMap<Entity*, EntityNode*> m_entitiesMap; // maps entities to the data 
    HashSet<const EntityNode*> m_rootUpdateEntities; // roots of the update tree
    HashSet<const EntityNode*> m_rootTransformEntities; // roots of the transform tree
    HashSet<const EntityNode*> m_rootUpdateEntitiesActiveList; // roots of the update tree that actually need updating (ie. non zero update mask)

    volatile bool m_trackEntitiesAttachedDuringUpdate = false;
    HashSet<const EntityNode*> m_entitiesAttachedDuringUpdate;
    Array<EntityPtr> m_entitiesAttachedDuringUpdateArray;

    std::atomic<uint32_t> m_numEntitiesAttached = 0;
    std::atomic<uint32_t> m_numEntitiesDetached = 0;

    void updateUpdateMask_NoLock(EntityNode* node);

    SpinLock m_entitiesIDMappingLock;
    HashMap<EntityStaticID, const EntityNode*> m_entityStaticIDMap;
    HashMap<EntityRuntimeID, const EntityNode*> m_entityRuntimeIDMap;

    //--

    struct EntityTransformUpdateRequest
    {
        EntityPtr ptr;
        Transform transform;
        bool localUpdate = true;
        mutable EntityNode* node = nullptr;
    };

    Mutex m_entitiesTransformUpdateLock; // fat lock
    StructurePool<EntityTransformUpdateRequest> m_entitiesTransformRequestPool; // pool for the entities
    HashMap<Entity*, EntityTransformUpdateRequest*> m_entitiesTransformRequests;
    HashSet<const EntityNode*> m_entitiesTransformAllNodes;

    void calculateDryLocalToWorldTransform_NoLock(const EntityNode*, Transform& outLocalToWorld) const;
    void scheduleEntityForTransformUpdate(Entity* entity, bool local, const Transform& newTransform);

    //--

    static const uint32_t MAX_BUCKET_LEVELS = 16;

    Array<EntityPtr> m_updateBuckets[MAX_BUCKET_LEVELS];

    void extractUpdateBuckets_NoLock(WorldUpdatePhase phase);
    void updateEntitiesInternalWithFixup(double dt, WorldUpdatePhase phase, WorldTickStats& outStats);

    //--

    struct EntityTransformJob
    {
        EntityTransformUpdateRequest* request = nullptr;
        const EntityNode* node = nullptr;
    };

    Array<EntityTransformJob> m_transformBuckets[MAX_BUCKET_LEVELS];

    void extractTransformBuckets();
    void processTransformRequests(WorldTickStats& outStats);

    //--

    WorldStats m_stats;

    void updateInternal(double dt, WorldStats& outStats);
    void updateInternal_Tick(double dt, WorldUpdatePhase phase, WorldTickStats& outStats);
    void updateInternal_Transform(WorldUpdatePhase phase, WorldTickStats& outStats);

    //--

    WorldStreamingPtr m_streaming;
    WorldStreamingTaskPtr m_streamingTask;

    SpinLock m_streamingPersistentObserversLock;
    Array<WorldPersistentObserverPtr> m_streamingPersistentObservers;

    bool m_streamingEnabled = false;

    void updateStreaming();
    void collectStreamingObservers(Array<WorldObserverInfo>& outInfos) const;

    //--

    friend class Entity;
        
};

//---

END_BOOMER_NAMESPACE()
