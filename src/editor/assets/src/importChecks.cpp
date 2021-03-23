/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "importChecks.h"

#include "core/resource/include/metadata.h"
#include "core/resource/include/depot.h"
#include "core/resource_compiler/include/importer.h"
#include "core/resource_compiler/include/sourceAssetService.h"
#include "core/containers/include/path.h"

#include "engine/ui/include/uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

AssetImportStatusCheck::AssetImportStatusCheck(StringView depotPath, ui::IElement* owner)
    : m_owner(owner)
{
    m_depotPath = ReplaceExtension(depotPath, ResourceMetadata::FILE_EXTENSION);

    auto selfRef = RefWeakPtr<AssetImportStatusCheck>(this);
    RunFiber("AssetImportStatusCheck") << [selfRef](FIBER_FUNC)
    {
        if (auto self = selfRef.lock())
            self->runCheck();
    };
}

AssetImportStatusCheck::~AssetImportStatusCheck()
{}

bool AssetImportStatusCheck::status(ImportExtendedStatusFlags& outFlags) const
{
    auto lock = CreateLock(m_lock);

    if (m_cancelFlag)
    {
        outFlags |= ImportExtendedStatusBit::Canceled;
        return true;
    }

    if (m_readyFlag)
    {
        outFlags = m_statusFlags;
        return true;
    }

    return false;
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

void AssetImportStatusCheck::postStatusChange(ImportExtendedStatusFlags flags)
{
    {
        auto lock = CreateLock(m_lock);
        m_statusFlags = flags;
        m_readyFlag = 1;
    }

    if (m_owner)
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
    auto metadata = GetService<DepotService>()->loadFileToXMLObject<ResourceMetadata>(m_depotPath);

    {
        static Importer localImporter;

        ImportExtendedStatusFlags flags;
        localImporter.checkStatus(flags, metadata);
        postStatusChange(flags);
    }
}

//--

AssetSourceFileCheck::AssetSourceFileCheck(const StringBuf& sourceAssetPath, const TimeStamp& lastKnownTimestamp, const SourceAssetFingerprint& lastKnownCRC)
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
