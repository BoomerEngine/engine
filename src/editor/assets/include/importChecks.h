/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "core/resource_compiler/include/fingerprint.h"
#include "core/io/include/timestamp.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

DECLARE_UI_EVENT(EVENT_IMPORT_STATUS_CHANGED)

//---

struct AssetStateVisualData
{
    bool imported = false;
    StringBuf tag;
    StringBuf color;
    StringBuf caption;
    StringBuf tooltip;
    StringBuf icon;
};

extern void CompileAssetStateVisualData(ImportExtendedStatusFlags flags, AssetStateVisualData& outState);

//---

/// internal helper class that checks the reimport status of a file
class AssetImportStatusCheck : public IReferencable, public IProgressTracker
{
public:
    AssetImportStatusCheck(StringView depotPath, ui::IElement* owner = nullptr);
    ~AssetImportStatusCheck();

    /// file being checked
    INLINE const StringBuf& depotPath() const { return m_depotPath; }

    //--

    /// get last posted status
    bool status(ImportExtendedStatusFlags& outFlags) const;

    /// cancel request 
    void cancel();

private:
    StringBuf m_depotPath;

    SpinLock m_lock;

    ImportExtendedStatusFlags m_statusFlags;
    std::atomic<uint32_t> m_readyFlag = 0;
    std::atomic<uint32_t> m_cancelFlag = 0;

    ui::ElementWeakPtr m_owner;

    virtual bool checkCancelation() const override final;
    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override final;

    void postStatusChange(ImportExtendedStatusFlags flags);

    CAN_YIELD void runCheck();
};

//---

/// internal helper class that checks ONE SINGLE SOURCE ASSET
class AssetSourceFileCheck : public IReferencable, public IProgressTracker
{
public:
    AssetSourceFileCheck(const StringBuf& sourceAssetPath, const TimeStamp& lastKnownTimestamp, const SourceAssetFingerprint& lastKnownCRC);
    ~AssetSourceFileCheck();

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
    SourceAssetFingerprint m_sourceLastKnownCRC;

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

