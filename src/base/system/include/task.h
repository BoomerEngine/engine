/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once
#include "spinLock.h"

BEGIN_BOOMER_NAMESPACE(base)

/// task tracking utility
class BASE_SYSTEM_API Task : public base::NoCopy
{
public:
    Task(Task* knownParent, uint32_t numParts, const char* name);
    Task(uint32_t numParts, const char* name);
    virtual ~Task();

    // advance to next part (updates the progress)
    void advance(uint32_t count = 1);

    //--

    // detach current task block from current thread
    static Task* DetachTask();

    // attach task block to current thread
    static void AttachTask(Task* task);

protected:
    Task* m_parent;
    Task* m_prev;
    const uint32_t m_numParts;
    bool m_isMute;
    double m_currentProportion;
    std::atomic<uint32_t> m_numFinishedTasks;
    std::atomic<uint32_t> m_numActiveSubTasks;

    SpinLock m_listLock;
    Task* m_childTasks;
    Task* m_nextTask;
    Task** m_prevTask;

    void beginSubTask(Task* subTask, const char* name, bool& isMute);
    void endSubTask( Task* subTask);

    void recalculateProgress();

    //--

    virtual void notifyBeginTask(Task* caller, const char* desc);
    virtual void notifyEndTask(Task* caller);
    virtual void notifySetProgress(Task* caller, float progress);
};
    
END_BOOMER_NAMESPACE(base)