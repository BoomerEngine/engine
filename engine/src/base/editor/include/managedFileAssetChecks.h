/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "managedItem.h"
#include "base/resource_compiler/include/importFileFingerprint.h"

namespace ed
{

    //---

    /// internal helper class that checks the reimport status of a file
    class ManagedFileImportStatusCheck : public base::IReferencable, public base::IProgressTracker
    {
    public:
        ManagedFileImportStatusCheck(const ManagedFile* file);
        ~ManagedFileImportStatusCheck();

        /// file being checked
        INLINE const ManagedFile* file() const { return m_file; }

        //--

        /// get last posted status
        base::res::ImportStatus status();

        /// cancel request 
        void cancel();

    private:
        const ManagedFile* m_file = nullptr;

        base::SpinLock m_lock;
        base::res::ImportStatus m_status = base::res::ImportStatus::Checking;
        std::atomic<uint32_t> m_cancelFlag = 0;

        virtual bool checkCancelation() const override final;
        virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text) override final;

        void postStatusChange(base::res::ImportStatus status);

        CAN_YIELD void runCheck();
    };

    //---

    /// internal helper class that checks ONE SINGLE SOURCE ASSET
    class ManagedFileSourceAssetCheck : public base::IReferencable, public base::IProgressTracker
    {
    public:
        ManagedFileSourceAssetCheck(const StringBuf& sourceAssetPath, const io::TimeStamp& lastKnownTimestamp, const base::res::ImportFileFingerprint& lastKnownCRC);
        ~ManagedFileSourceAssetCheck();

        /// file being checked
        INLINE const StringBuf& sourceAssetPath() const { return m_sourceAssetPath; }

        //--

        /// get last posted status
        base::res::SourceAssetStatus status();

        /// last progress (0-1) of check (usually CRC computation)
        float progress();

        /// cancel request 
        void cancel();

    private:
        base::StringBuf m_sourceAssetPath;
        io::TimeStamp m_sourceLastKnownTimestamp;
        base::res::ImportFileFingerprint m_sourceLastKnownCRC;

        base::SpinLock m_lock;

        base::res::SourceAssetStatus m_status = base::res::SourceAssetStatus::Checking;
        float m_progress = 0.0f;

        std::atomic<uint32_t> m_cancelFlag = 0;

        virtual bool checkCancelation() const override final;
        virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text) override final;

        void postStatusChange(base::res::SourceAssetStatus status);

        CAN_YIELD void runCheck();
    };

    //---

} // editor

