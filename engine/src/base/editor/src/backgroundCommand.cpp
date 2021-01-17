/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: command #]
***/

#include "build.h"
#include "editorService.h"
#include "backgroundCommand.h"

namespace ed
{

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IBackgroundJob);
    RTTI_END_TYPE();

    IBackgroundJob::IBackgroundJob(StringView name)
        : m_description(name)
    {
        m_startTime.resetToNow();

        m_lastProgress.text = StringBuf("Processing...");
    }

    IBackgroundJob::~IBackgroundJob()
    {}

    void IBackgroundJob::requestCancel()
    {
        if (0 == m_cancelRequested.exchange(true))
        {
            TRACE_INFO("Requested cancelation of job '{}'", m_description);
        }
    }

    ui::ElementPtr IBackgroundJob::fetchDetailsDialog()
    {
        return nullptr;
    }

    ui::ElementPtr IBackgroundJob::fetchStatusDialog()
    {
        return nullptr;
    }

    void IBackgroundJob::queryProgressInfo(BackgroundJobProgress& outInfo) const
    {
        auto lock = CreateLock(m_lastProgressLock);
        outInfo = m_lastProgress;
    }

    bool IBackgroundJob::checkCancelation() const
    {
        return m_cancelRequested.load();
    }

    void IBackgroundJob::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
    {
        auto lock = CreateLock(m_lastProgressLock);
        m_lastProgress.text = StringBuf(text);
        m_lastProgress.currentCount = currentCount;
        m_lastProgress.totalCount = totalCount;
        m_lastProgress.timestamp += 1;
    }

    //--

} // editor
