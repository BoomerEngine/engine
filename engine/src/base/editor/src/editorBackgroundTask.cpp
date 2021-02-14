/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "editorService.h"
#include "editorBackgroundTask.h"

namespace ed
{

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IBackgroundTask);
    RTTI_END_TYPE();

    IBackgroundTask::IBackgroundTask(StringView name)
        : m_description(name)
    {
        m_startTime.resetToNow();

        m_lastProgress.text = StringBuf("Processing...");
    }

    IBackgroundTask::~IBackgroundTask()
    {}

    void IBackgroundTask::requestCancel()
    {
        if (0 == m_cancelRequested.exchange(true))
        {
            TRACE_INFO("Requested cancelation of job '{}'", m_description);
        }
    }

    ui::ElementPtr IBackgroundTask::fetchDetailsDialog()
    {
        return nullptr;
    }

    ui::ElementPtr IBackgroundTask::fetchStatusDialog()
    {
        return nullptr;
    }

    void IBackgroundTask::queryProgressInfo(BackgroundTaskProgress& outInfo) const
    {
        auto lock = CreateLock(m_lastProgressLock);
        outInfo = m_lastProgress;
    }

    bool IBackgroundTask::checkCancelation() const
    {
        return m_cancelRequested.load();
    }

    void IBackgroundTask::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
    {
        auto lock = CreateLock(m_lastProgressLock);
        m_lastProgress.text = StringBuf(text);
        m_lastProgress.currentCount = currentCount;
        m_lastProgress.totalCount = totalCount;
        m_lastProgress.timestamp += 1;
    }

    //--

} // editor
