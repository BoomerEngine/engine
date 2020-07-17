/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"
#include "managedFileAssetChecks.h"
#include "base/resource_compiler/include/importFileService.h"

namespace ed
{
    //---

    ManagedFileImportStatusCheck::ManagedFileImportStatusCheck(const ManagedFile* file)
        : m_file(file)
        , m_status(base::res::ImportStatus::Checking)
    {
        auto selfRef = ManagedFileImportStatusCheckPtr(AddRef(this));
        RunFiber("ManagedFileImportStatusCheck") << [selfRef](FIBER_FUNC)
        {
            selfRef->runCheck();
        };
    }

    ManagedFileImportStatusCheck::~ManagedFileImportStatusCheck()
    {}

    base::res::ImportStatus ManagedFileImportStatusCheck::status()
    {
        auto lock = base::CreateLock(m_lock);
        return m_status;
    }

    void ManagedFileImportStatusCheck::cancel()
    {
        m_cancelFlag.exchange(1);
    }

    bool ManagedFileImportStatusCheck::checkCancelation() const
    {
        return m_cancelFlag.load();
    }

    void ManagedFileImportStatusCheck::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text)
    {
        // TODO: we can utilize this somehow
    }

    void ManagedFileImportStatusCheck::postStatusChange(base::res::ImportStatus status)
    {
        auto lock = base::CreateLock(m_lock);
        m_status = status;
    }

    void ManagedFileImportStatusCheck::runCheck()
    {

    }

    //--

    ManagedFileSourceAssetCheck::ManagedFileSourceAssetCheck(const StringBuf& sourceAssetPath, const io::TimeStamp& lastKnownTimestamp, const base::res::ImportFileFingerprint& lastKnownCRC)
        : m_sourceAssetPath(sourceAssetPath)
        , m_sourceLastKnownTimestamp(lastKnownTimestamp)
        , m_sourceLastKnownCRC(lastKnownCRC)
    {
        auto selfRef = RefPtr< ManagedFileSourceAssetCheck>(AddRef(this));
        RunFiber("ManagedFileSourceAssetCheck") << [selfRef](FIBER_FUNC)
        {
            selfRef->runCheck();
        };
    }

    ManagedFileSourceAssetCheck::~ManagedFileSourceAssetCheck()
    {}

    base::res::SourceAssetStatus ManagedFileSourceAssetCheck::status()
    {
        auto lock = base::CreateLock(m_lock);
        return m_status;
    }

    float ManagedFileSourceAssetCheck::progress()
    {
        auto lock = base::CreateLock(m_lock);
        return m_progress;
    }

    void ManagedFileSourceAssetCheck::cancel()
    {
        m_cancelFlag.exchange(1);
    }

    bool ManagedFileSourceAssetCheck::checkCancelation() const
    {
        return m_cancelFlag.load();
    }

    void ManagedFileSourceAssetCheck::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView<char> text)
    {
        double progress = (double)currentCount / (double)(totalCount ? totalCount : 1);

        auto lock = base::CreateLock(m_lock);
        m_progress = (float)progress;
    }

    void ManagedFileSourceAssetCheck::postStatusChange(base::res::SourceAssetStatus status)
    {
        auto lock = base::CreateLock(m_lock);
        if (m_status != status)
        {
            m_status = status;
        }
    }

    void ManagedFileSourceAssetCheck::runCheck()
    {
        const auto ret = base::GetService<base::res::ImportFileService>()->checkFileStatus(m_sourceAssetPath, m_sourceLastKnownTimestamp, m_sourceLastKnownCRC, this);
        postStatusChange(ret);
    }

    //--

} // editor

