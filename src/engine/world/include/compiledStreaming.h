/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\streaming #]
***/

#pragma once

#include "world.h"
#include "core/containers/include/bitSet.h"

BEGIN_BOOMER_NAMESPACE()

//--

// mounted streaming island
class ENGINE_WORLD_API CompiledStreamingIslandInfo
{
public:
    uint32_t parentIndex = 0;
    uint32_t index = 0;
    bool alwaysLoaded = false;

    Box streamingBox;
    CompiledStreamingIslandPtr data;

    CompiledStreamingIslandInfo* parent = nullptr;
    Array<CompiledStreamingIslandInfo*> children;

    //--

    CompiledStreamingIslandInfo();
};

//--

// world streaming update tasks
class ENGINE_WORLD_API CompiledStreamingTask : public IWorldStreamingTask
{
    RTTI_DECLARE_VIRTUAL_CLASS(CompiledStreamingTask, IWorldStreamingTask);

public:
    CompiledStreamingTask(const CompiledWorldStreaming* data, const Array<WorldObserverInfo>& observers, const Array<CompiledStreamingIslandInfo>& islands, const Array<uint32_t>& attachedIslands, const BitSet<>& attachedIslandsMask);

    /// did we finish ?
    virtual bool finished() const override final { return m_finished; }

    /// request streaming task to be canceled
    virtual void requestCancel() override;

    /// process the task, can be called directly (blocking) 
    /// but it should be called on job
    virtual CAN_YIELD void process() override;

    //--

private:
    RefPtr<CompiledWorldStreaming> m_data;

    Array<WorldObserverInfo> m_observers;

    Array<uint32_t> m_attachedIslands; // modified
    BitSet<> m_attachedIslandsMask; // modified

    const Array<CompiledStreamingIslandInfo>& m_islands;

    Array<uint32_t> m_unloadedIslands;
    Array<uint32_t> m_loadedIslands;
    Array<CompiledStreamingIslandInstancePtr> m_loadedIslandsData;

    std::atomic<bool> m_finished;

    friend class CompiledWorldStreaming;
};

//--

/// world streaming system capable of loading dynamic content from compiled scene
class ENGINE_WORLD_API CompiledWorldStreaming : public IWorldStreaming
{
    RTTI_DECLARE_VIRTUAL_CLASS(CompiledWorldStreaming, IWorldStreaming);

public:
    CompiledWorldStreaming(const CompiledWorldData* scene);
    virtual ~CompiledWorldStreaming();

    //--

    /// unbind all loaded content
    void unbindEntities();

    //--

    /// create streaming update tasks using current observers and other settings
    /// NOTE: may return NULL if there's nothing to stream in/out
    virtual WorldStreamingTaskPtr createStreamingTask(const Array<WorldObserverInfo>& observers) const override;

    /// apply finished streaming update task
    /// first outgoing entities are detached then new entities are attached
    /// TODO: partial "budgeted" attach to minimize hitching
    virtual void applyStreamingTask(World* world, const IWorldStreamingTask* task) override;

protected:
    CompiledWorldDataPtr m_world;

    Array<CompiledStreamingIslandInfo> m_islands;
    Array<CompiledStreamingIslandInstancePtr> m_islandInstances;

    Array<uint32_t> m_attachedIslands;
    BitSet<> m_attachedIslandsMask;
};

//--

END_BOOMER_NAMESPACE()
