/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "managedItem.h"
#include "core/resource_compiler/include/importFileFingerprint.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

DECLARE_UI_EVENT(EVENT_IMPORT_STATUS_CHANGED)

//---

/// internal helper class that checks the reimport status of a file
class ManagedFileImportStatusCheck : public IReferencable, public IProgressTracker
{
public:
    ManagedFileImportStatusCheck(const ManagedFileNativeResource* file, ui::IElement* owner = nullptr);
    ~ManagedFileImportStatusCheck();

    /// file being checked
    INLINE const ManagedFileNativeResource* file() const { return m_file; }

    //--

    /// get last posted status
    ImportStatus status();

    /// cancel request 
    void cancel();

private:
    const ManagedFileNativeResource* m_file = nullptr;

    SpinLock m_lock;
    ImportStatus m_status = ImportStatus::Checking;
    std::atomic<uint32_t> m_cancelFlag = 0;

    ui::ElementWeakPtr m_owner;

    virtual bool checkCancelation() const override final;
    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override final;

    void postStatusChange(ImportStatus status);

    CAN_YIELD void runCheck();
};

//---

/// internal helper class that checks ONE SINGLE SOURCE ASSET
class ManagedFileSourceAssetCheck : public IReferencable, public IProgressTracker
{
public:
    ManagedFileSourceAssetCheck(const StringBuf& sourceAssetPath, const TimeStamp& lastKnownTimestamp, const ImportFileFingerprint& lastKnownCRC);
    ~ManagedFileSourceAssetCheck();

    /// file being checked
    INLINE const StringBuf& sourceAssetPath() const { return m_sourceAssetPath; }

    //--

    /// get last posted status
    SourceAssetStatus status();

    /// last progress (0-1) of check (usually CRC computation)
    float progress();

    /// cancel request 
    void cancel();

private:
    StringBuf m_sourceAssetPath;
    TimeStamp m_sourceLastKnownTimestamp;
    ImportFileFingerprint m_sourceLastKnownCRC;

    SpinLock m_lock;

    SourceAssetStatus m_status = SourceAssetStatus::Checking;
    float m_progress = 0.0f;

    std::atomic<uint32_t> m_cancelFlag = 0;

    virtual bool checkCancelation() const override final;
    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override final;

    void postStatusChange(SourceAssetStatus status);

    CAN_YIELD void runCheck();
};

//---

END_BOOMER_NAMESPACE_EX(ed)

