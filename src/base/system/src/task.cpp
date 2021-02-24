/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "task.h"

BEGIN_BOOMER_NAMESPACE(base)

//---

static Task* SwapCurrentTask(Task* newTask)
{
    static TYPE_TLS Task* GCurrentTask = nullptr;
    std::swap(GCurrentTask, newTask);
    return newTask;
}

Task::Task(Task* knownParent, uint32_t numParts, const char* name)
    : m_numParts(std::max<uint32_t>(1, numParts))
    , m_numFinishedTasks(0)
    , m_numActiveSubTasks(0)
    , m_currentProportion(0.0)
    , m_childTasks(nullptr)
    , m_nextTask(nullptr)
    , m_prevTask(nullptr)
    , m_isMute(false)
{
    // push task to stack
    m_prev = SwapCurrentTask(this);

    // inform parent that a sub task has started
    m_parent = knownParent ? knownParent : m_prev;
    if (m_parent)
        m_parent->beginSubTask(this, name, m_isMute);
}

Task::Task(uint32_t numParts, const char* name)
    : m_numParts(std::max<uint32_t>(1, numParts))
    , m_numFinishedTasks(0)
    , m_numActiveSubTasks(0)
    , m_currentProportion(0.0)
    , m_childTasks(nullptr)
    , m_nextTask(nullptr)
    , m_prevTask(nullptr)
    , m_isMute(false)
{
    // push task to stack
    m_prev = SwapCurrentTask(this);

    // inform parent that a sub task has started
    m_parent = m_prev;
    if (m_parent)
        m_parent->beginSubTask(this, name, m_isMute);
}

Task::~Task()
{
    // inform parent that the sub task has ended
    if (m_parent)
        m_parent->endSubTask(this);

    // make sure we are not used any more
    ASSERT(m_nextTask == nullptr);
    ASSERT(m_prevTask == nullptr);
    ASSERT(m_childTasks == nullptr);

    // restore thread's stack of tasks
    auto cur = SwapCurrentTask(m_prev);
    ASSERT(cur == this);
}

Task* Task::DetachTask()
{
    return SwapCurrentTask(nullptr);
}

void Task::AttachTask(Task* task)
{
    auto prev = SwapCurrentTask(task);
    ASSERT(prev == nullptr);
}

void Task::beginSubTask(Task* subTask, const char* name, bool& isMute)
{
    if (0 == m_numActiveSubTasks++)
    {
        isMute = false;
        if (!m_isMute)
            notifyBeginTask(this, name);
    }
    else
    {
        isMute = true;
    }

    {
        m_listLock.acquire();

        if (m_childTasks)
            m_childTasks->m_prevTask = &subTask->m_nextTask;
        subTask->m_prevTask = &m_childTasks;
        subTask->m_nextTask = m_childTasks;
        m_childTasks = subTask;

        m_listLock.release();
    }
}

void Task::endSubTask(Task* subTask)
{
    ++m_numFinishedTasks;

    if (0 == --m_numActiveSubTasks)
    {
        if (!m_isMute)
            notifyEndTask(this);
    }

    {
        m_listLock.acquire();

        if (subTask->m_nextTask)
            subTask->m_nextTask->m_prevTask = subTask->m_prevTask;
        *subTask->m_prevTask = subTask->m_nextTask;
        subTask->m_nextTask = nullptr;
        subTask->m_prevTask = nullptr;

        m_listLock.release();
    }

    recalculateProgress();
}

void Task::recalculateProgress()
{
    // get base value
    auto value = (double)m_numFinishedTasks.load();

    // add values from sub tasks
    m_listLock.acquire();
    for (auto task  = m_childTasks; task; task = task->m_nextTask)
        value += task->m_currentProportion;
    m_listLock.release();

    // update proportion
    auto newProportion = std::clamp<double>(value / (double)m_numParts, 0.0, 1.0);
    if (newProportion != m_currentProportion)
    {
        m_currentProportion = newProportion;

        if (m_parent)
        {
            m_parent->recalculateProgress();
        }
        else
        {
            notifySetProgress(this, (float)m_currentProportion);
        }
    }
}

void Task::advance(const uint32_t count/* = 1*/)
{
    m_numFinishedTasks += count;
    recalculateProgress();
}

void Task::notifyBeginTask(Task* caller, const char* desc)
{
    if (m_parent)
        m_parent->notifyBeginTask(caller, desc);
}

void Task::notifyEndTask(Task* caller)
{
    if (m_parent)
        m_parent->notifyEndTask(caller);
}

void Task::notifySetProgress(Task* caller, float progress)
{
    if (m_parent)
        m_parent->notifySetProgress(caller, progress);
}

//---

END_BOOMER_NAMESPACE(base)