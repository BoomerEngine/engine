/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "editorService.h"

#include "assetFileChecks.h"
#include "assetFileImportWidget.h"

#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiImage.h"
#include "engine/ui/include/uiEditBox.h"
#include "engine/ui/include/uiMessageBox.h"
#include "core/resource/include/metadata.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetFileImportWidget);
    RTTI_METADATA(ui::ElementClassNameMetadata).name("AssetFileImportWidget");
RTTI_END_TYPE();

AssetFileImportWidget::AssetFileImportWidget()
{
    layoutVertical();
    customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

    if (auto top = createChild())
    {
        top->layoutHorizontal();
        top->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

        auto image = top->createChild<ui::TextLabel>("[img:import32]");
        image->customVerticalAligment(ui::ElementVerticalLayout::Top);
        image->customMargins(5, 5, 5, 5);

        if (auto rightPanel = top->createChild())
        {
            rightPanel->layoutVertical();
            rightPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

            m_fileNameText = rightPanel->createChild<ui::EditBox>();
            m_fileNameText->text("-- no file --");
            m_fileNameText->customMargins(4, 2, 4, 2);
            m_fileNameText->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_fileNameText->enable(false);

            m_statusText = rightPanel->createChild<ui::TextLabel>("[img:question_red] Unknown");
            m_statusText->customMargins(0, 2, 0, 2);

            bind(EVENT_IMPORT_STATUS_CHANGED) = [this]() { updateStatus(); };
        }
    }

    {
        auto buttonBox = createChild();
        buttonBox->layoutHorizontal();

        {
            m_buttonRecheck = buttonBox->createChildWithType<ui::Button>("PushButton"_id, "[img:arrow_refresh] Recheck");
            m_buttonRecheck->bind(ui::EVENT_CLICKED) = [this]() { cmdRecheckeck(); };
            m_buttonRecheck->customProportion(1.0f);
            m_buttonRecheck->enable(false);
        }

        {
            m_buttonReimport = buttonBox->createChildWithType<ui::Button>("PushButton"_id, "[img:cog] Reimport");
            m_buttonReimport->bind(ui::EVENT_CLICKED) = [this]() { cmdReimport(); };
            m_buttonReimport->customProportion(1.0f);
            m_buttonReimport->enable(false);
        }

        {
            m_buttonShowFileList = buttonBox->createChildWithType<ui::Button>("PushButton"_id, "[img:table_gear] Assets");
            m_buttonShowFileList->bind(ui::EVENT_CLICKED) = [this]() { cmdShowSourceAssets(); };
            m_buttonShowFileList->customProportion(1.0f);
            m_buttonShowFileList->enable(false);
        }
    }
}

AssetFileImportWidget::~AssetFileImportWidget()
{
    if (m_checker)
    {
        m_checker->cancel();
        m_checker.reset();
    }
}

void AssetFileImportWidget::bindFile(StringView depotPath, const ResourceConfigurationPtr& config)
{
    m_config = config;
    m_events.clear();

    if (m_depotPath != depotPath)
    {
        m_depotPath = StringBuf(depotPath);

        m_buttonReimport->enable(false);
        m_buttonShowFileList->enable(false);
        m_buttonRecheck->enable(false);

        if (m_depotPath)
        {
            cmdRecheckeck();
        }
        else
        {
            m_fileNameText->text("-- no file --");
            m_statusText->text("[img:question_red] [b]Unknown");
        }

        /*m_events.bind(file->eventKey(), EVENT_MANAGED_FILE_RELOADED) = [this]() {
            cmdRecheckeck();
        };*/
    }
}

void AssetFileImportWidget::cmdShowSourceAssets()
{

}

void AssetFileImportWidget::cmdRecheckeck()
{
    if (m_checker)
    {
        m_checker->cancel();
        m_checker.reset();
    }

    if (m_depotPath)
    {
        m_fileNameText->text(m_depotPath);
        m_statusText->text("[img:hourglass] Checking...");

        m_checker = RefNew<AssetImportStatusCheck>(m_depotPath, this);
    }
}

void AssetFileImportWidget::cmdReimport()
{
    if (!m_checker || m_checker->status() == ImportStatus::Checking)
        return;

    auto setup = ui::MessageBoxSetup().question().yes().no().defaultNo().title("Update imported asset");
    if (m_checker->status() == ImportStatus::UpToDate)
    {
        if (ui::MessageButton::No == ui::ShowMessageBox(this, setup.message("Resource is up to date, update any way?")))
            return;
    }
    else if (m_checker->status() == ImportStatus::NotImportable)
    {
        if (ui::MessageButton::No == ui::ShowMessageBox(this, setup.message("Asset seems to be not importable any more, update any way?")))
            return;
    }
    else if (m_checker->status() == ImportStatus::MissingAssets)
    {
        if (ui::MessageButton::No == ui::ShowMessageBox(this, setup.message("Seems like some source files are missing, update any way?")))
            return;
    }
    else if (m_checker->status() != ImportStatus::Checking)
    {
        if (ui::MessageButton::No == ui::ShowMessageBox(this, setup.message("Checking for asset state has not finished yet, update any way?")))
            return;
    }
    else if (m_checker->status() != ImportStatus::NotUpToDate)
    {
        if (ui::MessageButton::No == ui::ShowMessageBox(this, setup.message("Asset is not a valid state to be imported, update any way?")))
            return;
    }

    call(EVENT_RESOURCE_REIMPORT_WITH_CONFIG, m_config);
}

void AssetFileImportWidget::cmdSaveConfiguration()
{

}

void AssetFileImportWidget::cmdLoadConfiguration()
{

}

//--

extern StringView ImportStatusToDisplayText(ImportStatus status, bool withIcon);

void AssetFileImportWidget::updateStatus()
{
    bool canShowFiles = false;
    bool canReimport = false;

    const auto status = m_checker->status();
    switch (status)
    {
    case ImportStatus::Pending:
    case ImportStatus::Checking:
    case ImportStatus::Canceled:
    case ImportStatus::Processing:
    case ImportStatus::NotImportable:
        break;

    case ImportStatus::NewAssetImported:
    case ImportStatus::NotUpToDate:
    case ImportStatus::UpToDate:
    case ImportStatus::MissingAssets:
        canShowFiles = true;
        canReimport = true;
        break;

    case ImportStatus::InvalidAssets:
    case ImportStatus::NotSupported:
        canShowFiles = true;
        break;
    }

    m_buttonReimport->enable(canReimport);
    m_buttonShowFileList->enable(canShowFiles);
    m_buttonRecheck->enable(true);
    m_statusText->text(ImportStatusToDisplayText(status, true));
}

END_BOOMER_NAMESPACE_EX(ed)
