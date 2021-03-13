/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "task.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_STRUCT(EditorTaskProgress);
    RTTI_PROPERTY(text);
    RTTI_PROPERTY(timestamp);
    RTTI_PROPERTY(currentCount);
    RTTI_PROPERTY(totalCount);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IEditorTask);
RTTI_END_TYPE();

IEditorTask::IEditorTask(StringView name)
    : m_description(name)
{
    m_startTime.resetToNow();
    m_lastProgress.text = StringBuf("Processing...");
}

IEditorTask::~IEditorTask()
{}

void IEditorTask::requestCancel()
{
    if (0 == m_cancelRequested.exchange(true))
    {
        TRACE_INFO("Requested cancelation of job '{}'", m_description);
    }
}

ui::ElementPtr IEditorTask::fetchDetailsDialog()
{
    return nullptr;
}

bool IEditorTask::checkCancelation() const
{
    return m_cancelRequested.load();
}

void IEditorTask::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
{
    auto lock = CreateLock(m_lastProgressLock);
    m_lastProgress.text = StringBuf(text);
    m_lastProgress.currentCount = currentCount;
    m_lastProgress.totalCount = totalCount;
    m_lastProgress.timestamp += 1;

    if (IsMainThread())
    {
        DispatchGlobalEvent(eventKey(), EVENT_EDITOR_TASK_PROGRESS, m_lastProgress);
    }
    else
    {
        auto progress = m_lastProgress;
        auto key = m_eventKey;
        RunSync("EditorTaskStatusUpdate") << [key, progress](FIBER_FUNC)
        {
            DispatchGlobalEvent(key, EVENT_EDITOR_TASK_PROGRESS, progress);
        };
    }
}

//--

END_BOOMER_NAMESPACE_EX(ed)
