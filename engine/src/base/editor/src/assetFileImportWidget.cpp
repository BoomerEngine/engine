/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "editorService.h"

#include "managedFileNativeResource.h"
#include "managedFileAssetChecks.h"
#include "assetFileImportWidget.h"

#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiImage.h"

namespace ed
{

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetFileImportWidget);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("AssetFileImportWidget");
    RTTI_END_TYPE();

    AssetFileImportWidget::AssetFileImportWidget()
    {
        layoutHorizontal();

        /*if (auto leftPanel = createChild<>())
        {
            auto image = leftPanel->createChild<ui::TextLabel>("[img:import32]");
            image->customVerticalAligment(ui::ElementVerticalLayout::Top);
            image->customMargins(5, 5, 5, 5);
        }*/

        if (auto rightPanel = createChild<>())
        {
            rightPanel->expand();
            
            m_fileNameText = rightPanel->createChild<ui::TextLabel>("-- no file --");
            m_fileNameText->customMargins(0, 2, 0, 2);

            m_statusText = rightPanel->createChild<ui::TextLabel>("[img:question_red] Unknown");
            m_statusText->customMargins(0, 2, 0, 2);

            bind("OnImportStatusChanged"_id) = [this]() { updateStatus(); };

            {
                auto buttonBox = rightPanel->createChild();
                buttonBox->layoutHorizontal();

                {
                    m_buttonRecheck = buttonBox->createChildWithType<ui::Button>("PushButton"_id, "[img:arrow_refresh] Recheck");
                    m_buttonRecheck->OnClick = [this]() { cmdRecheckeck(); };
                    m_buttonRecheck->customProportion(1.0f);
                    m_buttonRecheck->enable(false);
                }

                {
                    m_buttonReimport = buttonBox->createChildWithType<ui::Button>("PushButton"_id, "[img:cog] Reimport");
                    m_buttonReimport->OnClick = [this]() { cmdReimport(); };
                    m_buttonReimport->customProportion(1.0f);
                    m_buttonReimport->enable(false);
                }

                {
                    m_buttonShowFileList = buttonBox->createChildWithType<ui::Button>("PushButton"_id, "[img:table_gear] Assets");
                    m_buttonShowFileList->OnClick = [this]() { cmdShowSourceAssets(); };
                    m_buttonShowFileList->customProportion(1.0f);
                    m_buttonShowFileList->enable(false);
                }
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

    void AssetFileImportWidget::bindFile(ManagedFileNativeResource* file, const res::ResourceConfigurationPtr& config)
    {
        m_config = config;
        m_events.clear();

        if (m_file != file)
        {
            m_file = file;

            m_buttonReimport->enable(false);
            m_buttonShowFileList->enable(false);
            m_buttonRecheck->enable(false);

            if (m_file)
            {
                cmdRecheckeck();
            }
            else
            {
                m_fileNameText = createChild<ui::TextLabel>("-- no file --");
                m_statusText = createChild<ui::TextLabel>("[img:question_red] [b]Unknown");
            }

            m_events.bind(file->eventKey(), EVENT_MANAGED_FILE_RELOADED) = [this]() {
                cmdRecheckeck();
            };

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

        if (m_file)
        {
            m_fileNameText->text(m_file->name());
            m_statusText->text("[img:hourglass] Checking...");

            m_checker = base::CreateSharedPtr<ManagedFileImportStatusCheck>(m_file, this);
        }
    }

    void AssetFileImportWidget::cmdReimport()
    {
        if (m_file)
            base::GetService<Editor>()->addReimportFile(m_file, m_config);
    }

    void AssetFileImportWidget::cmdSaveConfiguration()
    {

    }

    void AssetFileImportWidget::cmdLoadConfiguration()
    {

    }

    //--

    extern StringView<char> ImportStatusToDisplayText(res::ImportStatus status);

    void AssetFileImportWidget::updateStatus()
    {
        bool canShowFiles = false;
        bool canReimport = false;

        StringBuilder txt;

        const auto status = m_checker->status();
        switch (status)
        {
        case res::ImportStatus::Pending:
        case res::ImportStatus::Checking:
        case res::ImportStatus::Canceled:
        case res::ImportStatus::Processing:
            break;

        case res::ImportStatus::NewAssetImported:
        case res::ImportStatus::NotUpToDate:
            txt.append("[img:warning] ");
            canShowFiles = true;
            canReimport = true;
            break;

        case res::ImportStatus::MissingAssets:
            txt.append("[img:exclamation] ");
            canShowFiles = true;
            canReimport = true;
            break;

        case res::ImportStatus::NotSupported:
            txt.append("[img:skull] ");
            canShowFiles = true;
            break;

        case res::ImportStatus::InvalidAssets:
            txt.append("[img:skull] ");
            canShowFiles = true;
            break;

        case res::ImportStatus::UpToDate:
            txt.append("[img:tick] ");
            canShowFiles = true;
            canReimport = true;
            break;

        case res::ImportStatus::NotImportable:
            txt.append("[img:tick] ");
            break;
        }

        txt.append(ImportStatusToDisplayText(status));

        m_buttonReimport->enable(canReimport);
        m_buttonShowFileList->enable(canShowFiles);
        m_buttonRecheck->enable(true);

        m_statusText->text(txt.view());
    }

} // ed