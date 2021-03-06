/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "managedFile.h"
#include "managedFileAssetChecks.h"
#include "managedFileNativeResource.h"
#include "assetFileSmallImportIndicator.h"

#include "engine/ui/include/uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetFileSmallImportIndicator);
    RTTI_METADATA(ui::ElementClassNameMetadata).name("AssetFileSmallImportIndicator");
RTTI_END_TYPE();

AssetFileSmallImportIndicator::AssetFileSmallImportIndicator(ManagedFileNativeResource* file)
    : m_file(file)
    , m_events(this)
{
    visibility(false);

    m_caption = createChild<ui::TextLabel>("[img:question_red]");

    bind(EVENT_IMPORT_STATUS_CHANGED) = [this]() { updateStatus(); };

    m_checker = RefNew<ManagedFileImportStatusCheck>(m_file, this);

    m_events.bind(file->eventKey(), EVENT_MANAGED_FILE_RELOADED) = [this]()
    {
        recheck();
    };
}

void AssetFileSmallImportIndicator::recheck()
{
    if (m_checker)
    {
        m_checker->cancel();
        m_checker.reset();
    }

    m_checker = RefNew<ManagedFileImportStatusCheck>(m_file, this);
}

AssetFileSmallImportIndicator::~AssetFileSmallImportIndicator()
{
    if (m_checker)
    {
        m_checker->cancel();
        m_checker.reset();
    }
}

void AssetFileSmallImportIndicator::showContextMenu()
{
    // TODO
}

void AssetFileSmallImportIndicator::updateStatus()
{
    StringView statusText = "[img:question_red]";

    bool visible = true;

    const auto status = m_checker->status();
    switch (status)
    {
        case ImportStatus::Pending:
        case ImportStatus::Checking:
        case ImportStatus::Canceled:
        case ImportStatus::Processing:
            visible = false;
            tooltip("Unknown file status");
            break;
            
        case ImportStatus::NewAssetImported:
        case ImportStatus::NotUpToDate:
            statusText = "[img:warning]";
            tooltip("File is not up to date, source files have changed");
            break;

        case ImportStatus::MissingAssets:
            statusText = "[img:exclamation]";
            tooltip("Source files are missing");
            break;

        case ImportStatus::NotSupported:
            tooltip("Source format is no longer supported for importing (missing plugins?)");
            statusText = "[img:skull]";
            break;

        case ImportStatus::InvalidAssets:
            tooltip("There's a problem loading source assets for this file (data corruption?)");
            statusText = "[img:skull]";
            break;

        case ImportStatus::UpToDate:
            tooltip("File is up to date with the source data");
            statusText = "[img:tick]";
            break;

        case ImportStatus::NotImportable:
            tooltip("File was not imported from any source data but was generated in-engine");
            statusText = "[img:tick]";
            visible = false;
            break;
    }

    m_caption->text(statusText);
    visibility(visible);
}

//--

END_BOOMER_NAMESPACE_EX(ed)
