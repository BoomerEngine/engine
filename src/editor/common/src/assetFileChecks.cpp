/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedFile.h"
#include "assetFormat.h"
#include "assetFileChecks.h"

#include "core/resource_compiler/include/importFileService.h"
#include "core/resource/include/metadata.h"
#include "core/resource_compiler/include/importer.h"
#include "core/resource_compiler/include/importSourceAssetRepository.h"
#include "engine/ui/include/uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

AssetImportStatusCheck::AssetImportStatusCheck(StringView depotPath, ui::IElement* owner)
    : m_depotPath(depotPath)
    , m_status(ImportStatus::Checking)
    , m_owner(owner)
{
    auto selfRef = RefWeakPtr<AssetImportStatusCheck>(this);
    RunFiber("AssetImportStatusCheck") << [selfRef](FIBER_FUNC)
    {
        if (auto self = selfRef.lock())
            self->runCheck();
    };
}

AssetImportStatusCheck::~AssetImportStatusCheck()
{}

ImportStatus AssetImportStatusCheck::status()
{
    auto lock = CreateLock(m_lock);
    return m_status;
}

void AssetImportStatusCheck::cancel()
{
    m_cancelFlag.exchange(1);
}

bool AssetImportStatusCheck::checkCancelation() const
{
    return m_cancelFlag.load();
}

void AssetImportStatusCheck::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
{
    // TODO: we can utilize this somehow
}

void AssetImportStatusCheck::postStatusChange(ImportStatus status)
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

void AssetImportStatusCheck::runCheck()
{
    DEBUG_CHECK_EX(m_file->fileFormat().nativeResourceClass(), "Non importable file");

    // load the metadata from file
    ResourceMetadataPtr metadata;
    //auto metadata = Get m_file->loadMetadata();
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
        auto* assetSource = GetService<SourceAssetService>();
        static SourceAssetRepository repository(assetSource);
        static Importer localImporter(&repository, nullptr); // TODO: cleanup!

        const auto status = localImporter.checkStatus(m_depotPath, *metadata, nullptr, this);
        postStatusChange(status);
    }

    //--
}

//--

AssetSourceFileCheck::AssetSourceFileCheck(const StringBuf& sourceAssetPath, const TimeStamp& lastKnownTimestamp, const ImportFileFingerprint& lastKnownCRC)
    : m_sourceAssetPath(sourceAssetPath)
    , m_sourceLastKnownTimestamp(lastKnownTimestamp)
    , m_sourceLastKnownCRC(lastKnownCRC)
{
    auto selfRef = RefPtr<AssetSourceFileCheck>(AddRef(this));
    RunFiber("AssetSourceFileCheck") << [selfRef](FIBER_FUNC)
    {
        selfRef->runCheck();
    };
}

AssetSourceFileCheck::~AssetSourceFileCheck()
{}

SourceAssetStatus AssetSourceFileCheck::status()
{
    auto lock = CreateLock(m_lock);
    return m_status;
}

float AssetSourceFileCheck::progress()
{
    auto lock = CreateLock(m_lock);
    return m_progress;
}

void AssetSourceFileCheck::cancel()
{
    m_cancelFlag.exchange(1);
}

bool AssetSourceFileCheck::checkCancelation() const
{
    return m_cancelFlag.load();
}

void AssetSourceFileCheck::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
{
    double progress = (double)currentCount / (double)(totalCount ? totalCount : 1);

    auto lock = CreateLock(m_lock);
    m_progress = (float)progress;
}

void AssetSourceFileCheck::postStatusChange(SourceAssetStatus status)
{
    auto lock = CreateLock(m_lock);
    if (m_status != status)
    {
        m_status = status;
    }
}

void AssetSourceFileCheck::runCheck()
{
    const auto ret = GetService<SourceAssetService>()->checkFileStatus(m_sourceAssetPath, m_sourceLastKnownTimestamp, m_sourceLastKnownCRC, this);
    postStatusChange(ret);
}

//--

END_BOOMER_NAMESPACE_EX(ed)

