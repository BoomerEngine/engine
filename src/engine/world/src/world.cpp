/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
*
***/

#include "build.h"
#include "world.h"
#include "worldSystem.h"
#include "worldRendering.h"
#include "worldParameters.h"
#include "worldEntity.h"
#include "worldEntityID.h"

#include "engine/imgui/include/imgui.h"
#include "engine/rendering/include/stats.h"

#include "core/object/include/rttiClassType.h"
#include "core/containers/include/queue.h"
#include "worldEntityID.h"

BEGIN_BOOMER_NAMESPACE()

//---

template< typename T >
static void UnlinkTransformList(T& link)
{
    DEBUG_CHECK_RETURN_EX(link.prev != nullptr, "Not linked");
    DEBUG_CHECK_RETURN_EX(link.parent != nullptr, "Not linked");

    if (link.next)
        link.next->transformHierarchy.prev = link.prev;
    if (link.prev)
        *link.prev = link.next;
    link.next = nullptr;
    link.prev = nullptr;
    link.parent = nullptr;
}

template< typename T >
static void LinkTransformList(T* parent, T* cur)
{
    DEBUG_CHECK_RETURN_EX(cur->transformHierarchy.prev == nullptr, "Already linked");
    DEBUG_CHECK_RETURN_EX(cur->transformHierarchy.next == nullptr, "Already linked");

    DEBUG_CHECK_RETURN_EX(cur->transformHierarchy.parent == nullptr, "Already linked");
    cur->transformHierarchy.parent = parent;

    auto*& list = parent->transformHierarchy.children;

    cur->transformHierarchy.prev = &list;
    cur->transformHierarchy.next = list;
    if (list)
        list->transformHierarchy.prev = &cur->transformHierarchy.next;
    list = cur;
}

//---

RTTI_BEGIN_TYPE_CLASS(WorldObserver);
    RTTI_PROPERTY(position);
    RTTI_PROPERTY(velocity);
    RTTI_PROPERTY(maxStreamingRange);
RTTI_END_TYPE();

//---

IWorldParametersSource::~IWorldParametersSource()
{}

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IWorldStreaming);
RTTI_END_TYPE();

IWorldStreaming::~IWorldStreaming()
{}

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IWorldStreamingTask);
RTTI_END_TYPE();

IWorldStreamingTask::~IWorldStreamingTask()
{}

//---

RTTI_BEGIN_TYPE_CLASS(WorldPersistentObserver);
    RTTI_PROPERTY(m_position);
    RTTI_PROPERTY(m_velocitiy);
RTTI_END_TYPE();

WorldPersistentObserver::WorldPersistentObserver()
{
}

void WorldPersistentObserver::move(Vector3 position, Vector3 velocity)
{
    auto lock = CreateLock(m_lock);
    m_velocitiy = velocity;
    m_position = position;
    m_valid = true;
}

bool WorldPersistentObserver::snapshot(WorldObserverInfo& outInfo) const
{
    auto lock = CreateLock(m_lock);

    if (m_valid)
    {
        outInfo.position = m_position;
        outInfo.velocity = m_velocitiy;
    }

    return m_valid;
}

//---

RTTI_BEGIN_TYPE_CLASS(EntityAttachmentSetup);
RTTI_PROPERTY(transformParent);
RTTI_PROPERTY(customLocalToWorld);
RTTI_PROPERTY(ensureCustomLocalToWorld);
RTTI_PROPERTY(updateParent);
RTTI_END_TYPE();

EntityAttachmentSetup::EntityAttachmentSetup()
{}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(World);
RTTI_END_TYPE();

///--

WorldPtr World::CreatePreviewWorld()
{
    return RefNew<World>(WorldType::SimplePreview, nullptr, nullptr);
}

///--


///--

World::World(WorldType type, const IWorldParametersSource* parameters, IWorldStreaming* streaming)
    : m_type(type)
    , m_streaming(AddRef(streaming))
{
    const auto typicalEntityCount = (type == WorldType::Compiled) ? (128 << 10) : (1 << 10);

    m_rootUpdateEntities.reserve(typicalEntityCount);
    m_entitiesMap.reserve(typicalEntityCount);
    m_rootUpdateEntities.reserve(typicalEntityCount);
    m_rootTransformEntities.reserve(typicalEntityCount);
    m_rootUpdateEntitiesActiveList.reserve(typicalEntityCount);
    m_entityStaticIDMap.reserve(typicalEntityCount);;
    m_entityRuntimeIDMap.reserve(typicalEntityCount);;

    m_entitiesTransformRequests.reserve(1024);

    m_entitiesAttachedDuringUpdate.reserve(256);
    m_entitiesAttachedDuringUpdateArray.reserve(256);

    if (parameters)
        m_parameters = parameters->compileWorldParameters();

    for (auto& bucket : m_transformBuckets)
        bucket.reserve(4096);

    for (auto& bucket : m_updateBuckets)
        bucket.reserve(4096);

    initializeParameters();
    initializeSystems();
}

World::~World()
{
    if (m_streamingTask)
    {
        m_streamingTask->requestCancel();
        m_streamingTask.reset();
    }

    for (auto* req : m_entitiesTransformRequests.values())
        m_entitiesTransformRequestPool.free(req);

    m_entitiesTransformRequests.clear();

    for (auto* node : m_entitiesMap.values())
    {
        if (node->ptr)
            node->ptr->detachFromWorld(this);

        node->ptr.reset();
        node->transformHierarchy = EntityHierarchy();
        node->updateHierarchy = EntityHierarchy();
        m_entitiesPool.free(node);
    }

    m_entitiesMap.clear();
    m_rootUpdateEntities.clear();
    m_rootTransformEntities.clear();
    m_rootUpdateEntitiesActiveList.clear();
    m_entitiesAttachedDuringUpdate.clear();
    m_entitiesAttachedDuringUpdateArray.clear();

    destroySystems();
}

struct ScopeBool : public NoCopy
{
public:
    ScopeBool(bool& flag)
        : m_flag(flag)
    {
        DEBUG_CHECK_EX(flag == false, "Flag expected to be false");
        flag = true;
    }

    ~ScopeBool()
    {
        DEBUG_CHECK_EX(m_flag == true, "Flag expected to be true");
        m_flag = false;
    }

private:
    bool& m_flag;
};

void CAN_YIELD World::update(double dt)
{
    PC_SCOPE_LVL1(WorldUpdate);

    ScopeTimer timer;

    updateStreaming();

    WorldStats stats;
    updateInternal(dt, stats);

    m_stats = stats;

    {
        auto lock = CreateLock(m_entitiesLock);
        m_stats.numEntitiesAttached = m_numEntitiesAttached.exchange(0);
        m_stats.numEntitiesDetached = m_numEntitiesDetached.exchange(0);
        m_stats.numEntites = m_entitiesMap.size();
        m_stats.totalTime = timer.timeElapsed();
    }
}

void World::updateInternal(double dt, WorldStats& outStats)
{
    {
        PC_SCOPE_LVL1(PreTickSystems);

        ScopeTimer timer;
        for (const auto& sys : m_systems)
            sys->handlePreTick(dt);
        outStats.preTick.updateTime = timer.timeElapsed();
    }

    {
        PC_SCOPE_LVL1(PreTickEntities);

        ScopeTimer timer;
        updateInternal_Tick(dt, WorldUpdatePhase::PreTick, outStats.preTick);
        updateInternal_Transform(WorldUpdatePhase::PreTick, outStats.preTick);
        outStats.preTickTime = timer.timeElapsed();
    }

    {
        PC_SCOPE_LVL1(MainTick);

        ScopeTimer timer;

        for (const auto& sys : m_systems)
            sys->handleMainTickStart(dt);

        // TODO: tick behaviors

        for (const auto& sys : m_systems)
            sys->handleMainTickFinish(dt);

        for (const auto& sys : m_systems)
            sys->handleMainTickPublish(dt);

        outStats.systemTime = timer.timeElapsed();
    }

    {
        PC_SCOPE_LVL1(PostTickEntities);

        ScopeTimer timer;
        updateInternal_Tick(dt, WorldUpdatePhase::PostTick, outStats.preTick);
        updateInternal_Transform(WorldUpdatePhase::PostTick, outStats.preTick);
        outStats.preTickTime = timer.timeElapsed();
    }

    {
        PC_SCOPE_LVL1(PostTickSystems);

        ScopeTimer timer;
        for (const auto& sys : m_systems)
            sys->handlePostTick(dt);
        outStats.postTick.updateTime = timer.timeElapsed();
    }
}

void World::extractUpdateBuckets_NoLock(WorldUpdatePhase phase)
{
    PC_SCOPE_LVL1(ExtractUpdateBuckets);

    Queue<std::pair<const EntityNode*, int>, InplaceArray<std::pair<const EntityNode*, int>, 100>> stack;

    for (const auto* root : m_rootUpdateEntitiesActiveList.keys())
    {
        DEBUG_CHECK(root->localUpdateMask.rawValue() || root->numUpdatableChildren);

        stack.push(std::make_pair(root, 0));

        while (!stack.empty())
        {
            const auto* node = stack.top().first;
            auto depth = stack.top().second;
            stack.pop();

            // collect node only if it has any update for this level
            if (node->localUpdateMask.test(phase))
            {
                if (depth < MAX_BUCKET_LEVELS)
                    m_updateBuckets[depth].pushBack(node->ptr);

                depth += 1; // since we update this node make sure our children get updated later
            }

            // if node has any updatable children we need to visit them
            if (root->numUpdatableChildren)
            {
                const auto* child = node->updateHierarchy.children;
                while (child)
                {
                    if (child->localUpdateMask.test(phase) || child->numUpdatableChildren)
                        stack.push(std::make_pair(child, depth)); // add only if we have a business there
                    child = child->updateHierarchy.next;
                }
            }
        }
    }
}

void World::updateEntitiesInternalWithFixup(double dt, WorldUpdatePhase phase, WorldTickStats& outStats)
{
    PC_SCOPE_LVL1(Tick);

    // TODO: fibers
    outStats.numUpdateRootJobs += m_updateBuckets[0].size();
    for (auto& bucket : m_updateBuckets)
    {
        outStats.numUpdateTotalJobs += bucket.size();
        for (const auto& ptr : bucket)
            ptr->handleUpdate(EntityThreadContext(), phase, dt);

        bucket.reset();
    }

    // fixups
    for (;;)
    {
        PC_SCOPE_LVL1(FixupTick);

        // get list of entities that got attached in the update of already existing entities
        {
            auto lock = CreateLock(m_entitiesLock);

            DEBUG_CHECK(m_entitiesAttachedDuringUpdateArray.empty());
            m_entitiesAttachedDuringUpdateArray.reset();
            m_entitiesAttachedDuringUpdateArray.reserve(m_entitiesAttachedDuringUpdate.size());

            for (const auto* node : m_entitiesAttachedDuringUpdate.keys())
                m_entitiesAttachedDuringUpdateArray.emplaceBack(node->ptr);

            m_entitiesAttachedDuringUpdate.reset();

            if (m_entitiesAttachedDuringUpdateArray.empty())
            {
                m_trackEntitiesAttachedDuringUpdate = false;
                break;
            }
        }

        // tick the entities in a standard creation order without threads
        outStats.numUpdateFixupPasses += 1;
        outStats.numUpdateFixupJobs += m_entitiesAttachedDuringUpdateArray.size();
        for (const auto& ent : m_entitiesAttachedDuringUpdateArray)
            ent->handleUpdate(EntityThreadContext(), phase, dt);

        // make sure we don't keep any referenced
        m_entitiesAttachedDuringUpdateArray.reset();
    }

    DEBUG_CHECK(!m_trackEntitiesAttachedDuringUpdate);
    DEBUG_CHECK(m_entitiesAttachedDuringUpdate.empty());
    DEBUG_CHECK(m_entitiesAttachedDuringUpdateArray.empty());
}

void World::extractTransformBuckets()
{
    PC_SCOPE_LVL1(ExtractTransformBuckets);

    auto lock = CreateLock(m_entitiesTransformUpdateLock);
    auto lock2 = CreateLock(m_entitiesLock);

    // explore hierarchy of dirty nodes, make sure to assign each node to a proper depth
    Queue<std::pair<const EntityNode*, int>, InplaceArray<std::pair<const EntityNode*, int>, 100>> stack;
    for (const auto* req : m_entitiesTransformRequests.values())
    {
        req->node = nullptr;
        m_entitiesMap.find(req->ptr, req->node);

        if (req->node)
        {
            stack.push(std::make_pair(req->node, 0));

            while (!stack.empty())
            {
                const auto* node = stack.top().first;
                auto depth = stack.top().second;
                stack.pop();

                m_entitiesTransformAllNodes.insert(node);

                // node may be seen more than once if more than one of parents got a transform request, we need to assign the node to the deepest level
                if (depth > node->transformUpdateDepth)
                { 
                    node->transformUpdateDepth = depth;

                    const auto* child = node->transformHierarchy.children;
                    while (child)
                    {
                        stack.push(std::make_pair(child, depth + 1));
                        child = child->transformHierarchy.next;
                    }
                }
            }
        }
    }

    // put jobs in buckets
    for (auto* node : m_entitiesTransformAllNodes.keys())
    {
        EntityTransformJob job;
        job.node = node;

        m_entitiesTransformRequests.find(node->ptr, job.request);

        DEBUG_CHECK(node->transformUpdateDepth != -1);
        if (node->transformUpdateDepth >= 0 && node->transformUpdateDepth < MAX_BUCKET_LEVELS)
            m_transformBuckets[node->transformUpdateDepth].pushBack(job);
    }

    // prepare for next pass
    m_entitiesTransformRequests.reset();
    m_entitiesTransformAllNodes.reset();
}

void World::processTransformRequests(WorldTickStats& outStats)
{
    PC_SCOPE_LVL1(Transform);

    // TODO: fibers
    outStats.numTransformRootJobs += m_transformBuckets[0].size();
    for (auto& bucket : m_transformBuckets)
    {
        outStats.numTransformTotalJobs += bucket.size();
        for (const auto& job : bucket)
        {
            if (job.request)
            {
                if (job.request->localUpdate)
                {
                    if (job.node->transformHierarchy.parent)
                    {
                        const auto& parentToWorld = job.node->transformHierarchy.parent->ptr->cachedWorldTransform();
                        job.node->ptr->applyTransform(this, EntityThreadContext(), job.request->transform, job.request->transform * parentToWorld);
                    }
                    else
                    {
                        job.node->ptr->applyTransform(this, EntityThreadContext(), job.request->transform, job.request->transform);
                    }
                }
                else
                {
                    if (job.node->transformHierarchy.parent)
                    {
                        const auto& parentToWorld = job.node->transformHierarchy.parent->ptr->cachedWorldTransform();
                        job.node->ptr->applyTransform(this, EntityThreadContext(), job.request->transform / parentToWorld, job.request->transform);
                    }
                    else
                    {
                        job.node->ptr->applyTransform(this, EntityThreadContext(), job.request->transform, job.request->transform);
                    }
                }

                job.request->node = nullptr;
            }
            else
            {
                DEBUG_CHECK(job.node->transformHierarchy.parent);
                if (job.node->transformHierarchy.parent)
                {
                    const auto localToParent = job.node->ptr->transform();
                    const auto& parentToWorld = job.node->transformHierarchy.parent->ptr->cachedWorldTransform();
                    job.node->ptr->applyTransform(this, EntityThreadContext(), localToParent, localToParent * parentToWorld);
                }
            }

            job.node->transformUpdateDepth = -1;
        }
    }
}

void World::updateInternal_Tick(double dt, WorldUpdatePhase phase, WorldTickStats& outStats)
{
    ScopeTimer timer;

    {
        auto lock = CreateLock(m_entitiesLock);

        // make sure all entities that get added during the update get ticked as well (the fixup)
        DEBUG_CHECK(m_entitiesAttachedDuringUpdate.empty());
        m_trackEntitiesAttachedDuringUpdate = true;
        m_entitiesAttachedDuringUpdate.reset();

        // collect update buckets
        extractUpdateBuckets_NoLock(phase);
    }

    updateEntitiesInternalWithFixup(dt, phase, outStats);
    outStats.updateTime = timer.timeElapsed();
}

void World::updateInternal_Transform(WorldUpdatePhase phase, WorldTickStats& outStats)
{
    ScopeTimer timer;

    // get jobs to process
    extractTransformBuckets();

    // process the jobs
    processTransformRequests(outStats);

    // release the jobs
    {
        auto lock = CreateLock(m_entitiesTransformUpdateLock);

        for (auto& bucket : m_transformBuckets)
        {
            for (const auto& job : bucket)
            {
                if (job.request)
                {
                    job.request->ptr.reset();
                    m_entitiesTransformRequestPool.free(job.request);
                }
            }

            bucket.reset();
        }
    }

    outStats.transformTime = timer.timeElapsed();
}

void World::collectStreamingObservers(Array<WorldObserverInfo>& outInfos) const
{
    {
        auto lock = CreateLock(m_streamingPersistentObserversLock);
        for (const auto& observer : m_streamingPersistentObservers)
        {
            WorldObserverInfo info;
            if (observer->snapshot(info))
                outInfos.pushBack(info);
        }
    }

    if (outInfos.empty())
    {
        auto& defaultObserver = outInfos.emplaceBack();
    }
}

void World::syncStreamingLoad()
{
    // always disable streaming before doing a dry load
    enableStreaming(false);

    // collect observers
    InplaceArray<WorldObserverInfo, 4> observers;
    collectStreamingObservers(observers);

    // start streaming
    if (auto task = m_streaming->createStreamingTask(observers))
        m_streaming->applyStreamingTask(this, task);
}

void World::updateStreaming()
{
    PC_SCOPE_LVL1(UpdateStreaming);

    if (m_streamingTask)
    {
        if (m_streamingTask->finished())
        {
            m_streaming->applyStreamingTask(this, m_streamingTask);
            m_streamingTask.reset();
        }
    }

    if (!m_streamingTask && m_streamingEnabled)
    {
        // collect observers
        InplaceArray<WorldObserverInfo, 4> observers;
        collectStreamingObservers(observers);

        // start streaming
        m_streamingTask = m_streaming->createStreamingTask(observers);
        if (m_streamingTask)
        {
            auto task = m_streamingTask;
            RunFiber("WorldStreaming") << [task](FIBER_FUNC)
            {
                task->process();
            };
        }
    }
}

void World::attachPersistentObserver(WorldPersistentObserver* observer)
{
    DEBUG_CHECK_RETURN_EX(observer, "Invalid observer");

    auto lock = CreateLock(m_streamingPersistentObserversLock);
    DEBUG_CHECK_RETURN_EX(!m_streamingPersistentObservers.contains(observer), "Already registered");

    m_streamingPersistentObservers.emplaceBack(AddRef(observer));
}

void World::dettachPersistentObserver(WorldPersistentObserver* observer)
{
    DEBUG_CHECK_RETURN_EX(observer, "Invalid observer");

    DEBUG_CHECK_RETURN_EX(m_streamingPersistentObservers.contains(observer), "Not registered");
    m_streamingPersistentObservers.remove(observer);
}

void World::enableStreaming(bool flag)
{
    m_streamingEnabled = flag;

    if (!flag && m_streamingTask)
    {
        m_streamingTask->requestCancel();
        m_streamingTask.reset();
    }
}

//--

void World::updateUpdateMask_NoLock(EntityNode* node)
{
    const auto oldLocalMask = node->localUpdateMask;
    node->ptr->handleUpdateMask(node->localUpdateMask);

    if (node->localUpdateMask.rawValue() && !oldLocalMask.rawValue()) // update added
    {
        if (node->updateHierarchy.parent)
        {
            auto lastParent = node->updateHierarchy.parent;
            while (node->updateHierarchy.parent)
            {
                node->updateHierarchy.parent->numUpdatableChildren += 1;
                if (node->updateHierarchy.parent->numUpdatableChildren > 1)
                    break; // already updated

                lastParent = node->updateHierarchy.parent;
                node = node->updateHierarchy.parent;
            }
        }

        // make sure root is updated
        DEBUG_CHECK(node->numUpdatableChildren || node->localUpdateMask.rawValue());
        m_rootUpdateEntitiesActiveList.insert(node);
    }
    else if (!node->localUpdateMask.rawValue() && oldLocalMask.rawValue()) // update removed
    {
        if (node->updateHierarchy.parent)
        {
            auto lastParent = node->updateHierarchy.parent;
            while (node->updateHierarchy.parent)
            {
                node->updateHierarchy.parent->numUpdatableChildren -= 1;
                if (node->updateHierarchy.parent->numUpdatableChildren > 0)
                    break; // already updated

                lastParent = node->updateHierarchy.parent;
                node = node->updateHierarchy.parent;
            }
        }

        // make sure root is removed if no longer needed
        if (node->numUpdatableChildren == 0 && !node->localUpdateMask.rawValue())
            m_rootUpdateEntitiesActiveList.remove(node);
    }    
}

void World::attachEntity(Entity* entity, const EntityAttachmentSetup& setup)
{
    Transform localToParentTransform;
    Transform localToWorldTransform;

    StringID assignedName;
    EntityStaticID assignedStaticID = 0;
    EntityRuntimeID assignedRuntimeID = 0;

    {
        auto lock = CreateLock(m_entitiesLock);

        DEBUG_CHECK_RETURN_EX(entity, "Invalid entity");
        DEBUG_CHECK_RETURN_EX(entity->world() == nullptr, "Entity already owned by some world");
        DEBUG_CHECK_RETURN_EX(!m_entitiesMap.contains(entity), "Entity already registered");

        // link entity and local data
        auto node = m_entitiesPool.create();
        node->ptr = AddRef(entity);
        m_entitiesMap[entity] = node;
        m_numEntitiesAttached += 1;

        // find and link to transform parent
        {
            EntityNode* transformParent = nullptr;
            if (setup.transformParent)
            {
                m_entitiesMap.find(setup.transformParent, transformParent);
                DEBUG_CHECK_EX(transformParent, "Transform parent entity not attached to the same world");
            }

            if (transformParent)
                LinkTransformList(transformParent, node);
            else
                m_rootTransformEntities.insert(node);
        }

        // find and link to update parent
        {
            m_rootUpdateEntities.insert(node);
        }

        // calculate initial transform and push it to entity
        if (setup.ensureCustomLocalToWorld)
        {
            localToWorldTransform = setup.customLocalToWorld;

            if (node->transformHierarchy.parent)
            {
                const auto& parentToWorld = node->transformHierarchy.parent->ptr->m_cachedLocalToWorldTransfrom;
                localToParentTransform = localToWorldTransform / parentToWorld;
            }
            else
            {
                localToParentTransform = localToWorldTransform;
            }
        }
        else
        {
            localToParentTransform = entity->transform();

            if (node->transformHierarchy.parent)
            {
                const auto& parentToWorld = node->transformHierarchy.parent->ptr->m_cachedLocalToWorldTransfrom;
                localToWorldTransform = localToParentTransform * parentToWorld;
            }
            else
            {
                localToWorldTransform = localToParentTransform;
            }
        }

        // if we are during update phase make sure entity gets ticked
        if (m_trackEntitiesAttachedDuringUpdate)
            m_entitiesAttachedDuringUpdate.insert(node);

        // ID
        {
            auto lock = CreateLock(m_entitiesIDMappingLock);

            if (setup.staticID)
            {
                DEBUG_CHECK_EX(!m_entityStaticIDMap.contains(setup.staticID), "Duplicated entity static ID");

                assignedStaticID = setup.staticID;
                node->staticId = assignedStaticID;
                m_entityStaticIDMap[assignedStaticID] = node;
            }

            static std::atomic<EntityRuntimeID> GRuntimeIDAllocator;

            assignedRuntimeID = GRuntimeIDAllocator++;
            node->runtimeId = assignedRuntimeID;
            m_entityRuntimeIDMap[assignedRuntimeID] = node;
        }

        // Name
        if (setup.name)
            assignedName = setup.name;
        else
            assignedName = entity->cls()->name(); // just use class name as name

        node->name = assignedName;

        // insert node to the update system
        updateUpdateMask_NoLock(node);
    }

    // inform entity
    // NOTE: must be done last in case entity detaches...
    entity->attachToWorld(this, localToParentTransform, localToWorldTransform, assignedName, assignedStaticID, assignedRuntimeID);
}

void World::detachEntity(Entity* entity)
{
    {
        auto lock = CreateLock(m_entitiesLock);

        DEBUG_CHECK_RETURN_EX(entity, "Invalid entity");
        DEBUG_CHECK_RETURN_EX(entity->world() == this, "Entity not attached to world");

        EntityNode* node = nullptr;
        m_entitiesMap.find(entity, node);
        DEBUG_CHECK_RETURN_EX(node, "Entity not properly attached");

        // find and link to transform parent
        if (node->transformHierarchy.parent)
        {
            UnlinkTransformList(node->transformHierarchy);
        }
        else
        {
            DEBUG_CHECK_EX(m_rootTransformEntities.contains(node), "Entity not in root transform list");
            m_rootTransformEntities.remove(node);
        }

        // find and link to update parent
        if (node->updateHierarchy.parent)
        {

        }
        else
        {
            DEBUG_CHECK_EX(m_rootUpdateEntities.contains(node), "Entity not in root update list");
            m_rootUpdateEntities.remove(node);
            m_rootUpdateEntitiesActiveList.remove(node);
        }

        // unlink from static map
        {
            auto lock = CreateLock(m_entitiesIDMappingLock);

            if (node->staticId)
            {
                DEBUG_CHECK_EX(node == m_entityStaticIDMap.findSafe(node->staticId, nullptr), "Invalid node found under static ID");
                m_entityStaticIDMap.remove(node->staticId);
                node->staticId = EntityStaticID();
            }

            if (node->runtimeId)
            {
                DEBUG_CHECK_EX(node == m_entityRuntimeIDMap.findSafe(node->runtimeId, nullptr), "Invalid node found under runtime ID");
                m_entityRuntimeIDMap.remove(node->runtimeId);
                node->runtimeId = EntityRuntimeID();
            }
        }

        // unlink entity and local data
        node->name = StringID();
        node->ptr = nullptr;
        m_entitiesMap.remove(entity);
        m_entitiesPool.free(node);
        m_numEntitiesDetached += 1;

        // if we are during update phase make sure entity gets ticked
        if (m_trackEntitiesAttachedDuringUpdate)
            m_entitiesAttachedDuringUpdate.remove(node);
    }

    // inform entity
    // NOTE: must be done last in case entity detaches...
    entity->detachFromWorld(this);
}

//--

void World::scheduleEntityForTransformUpdate(Entity* entity, bool local, const Transform& newTransform)
{
    auto lock = CreateLock(m_entitiesTransformUpdateLock);

    // create pending request
    EntityTransformUpdateRequest* req = nullptr;
    if (!m_entitiesTransformRequests.find(entity, req))
    {
        req = m_entitiesTransformRequestPool.create();
        req->ptr = AddRef(entity);
        m_entitiesTransformRequests[entity] = req;
    }

    // setup request data
    req->localUpdate = local;
    req->transform = newTransform;
}

void World::calculateDryLocalToWorldTransform_NoLock(const EntityNode* node, Transform& outLocalToWorld) const
{
    DEBUG_CHECK_RETURN_EX(node, "Invalid node");

    outLocalToWorld = node->ptr->transform();

    node = node->transformHierarchy.parent;
    while (node)
    {
        outLocalToWorld = outLocalToWorld * node->ptr->transform();
        node = node->transformHierarchy.parent;
    }
}

//--

void World::renderViewport(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, const WorldRenderingContext& context)
{
    if (auto rs = system<WorldRenderingSystem>())
        rs->renderViewport(this, cmd, output, context);
}

void World::renderDebugFragments(rendering::FrameParams& info)
{
    PC_SCOPE_LVL1(RenderWorldFrame);

    for (auto& param : m_parameters)
        param->handleDebugRender(this, info);

    for (auto &sys : m_systems)
        sys->handleDebugRender(info);

    for (auto& ent : m_entitiesMap.values())
        ent->ptr->handleDebugRender(info);
}

//--

void World::destroySystems()
{
    // shutdown systems
    for (auto i = m_systems.lastValidIndex(); i >= 0; --i)
        m_systems[i]->handleShutdown();

    // close all systems
    for (auto& system : m_systems)
        system.reset();
    m_systems.clear();
}

//--

namespace WorldSystemCreator
{
    struct Info : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_TEMP)

    public:
        SpecificClassType<IWorldSystem> m_class = nullptr;
        Array<Info*> m_dependsOnInit;
        Array<Info*> m_tickBefore;
        Array<Info*> m_tickAfter;

        Array<Info*> m_adj; // temp

        RefPtr<IWorldSystem> m_system;

        int m_depth = 0;

    };

    typedef HashMap<SpecificClassType<IWorldSystem>, Info*> Graph;

    static Info* CreateClassEntry(SpecificClassType<IWorldSystem> classType, Graph& table)
    {
        Info* info = nullptr;
        if (table.find(classType, info))
            return info;

        // map entry
        info = new Info;
        info->m_class = classType;
        table[classType] = info;

        // extract dependency list
        auto classList = classType->findMetadata<DependsOnWorldSystemMetadata>();
        if (classList)
        {
            for (auto refClassType : classList->classes())
                info->m_dependsOnInit.pushBack(CreateClassEntry(refClassType, table));
        }

        // TODO: extract other types of dependencies

        return info;
    }

    void ExtractServiceClasses(Graph& outClasses)
    {
        // enumerate service classes
        Array<SpecificClassType<IWorldSystem>> serviceClasses;
        RTTI::GetInstance().enumClasses(serviceClasses);
        TRACE_INFO("Found {} world system classes", serviceClasses.size());

        // assign indices
        static short numSceneSystems = 0;
        if (0 == numSceneSystems)
        {
            for (auto subSystemClass : serviceClasses)
            {
                subSystemClass->assignUserIndex(numSceneSystems);
                numSceneSystems += 1;
            }
        }

        // create entries
        // NOTE: this may recurse a lot
        outClasses.reserve(serviceClasses.size());
        for (auto serviceClass : serviceClasses)
            CreateClassEntry(serviceClass, outClasses);
    }

    typedef Array<Info*> ServiceOrderTable;

    bool TopologicalSort(const Graph& graph, ServiceOrderTable& outOrder)
    {
        // reset depth
        for (auto service : graph.values())
            service->m_depth = 0;

        // traverse adjacency lists to fill the initial depths
        for (auto service : graph.values())
            for (auto dep : service->m_adj)
                dep->m_depth += 1;

        // create a queue and add all not referenced crap
        Queue<Info*> q;
        for (auto service : graph.values())
            if (service->m_depth == 0)
                q.push(service);

        // process vertices
        outOrder.clear();
        while (!q.empty())
        {
            // pop and add to order
            auto p = q.top();
            q.pop();
            outOrder.pushBack(p);

            // reduce depth counts, if they become zero we are ready to dispatch
            for (auto dep : p->m_adj)
                if (0 == --dep->m_depth)
                    q.push(dep);
        }

        // swap order
        for (uint32_t i = 0; i < outOrder.size() / 2; ++i)
            std::swap(outOrder[i], outOrder[outOrder.size() - 1 - i]);

        // we must have the same count, if not, there's a cycle
        return outOrder.size() == graph.size();
    }

    bool GenerateInitializationOrder(const Graph& graph, ServiceOrderTable& outTable)
    {
        // create the adjacency from the "dependsOn" part
        for (auto service : graph.values())
            service->m_adj = service->m_dependsOnInit;

        // generate topology
        if (!TopologicalSort(graph, outTable))
        {
            TRACE_ERROR("Unable to sort the service nodes");
            //PrintGraph(graph);
            //PrintTopology(graph, outTable);
            return false;
        }

        // print the table
        //TRACE_SPAM("Found order for service initialization:");
        //PrintTopology(graph, outTable);
        return true;
    }

    bool GenerateTickingOrder(const Graph& graph, ServiceOrderTable& outTable)
    {
        // create the adjacency from the "tickBefore"
        for (auto service : graph.values())
            service->m_adj = service->m_tickBefore;

        // create the adjacency from the "tickAfter"
        for (auto service : graph.values())
            for (auto otherService : service->m_tickAfter)
                otherService->m_adj.pushBackUnique(service);

        // generate topology
        if (!TopologicalSort(graph, outTable))
        {
            TRACE_ERROR("Unable to sort the service nodes");
            //PrintGraph(graph);
            //PrintTopology(graph, outTable);
            return false;
        }

        // print the table
        //TRACE_SPAM("Found order for service ticking:");
        //PrintTopology(graph, outTable);
        return true;
    }
};

//--

void World::initializeSystems()
{
    // extract all systems
    WorldSystemCreator::Graph graph;
    WorldSystemCreator::ExtractServiceClasses(graph);

    // generate ticking order
    WorldSystemCreator::ServiceOrderTable initOrder;
    if (!WorldSystemCreator::GenerateInitializationOrder(graph, initOrder))
    {
        FATAL_ERROR("World system initialization graph contains cycles. Initialization impossible.");
    }

    // initialize the services
    memset(m_systemMap, 0, sizeof(m_systemMap));
    for (auto& info : initOrder)
    {
        TRACE_SPAM("Initializing world system '{}'", info->m_class->name());

        // create service
        auto systemPtr = info->m_class.create();
        if (systemPtr->handleInitialize(*this))
        {
            m_systems.pushBack(systemPtr);
            m_systemMap[info->m_class->userIndex()] = systemPtr;
        }
        else
        {
            TRACE_WARNING("World system '{}' failed to initialize", info->m_class->name());
        }
    }
}

//---

void World::initializeParameters()
{
    m_parameters.removeAll(nullptr);

    InplaceArray<SpecificClassType<IWorldParameters>, 10> parameterClasses;
    RTTI::GetInstance().enumClasses(parameterClasses);

    int maxIndex = 0;
    for (const auto cls : parameterClasses)
        maxIndex = std::max<int>(maxIndex, cls->userIndex());

    memzero(m_parametersMap, sizeof(m_parametersMap));

    for (const auto cls : parameterClasses)
    {
        WorldParametersPtr existingParams;
        for (const auto& param : m_parameters)
        {
            if (param->cls() == cls)
            {
                existingParams = param;
                break;
            }
        }

        if (existingParams)
        {
            existingParams = cls->create<IWorldParameters>();
            existingParams->parent(this);
            m_parameters.pushBack(existingParams);
        }

        if (cls->userIndex() == INDEX_NONE)
            cls->assignUserIndex(++maxIndex);

        ASSERT(!m_parametersMap[cls->userIndex()]);
        m_parametersMap[cls->userIndex()] = existingParams;
    }
}

//---

EntityPtr World::findEntityByStaticID(EntityStaticID id) const
{
    if (!id)
        return nullptr;

    auto lock = CreateLock(m_entitiesIDMappingLock);
    
    const EntityNode* node = nullptr;
    if (m_entityStaticIDMap.find(id, node))
        return node->ptr;

    return nullptr;
}

EntityPtr World::findEntityByStaticPath(StringView staticPath) const
{
    if (!staticPath)
        return nullptr;

    DEBUG_CHECK_RETURN_EX_V(staticPath.beginsWith("/"), "Static path should begin with /", nullptr);
    const auto id = EntityStaticIDBuilder::CompileFromPath(staticPath);

    return findEntityByStaticID(id);
}

EntityPtr World::findEntityByRuntimeID(EntityRuntimeID id) const
{
    if (!id)
        return nullptr;

    auto lock = CreateLock(m_entitiesIDMappingLock);

    const EntityNode* node = nullptr;
    if (m_entityRuntimeIDMap.find(id, node))
        return node->ptr;

    return nullptr;
}

EntityPtr World::findEntityByPath(const Entity* relativeTo, StringView path) const
{
    return nullptr;
}

//---

static ConfigProperty<bool> cvDebugPageWorldStats("DebugPage.World.Stats", "IsVisible", false);

static void RenderWorldTickStats(const char* prefix, const WorldTickStats& stats)
{
    ImGui::Text("%s: system time: %u [us]", prefix, (int)(stats.systemTine * 1000000.0));
    ImGui::Separator();

    ImGui::Text("%s: %u tick jobs (%u root)", prefix, stats.numUpdateTotalJobs, stats.numUpdateRootJobs);
    ImGui::Text("%s: %u fixup tick jobs (%u loops)", prefix, stats.numUpdateFixupJobs, stats.numUpdateFixupPasses);
    ImGui::Text("%s: tick time: %u [us]", prefix, (int)(stats.updateTime * 1000000.0));
    ImGui::Separator();

    ImGui::Text("%s: %u transform jobs (%u root)", prefix, stats.numTransformTotalJobs, stats.numTransformRootJobs);
    ImGui::Text("%s: transform time: %u [us]", prefix, (int)(stats.transformTime * 1000000.0));

    ImGui::Separator();
}

static void RenderWorldStats(const WorldStats& stats)
{
    ImGui::Text("Total Update [us]: %d", (int)(stats.totalTime * 1000000.0));
    ImGui::Text("System time: %u [us]", (int)(stats.systemTime * 1000000.0));

    ImGui::Separator();

    ImGui::Text("Entities: %d", stats.numEntites);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "  (+%u)", stats.numEntitiesAttached);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "  (-%u)", stats.numEntitiesDetached);

    ImGui::Separator();

    RenderWorldTickStats("PreTick", stats.preTick);
    RenderWorldTickStats("PostTick", stats.preTick);
}

void World::renderDebugGui()
{
    if (cvDebugPageWorldStats.get() && ImGui::Begin("World stats"))
    {
        RenderWorldStats(m_stats);
        ImGui::End();
    }

    for (const auto& system : m_systems)
        system->handleImGuiDebugInterface();
}

//---

END_BOOMER_NAMESPACE()


/*EntityPtr World::createPrefabInstance(const Transform& placement, const Prefab* prefab, StringID appearance)
{
    DEBUG_CHECK_RETURN_V(prefab, nullptr);

    const auto& rootNode = prefab->root();
    DEBUG_CHECK_RETURN_V(rootNode, nullptr);

    EntityStaticIDBuilder path;
    if (const auto root = CompileEntityHierarchy(path, rootNode, &placement, true))
    {
        InplaceArray<EntityPtr, 10> allEntities;
        root->collectEntities(allEntities);

        if (!allEntities.empty())
        {
            for (const auto& entity : allEntities)
                attachEntity(entity);

            return allEntities[0];
        }
    }

    TRACE_WARNING("Failed to create entity from prefab '{}'", prefab->loadPath());
    return nullptr;
}*/