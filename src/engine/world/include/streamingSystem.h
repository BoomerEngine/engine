/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\streaming #]
***/

#pragma once

#include "system.h"
#include "core/containers/include/bitSet.h"

BEGIN_BOOMER_NAMESPACE()

//--

class StreamingSystem;

//--

// mounted streaming island
class ENGINE_WORLD_API StreamingIslandInfo
{
public:
    uint32_t parentIndex = 0;
    uint32_t index = 0;
    bool alwaysLoaded = false;

    Box streamingBox;
    StreamingIslandPtr data;

    StreamingIslandInfo* parent = nullptr;
    Array<StreamingIslandInfo*> children;

    //--

    StreamingIslandInfo();
};

//--

// world streaming update tasks
class ENGINE_WORLD_API StreamingTask : public IReferencable
{
public:
    StreamingTask(const Array<StreamingObserverInfo>& observers, const Array<StreamingIslandInfo>& islands, const Array<uint32_t>& attachedIslands, const BitSet<>& attachedIslandsMask);

    /// request streaming task to be canceled
    void requestCancel();

    /// process the task, can be called directly (blocking) 
    /// but it should be called on job
    CAN_YIELD void process();

    //--

private:
    Array<StreamingObserverInfo> m_observers;

    Array<uint32_t> m_attachedIslands; // modified
    BitSet<> m_attachedIslandsMask; // modified

    const Array<StreamingIslandInfo>& m_islands;

    Array<uint32_t> m_unloadedIslands;
    Array<uint32_t> m_loadedIslands;
    Array<StreamingIslandInstancePtr> m_loadedIslandsData;

    friend class StreamingSystem;
};

//--

/// world streaming system capable of loading dynamic content from compiled scene
class ENGINE_WORLD_API StreamingSystem : public IWorldSystem
{
    RTTI_DECLARE_VIRTUAL_CLASS(StreamingSystem, IWorldSystem);

public:
    StreamingSystem();
    virtual ~StreamingSystem();

    //--

    /// unbind all loaded content
    void unbindEntities();

    /// attach compiled scene content
    void bindScene(const CompiledScene* scene);

    //--

    /// create streaming update tasks using current observers and other settings
    /// NOTE: may return NULL if there's nothing to stream in/out
    RefPtr<StreamingTask> createStreamingTask(const Array<StreamingObserverInfo>& observers) const;

    /// apply finished streaming update task
    /// first outgoing entities are detached then new entities are attached
    /// TODO: partial "budgeted" attach to minimize hitching
    void applyStreamingTask(const StreamingTask* task);

protected:
    virtual void handleShutdown() override;

    //--

    Array<StreamingIslandInfo> m_islands;
    Array<StreamingIslandInstancePtr> m_islandInstances;

    Array<uint32_t> m_attachedIslands;
    BitSet<> m_attachedIslandsMask;

    //--
};

//--

END_BOOMER_NAMESPACE()
