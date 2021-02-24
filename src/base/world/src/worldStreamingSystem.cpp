/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\streaming #]
*
***/

#include "build.h"
#include "worldStreamingIsland.h"
#include "worldStreamingSystem.h"
#include "worldCompiledScene.h"

#include "base/containers/include/bitSet.h"

BEGIN_BOOMER_NAMESPACE(base::world)

///---

RTTI_BEGIN_TYPE_CLASS(StreamingObserverInfo);
    RTTI_PROPERTY(position);
    RTTI_PROPERTY(velocity);
RTTI_END_TYPE();

StreamingObserverInfo::StreamingObserverInfo()
{}

///---

StreamingIslandInfo::StreamingIslandInfo()
{}
            
///---

StreamingTask::StreamingTask(const Array<StreamingObserverInfo>& observers, const Array<StreamingIslandInfo>& islands, const Array<uint32_t>& attachedIslands, const BitSet<>& attachedIslandsMask)
    : m_observers(observers)
    , m_attachedIslands(attachedIslands)
    , m_attachedIslandsMask(attachedIslandsMask)
    , m_islands(islands)
{}

void StreamingTask::requestCancel()
{

}

static bool CheckStreamingRange(const Array<StreamingObserverInfo>& observers, const Box& box)
{
    for (const auto& observer : observers)
        if (box.contains(observer.position))
            return true;

    return false;
}

void StreamingTask::process()
{
    PC_SCOPE_LVL0(WorldStreamingTask);

    // determine which islands are within streaming range
    // TODO: split on fibers based on observer count + number of islands
    Array<uint32_t> islandsInRange;
    BitSet islandsInRangeMask;
    {
        PC_SCOPE_LVL0(FindIslandsInRange);
        islandsInRangeMask.resizeWithZeros(m_islands.size());
        islandsInRange.reserve(m_islands.size() / 4); // experimental

        // TODO: extend the streaming box based on the projected velocity

        const auto* ptr = m_islands.typedData();
        const auto* ptrEnd = ptr + m_islands.size();
        uint32_t index = 0;
        while (ptr < ptrEnd)
        {
            if (!islandsInRangeMask[index])
            {
                if (ptr->alwaysLoaded || CheckStreamingRange(m_observers, ptr->streamingBox))
                {
                    islandsInRangeMask.set(index);
                    islandsInRange.pushBack(index);
                }
            }

            ++ptr;
            ++index;
        }
    }

    // any of the currently loaded islands that is not in range should be unloaded
    // NOTE: this does not have to be respected
    {
        PC_SCOPE_LVL0(FindIslandsToUnload);

        bool hadUnloadedIslands = false;

        for (auto& index : m_attachedIslands)
        {
            if (!islandsInRangeMask[index])
            {
                DEBUG_CHECK_EX(m_attachedIslandsMask[index], "Not marked as loaded");
                m_attachedIslandsMask.clear(index);

                m_unloadedIslands.pushBack(index);

                // mark index as removed
                hadUnloadedIslands = true;
                index = INDEX_MAX;
            }
        }

        // TODO: optimize
        if (hadUnloadedIslands)
            m_attachedIslands.removeAll(INDEX_MAX);
    }

    // load new islands
    // TODO: cancelation
    // TODO: time budgeting (ie. 100ms etc of loading)
    {
        PC_SCOPE_LVL0(LoadIslands);

        for (const auto index : islandsInRange)
        {
            const auto& islandInfo = m_islands.typedData()[index];

            if (!m_attachedIslandsMask[index]) // island is not currently attached
            {
                // make sure parent island is properly attached - without it we can't load the inner data
                if (islandInfo.parent)
                    if (!m_attachedIslandsMask[islandInfo.parentIndex])
                        continue;

                // load the island data
                ScopeTimer islandLoadTimer;
                if (const auto instance = islandInfo.data->load(GlobalLoader()))
                {
                    TRACE_INFO("Instanced island '{}', {} entitie(s) in {}", index, instance->size(), islandLoadTimer);

                    // island was loaded properly, add it to the list attached islands
                    m_attachedIslandsMask.set(index);
                    m_attachedIslands.pushBack(index);

                    // store data
                    m_loadedIslands.pushBack(index);
                    m_loadedIslandsData.pushBack(instance);
                }
            }
        }
    }
}

///---

RTTI_BEGIN_TYPE_CLASS(StreamingSystem);
RTTI_END_TYPE();

StreamingSystem::StreamingSystem()
{
}

StreamingSystem::~StreamingSystem()
{}

void StreamingSystem::unbindEntities()
{
    PC_SCOPE_LVL0(WorldUnstreamAll);

    for (auto i : m_attachedIslands.indexRange().reversed())
    {
        auto index = m_attachedIslands[i];
        DEBUG_CHECK_EX(m_attachedIslandsMask[index], "Island not marked as attached");
        if (m_attachedIslandsMask[index])
        {
            if (auto data = m_islandInstances[index])
            {
                m_islandInstances[index] = nullptr;
                //DEBUG_CHECK(!m_islandInstances.contains(data));
                //data->detach(world());
            }

            m_attachedIslandsMask.clear(index);
        }
    }

    m_attachedIslandsMask.clearAll();
    m_attachedIslands.clear();
}

void StreamingSystem::handleShutdown()
{
    TBaseClass::handleShutdown();
    unbindEntities();
}

static uint32_t CountIslands(StreamingIsland* island)
{
    uint32_t ret = 1;
    for (const auto& child : island->children())
        ret += CountIslands(child);
    return ret;
}

static uint32_t CountTotalIslands(const CompiledScene* scene)
{
    uint32_t ret = 0;

    for (const auto& child : scene->rootIslands())
        ret += CountIslands(child);

    return ret;
        
}

static void ExtractIslands(StreamingIslandInfo* extarctedIslands, uint32_t& index, StreamingIslandInfo* parent, const StreamingIsland* data)
{
    auto& entry = extarctedIslands[index];
    entry.index = index++;
    entry.parent = parent;
    entry.parentIndex = parent ? parent->index : 0;
    entry.data = AddRef(data);

    entry.streamingBox = data->streamingBounds();

    if (entry.streamingBox.size().length() < 70.0f)
    {
        const auto center = entry.streamingBox.center();
        entry.streamingBox = Box(center, 70.0f);
    }

    for (const auto& child : data->children())
        ExtractIslands(extarctedIslands, index, &entry, child);
}

void StreamingSystem::bindScene(const CompiledScene* scene)
{
    // detach all entities
    unbindEntities();

    // reset data structures
    m_islands.clear();
    m_islandInstances.clear();
    m_attachedIslands.clear();
    m_attachedIslandsMask.clear();

    // bind new data
    if (scene)
    {
        const auto numIslands = CountTotalIslands(scene);
        m_islands.resize(numIslands);
        m_islandInstances.resize(numIslands);
        m_attachedIslands.reserve(numIslands);
        m_attachedIslandsMask.resizeWithZeros(numIslands);

        uint32_t index = 0;
        for (const auto& rootIsland : scene->rootIslands())
            ExtractIslands(m_islands.typedData(), index, nullptr, rootIsland);
    }
}

RefPtr<StreamingTask> StreamingSystem::createStreamingTask(const Array<StreamingObserverInfo>& observers) const
{
    return RefNew<StreamingTask>(observers, m_islands, m_attachedIslands, m_attachedIslandsMask);
}

void StreamingSystem::applyStreamingTask(const StreamingTask* task)
{
    PC_SCOPE_LVL0(ApplyStreamingTask);

    // TODO: time budgets for detach/attach
    // TODO: flags like "don't unload"

    // unload/detach first
    // NOTE: this might release resources
    uint32_t numDetachedIslands = 0;
    {
        PC_SCOPE_LVL1(DetachIslands);

        // NOTE: reverse order!
        for (auto i : task->m_unloadedIslands.indexRange().reversed())
        {
            auto index = task->m_unloadedIslands[i];
            DEBUG_CHECK_EX(m_attachedIslandsMask[index], "Island not marked as attached");

            if (auto data = m_islandInstances[index])
            {
                data->detach(world());
                m_islandInstances[index] = nullptr;
                //DEBUG_CHECK(!m_islandInstances.contains(data));
            }

            m_attachedIslandsMask.clear(index);
            numDetachedIslands += 1;
        }
    }

    // attach newcomers
    uint32_t numAttachedIslands = 0;
    {
        PC_SCOPE_LVL1(AttachIslands);

        for (auto i : task->m_loadedIslands.indexRange())
        {
            auto index = task->m_loadedIslands[i];
            auto data = task->m_loadedIslandsData[i];

            DEBUG_CHECK_EX(!m_attachedIslandsMask[index], "Island marked as attached");
            DEBUG_CHECK_EX(!m_islandInstances[index], "Island already has data");

            if (!m_islandInstances[index])
            {
                //DEBUG_CHECK(!m_islandInstances.contains(data));
                m_islandInstances[index] = data;
                data->attach(world());
            }

            m_attachedIslandsMask.set(index);
            numAttachedIslands += 1;
        }
    }

    // update internal state
    m_attachedIslands = task->m_attachedIslands;
    m_attachedIslandsMask = task->m_attachedIslandsMask;       

    // stats
    if (numAttachedIslands || numDetachedIslands)
    {
        TRACE_INFO("Streaming: attached {}, detached {} (current {}) islands", numAttachedIslands, numDetachedIslands, m_attachedIslands.size());
    }
}

//--

END_BOOMER_NAMESPACE(base::world)
