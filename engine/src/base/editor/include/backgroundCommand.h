/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: command #]
***/

#pragma once

#include "base/app/include/command.h"

namespace ed
{
    
    ///---

    enum class BackgroundJobStatus : uint8_t
    {
        Finished,
        Failed,
        Running,
    };

    ///--

    struct BackgroundJobProgress
    {
        base::StringBuf text;
        uint64_t timestamp = 0;
        uint64_t currentCount = 0;
        uint64_t totalCount = 0;
    };

    ///--

    /// editor side host that runs the command
    class BASE_EDITOR_API IBackgroundJob : public IReferencable, public base::IProgressTracker
    {
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IBackgroundJob);

    public:
        IBackgroundJob(StringView name); // NOTE: we should NOT start any processing in the constructor, only in the start() method
        virtual ~IBackgroundJob();

        //---

        /// descriptive name of command we are running
        ALWAYS_INLINE const StringBuf& description() const { return m_description; }

        /// was cancellation requested ?
        ALWAYS_INLINE bool canceled() const { return m_cancelRequested.load(); }

        //---

        /// request nice, well behaved cancel of the whole thing
        /// NOTE: job may not respect it and still run till the end :(
        void requestCancel();

        /// query progress info from job, returns last progress update
        void queryProgressInfo(BackgroundJobProgress& outInfo) const;

        //---

        /// start the job
        virtual bool start() = 0;

        /// update internal state
        /// NOTE: this function is called every frame but should not do any heavy lifting (all actual work should be done on fibers/threads)
        virtual BackgroundJobStatus update() = 0;

        //--

        /// create/retrieve job details dialog
        virtual ui::ElementPtr fetchDetailsDialog();

        /// create/retrieve job small status dialog
        virtual ui::ElementPtr fetchStatusDialog();

    protected:
        // IProgressTracker
        virtual bool checkCancelation() const override final;
        virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override final;

    private:
        NativeTimePoint m_startTime;
        StringBuf m_description;

        BackgroundJobProgress m_lastProgress;
        SpinLock m_lastProgressLock;

        std::atomic<bool> m_cancelRequested;
    };

    ///---

} // editor

