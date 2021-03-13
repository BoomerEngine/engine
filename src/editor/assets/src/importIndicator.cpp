/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "importChecks.h"
#include "importIndicator.h"

#include "engine/ui/include/uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetFileSmallImportIndicator);
    RTTI_METADATA(ui::ElementClassNameMetadata).name("AssetFileSmallImportIndicator");
RTTI_END_TYPE();

AssetFileSmallImportIndicator::AssetFileSmallImportIndicator(StringView depotPath)
    : m_depotPath(depotPath)
    , m_events(this)
{
    visibility(false);

    m_caption = createChild<ui::TextLabel>("[img:question_red]");

    bind(EVENT_IMPORT_STATUS_CHANGED) = [this]() { updateStatus(); };

    m_checker = RefNew<AssetImportStatusCheck>(m_depotPath, this);

    /*m_events.bind(file->eventKey(), EVENT_MANAGED_FILE_RELOADED) = [this]()
    {
        recheck();
    };*/
}

void AssetFileSmallImportIndicator::recheck()
{
    if (m_checker)
    {
        m_checker->cancel();
        m_checker.reset();
    }

    m_checker = RefNew<AssetImportStatusCheck>(m_depotPath, this);
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

/*        case ImportStatus::Pending: return "[b][color:#888]Pending[/color]";
        case ImportStatus::Processing: return "[b][color:#FF7]Processing...[/color]";
        case ImportStatus::Checking: return "[b][color:#AAA]Checking...[/color]";
        case ImportStatus::Canceled: return "[b][tag:#AAA]Canceled[/tag]";

        case ImportStatus::FinishedUpTodate:
            if (withIcon)
                return "[b][tag:#88F][img:tick] Up to date[/tag]";
            else
                return "[b][tag:#88F]Up to date[/tag]";

        case ImportStatus::FinishedNewContent:
            if (withIcon)
                return "[b][tag:#8F8][img:cog] [color:#000]Imported[/tag]";
            else
                return "[b][tag:#8F8][color:#000]Imported[/tag]";

        case ImportStatus::Failed:
            if (withIcon)
                return "[b][tag:#F88][color:#000][img:skull] Failed[/tag]";
            else
                return "[b][tag:#F88][color:#000]Failed[/tag]";
    }
    */

void CompileAssetStateVisualData(ImportExtendedStatusFlags flags, AssetStateVisualData& outState)
{
    outState = AssetStateVisualData();
    outState.imported = true;

    if (flags.test(ImportExtendedStatusBit::Canceled))
    {
        outState.icon = "question_red";
        outState.color = "[color:#AAA]";
        outState.tag = "[tag:#222]";
        outState.caption = "Canceled";
        outState.tooltip = "Checking was canceled, asset state is unknown";
    }
    else if (flags.test(ImportExtendedStatusBit::MetadataInvalid))
    {
        outState.icon = "skull";
        outState.color = "[color:#F00]";
        outState.tag = "[tag:#000]";
        outState.caption = "Corrupted";
        outState.tooltip = "Metadata for this file is invalid";
    }
    else if (flags.test(ImportExtendedStatusBit::NotImported))
    {
        outState.imported = false;
    }
    else if (flags.test(ImportExtendedStatusBit::UpToDate))
    {
        outState.icon = "tick";
        outState.color = "[color:#000]";
        outState.tag = "[tag:#88F]";
        outState.caption = "UpToDate";
        outState.tooltip = "File is up to date with the source data";
    }
    else
    {
        bool serious = false;
        bool missing = false;

        StringBuilder txt;

        if (flags.test(ImportExtendedStatusBit::SourceFileChanged))
        {
            txt << "Source files have changed\n";
        }
        if (flags.test(ImportExtendedStatusBit::ConfigurationChanged))
        {
            txt << "Import configuration has changed\n";
        }
        if (flags.test(ImportExtendedStatusBit::ImporterClassMissing))
        {
            txt << "Importer is missing\n";
            serious = true;
        }
        if (flags.test(ImportExtendedStatusBit::ImporterVersionChanged))
        {
            txt << "Importer has changed version\n";
            serious = true;
        }
        if (flags.test(ImportExtendedStatusBit::ResourceClassMissing))
        {
            txt << "Resource is no longer supported\n";
            serious = true;
        }
        if (flags.test(ImportExtendedStatusBit::ResourceVersionChanged))
        {
            txt << "Resource binary version is outdated\n";
            serious = true;
        }
        if (flags.test(ImportExtendedStatusBit::SourceFileMissing))
        {
            txt << "Source file is missing\n";
            missing = true;
        }
        if (flags.test(ImportExtendedStatusBit::SourceFileNotReadable))
        {
            txt << "Source file is not readable\n";
            missing = true;
        }

        outState.tooltip = txt.toString();

        if (serious)
        {
            outState.icon = "exclamation";
            outState.color = "[color:#000]";
            outState.tag = "[tag:#C00]";
            outState.caption = "Fail";
        }
        else if (missing)
        {
            outState.icon = "warning";
            outState.color = "[color:#000]";
            outState.tag = "[tag:#C70]";
            outState.caption = "Error";
        }
        else
        {
            outState.icon = "asterisk_yellow";
            outState.color = "[color:#000]";
            outState.tag = "[tag:#CC0]";
            outState.caption = "Changed";
        }
    }
}

void AssetFileSmallImportIndicator::updateStatus()
{
    ImportExtendedStatusFlags flags;
    if (m_checker->status(flags))
    {
        AssetStateVisualData visualState;
        CompileAssetStateVisualData(flags, visualState);

        visibility(visualState.imported);
        tooltip(visualState.tooltip);

        if (visualState.imported)
            m_caption->text(TempString("[img:{}]", visualState.icon));
    }    
}

//--

END_BOOMER_NAMESPACE_EX(ed)
