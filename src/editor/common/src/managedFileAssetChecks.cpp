/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedFile.h"
#include "managedFileFormat.h"
#include "managedFileAssetChecks.h"
#include "managedFileNativeResource.h"

#include "core/resource_compiler/include/importFileService.h"
#include "core/resource/include/metadata.h"
#include "core/resource_compiler/include/importer.h"
#include "core/resource_compiler/include/importSourceAssetRepository.h"
#include "engine/ui/include/uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

ManagedFileImportStatusCheck::ManagedFileImportStatusCheck(const ManagedFileNativeResource* file, ui::IElement* owner)
    : m_file(file)
    , m_status(ImportStatus::Checking)
    , m_owner(owner)
{
    if (m_file->fileFormat().nativeResourceClass())
    {
        auto selfRef = RefWeakPtr<ManagedFileImportStatusCheck>(this);
        RunFiber("ManagedFileImportStatusCheck") << [selfRef](FIBER_FUNC)
        {
            if (auto self = selfRef.lock())
                self->runCheck();
        };
    }
    else
    {
        m_status = ImportStatus::NotImportable;
    }
}

ManagedFileImportStatusCheck::~ManagedFileImportStatusCheck()
{}

ImportStatus ManagedFileImportStatusCheck::status()
{
    auto lock = CreateLock(m_lock);
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

void ManagedFileImportStatusCheck::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
{
    // TODO: we can utilize this somehow
}

void ManagedFileImportStatusCheck::postStatusChange(ImportStatus status)
{
    bool statusChanged = false;

    {
        auto lock = CreateLock(m_lock);
        if (status != m_status)
        {
            statusChanged = true;
            m_status = status;
        }
    }

    if (statusChanged && m_owner)
    {
        auto ownerRef = m_owner;
        RunSync("ManagedFileImportStatusUpdate") << [ownerRef](FIBER_FUNC)
        {
            if (auto owner = ownerRef.lock())
                owner->call(EVENT_IMPORT_STATUS_CHANGED);
        };
    }
}

void ManagedFileImportStatusCheck::runCheck()
{
    DEBUG_CHECK_EX(m_file->fileFormat().nativeResourceClass(), "Non importable file");

    // load the metadata from file
    auto metadata = m_file->loadMetadata();
    if (!metadata)
    {
        postStatusChange(ImportStatus::NotImportable); // TODO: different error code ?
        return;
    }

    // do we even have a single source file ? if not than this file was never imported but manually created
    if (metadata->importDependencies.empty())
    {
        postStatusChange(ImportStatus::UpToDate); // TODO: different error code ?
        return;
    }

    // check file status
    {
        auto* assetSource = GetService<ImportFileService>();
        static SourceAssetRepository repository(assetSource);
        static Importer localImporter(&repository, nullptr); // TODO: cleanup!

        const auto status = localImporter.checkStatus(m_file->depotPath(), *metadata, nullptr, this);
        postStatusChange(status);
    }

    //--
}

//--

ManagedFileSourceAssetCheck::ManagedFileSourceAssetCheck(const StringBuf& sourceAssetPath, const TimeStamp& lastKnownTimestamp, const ImportFileFingerprint& lastKnownCRC)
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

SourceAssetStatus ManagedFileSourceAssetCheck::status()
{
    auto lock = CreateLock(m_lock);
    return m_status;
}

float ManagedFileSourceAssetCheck::progress()
{
    auto lock = CreateLock(m_lock);
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

void ManagedFileSourceAssetCheck::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
{
    double progress = (double)currentCount / (double)(totalCount ? totalCount : 1);

    auto lock = CreateLock(m_lock);
    m_progress = (float)progress;
}

void ManagedFileSourceAssetCheck::postStatusChange(SourceAssetStatus status)
{
    auto lock = CreateLock(m_lock);
    if (m_status != status)
    {
        m_status = status;
    }
}

void ManagedFileSourceAssetCheck::runCheck()
{
    const auto ret = GetService<ImportFileService>()->checkFileStatus(m_sourceAssetPath, m_sourceLastKnownTimestamp, m_sourceLastKnownCRC, this);
    postStatusChange(ret);
}

//--

END_BOOMER_NAMESPACE_EX(ed)

