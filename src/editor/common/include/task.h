/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "core/app/include/command.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///--

struct EditorTaskProgress
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(EditorTaskProgress);

public:
    StringBuf text;
    uint64_t timestamp = 0;
    uint64_t currentCount = 0;
    uint64_t totalCount = 0;
};

///--

/// background editor task
class EDITOR_COMMON_API IEditorTask : public IReferencable, public IProgressTracker
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IEditorTask);

public:
    IEditorTask(StringView name); // NOTE: we should NOT start any processing in the constructor, only in the start() method
    virtual ~IEditorTask();

    //---

    /// event key for reporting task events to the outside world
    ALWAYS_INLINE GlobalEventKey eventKey() const { return m_eventKey; }

    /// descriptive name of command we are running
    ALWAYS_INLINE const StringBuf& description() const { return m_description; }

    /// was cancellation requested ?
    ALWAYS_INLINE bool canceled() const { return m_cancelRequested.load(); }

    //---

    /// request nice, well behaved cancel of the whole thing
    /// NOTE: job may not respect it and still run till the end :(
    void requestCancel();

    /// query progress info from job, returns last progress update
    void queryProgressInfo(EditorTaskProgress& outInfo) const;

    //---

    /// start the job
    virtual bool start() = 0;

    /// update internal state, return true when finished
    virtual bool update() = 0;

    //--

    /// create/retrieve job details dialog
    virtual ui::ElementPtr fetchDetailsDialog();

    //---

protected:
    // IProgressTracker
    virtual bool checkCancelation() const override;
    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override;

private:
    GlobalEventKey m_eventKey;

    NativeTimePoint m_startTime;
    StringBuf m_description;

    SpinLock m_lastProgressLock;
    EditorTaskProgress m_lastProgress;

    std::atomic<bool> m_cancelRequested;
};

///---

END_BOOMER_NAMESPACE_EX(ed)

