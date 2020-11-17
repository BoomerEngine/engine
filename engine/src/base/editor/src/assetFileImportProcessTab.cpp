/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "editorService.h"

#include "backgroundCommand.h"
#include "assetFileImportBackgroundCommand.h"
#include "assetFileImportProcessTab.h"

#include "base/ui/include/uiImage.h"
#include "base/ui/include/uiTextLabel.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiWindow.h"
#include "base/ui/include/uiAbstractItemView.h"
#include "base/ui/include/uiCheckBox.h"
#include "base/ui/include/uiStyleValue.h"
#include "base/ui/include/uiEditBox.h"
#include "base/ui/include/uiComboBox.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiSplitter.h"
#include "base/ui/include/uiListView.h"
#include "base/ui/include/uiColumnHeaderBar.h"
#include "base/ui/include/uiNotebook.h"
#include "base/ui/include/uiDockNotebook.h"
#include "base/ui/include/uiDataInspector.h"
#include "base/resource_compiler/include/importInterface.h"
#include "base/resource_compiler/include/importFileService.h"
#include "base/io/include/ioSystem.h"
#include "base/resource/include/resourceMetadata.h"
#include "base/resource_compiler/include/importFileList.h"
#include "base/ui/include/uiSearchBar.h"

namespace ed
{

    //--

    AssetProcessingListModel::AssetProcessingListModel()
    {}

    AssetProcessingListModel::~AssetProcessingListModel()
    {}

    void AssetProcessingListModel::clear()
    {
        if (m_files.size())
        {
            m_files.clear();
            m_fileMap.clear();
            reset();
        }
    }

    void AssetProcessingListModel::setFileStatus(const StringBuf& depotFileName, res::ImportStatus status, float time /*= 0.0*/)
    {
        FileData* data = nullptr;
        if (m_fileMap.find(depotFileName, data))
        {
            if (data->status != status || data->time != time)
            {
                data->status = status;
                data->time = time;
                requestItemUpdate(data->index);
            }
        }
        else
        {
            auto data = CreateSharedPtr<FileData>();
            data->index = ui::ModelIndex(this, data);
            data->depotPath = depotFileName;
            data->status = status;
            data->time = time;

            m_fileMap[depotFileName] = data;
            m_files.pushBack(data);

            notifyItemAdded(ui::ModelIndex(), data->index);
        }
    }

    void AssetProcessingListModel::setFileProgress(const StringBuf& depotFileName, uint64_t count, uint64_t total, StringView message)
    {

    }

    const base::StringBuf& AssetProcessingListModel::fileDepotPath(ui::ModelIndex index) const
    {
        if (index.model() == this)
            if (auto* ptr = index.unsafe<AssetProcessingListModel::FileData>())
                return ptr->depotPath;

        return base::StringBuf::EMPTY();
    }

    ui::ModelIndex AssetProcessingListModel::parent(const ui::ModelIndex& item /*= ui::ModelIndex()*/) const
    {
        return ui::ModelIndex();
    }

    bool AssetProcessingListModel::hasChildren(const ui::ModelIndex& parent) const
    {
        return !parent.valid();
    }

    void AssetProcessingListModel::children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const
    {
        if (!parent)
        {
            outChildrenIndices.reserve(m_files.size());

            for (const auto& file : m_files)
                outChildrenIndices.pushBack(file->index);
        }
    }

    static StringView GetClassDisplayName(ClassType currentClass)
    {
        auto name = currentClass.name().view();
        name = name.afterLastOrFull("::");
        return name;
    }

    StringView ImportStatusToDisplayText(res::ImportStatus status, bool withIcon=false)
    {
        switch (status)
        {
            case res::ImportStatus::Pending: return "[b][color:#888]Pending[/color]";
            case res::ImportStatus::Processing: return "[b][color:#FF7]Processing...[/color]";
            case res::ImportStatus::Checking: return "[b][color:#AAA]Checking...[/color]";
            case res::ImportStatus::Canceled: return "[b][tag:#AAA]Canceled[/tag]";

            case res::ImportStatus::NotSupported: 
                if (withIcon)
                    return "[b][tag:#F88][color:#000][img:skull] Not supported[/tag]";
                else
                    return "[b][tag:#F88][color:#000]Not supported[/tag]";

            case res::ImportStatus::MissingAssets: 
                if (withIcon)
                    return "[b][tag:#F88][color:#000][img:exclamation] Missing files[/tag]";
                else
                    return "[b][tag:#F88][color:#000]Missing files[/tag]";

            case res::ImportStatus::InvalidAssets: 
                if (withIcon)
                    return "[b][tag:#F88][color:#000][img:skull] Invalid files[/tag]";
                else
                    return "[b][tag:#F88][color:#000]Invalid files[/tag]";

            case res::ImportStatus::UpToDate:
                if (withIcon)
                    return "[b][tag:#88F][img:tick] Up to date[/tag]";
                else
                    return "[b][tag:#88F]Up to date[/tag]";

            case res::ImportStatus::NotUpToDate: 
                if (withIcon)
                    return "[b][tag:#FF8][color:#000][img:exclamation] Assets modified[/tag]";
                else
                    return "[b][tag:#FF8][color:#000]Assets modified[/tag]";

            case res::ImportStatus::NewAssetImported:
                if (withIcon)
                    return "[b][tag:#8F8][img:cog] [color:#000]Imported[/tag]";
                else
                    return "[b][tag:#8F8][color:#000]Imported[/tag]";
        }

        return "[color:#888][b]---[/color]";
    }


    void AssetProcessingListModel::visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const
    {
        if (item.model() == this)
        {
            if (auto file = item.unsafe<FileData>())
            {
                if (!content)
                {
                    content = CreateSharedPtr<ui::IElement>();
                    content->layoutMode(ui::LayoutMode::Columns);

                    {
                        auto icon = content->createChild<ui::TextLabel>("[img:file_add]");
                        icon->customMargins(4, 0, 4, 0);
                        icon->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                        icon->expand();
                    }

                    {
                        auto fileClass = content->createNamedChild<ui::TextLabel>("FileClass"_id);
                        fileClass->customMargins(4, 0, 4, 0);
                        fileClass->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

                        const auto ext = file->depotPath.stringAfterLast(".");
                        const auto resourceClass = res::IResource::FindResourceClassByExtension(ext);
                        fileClass->text(TempString("[img:class] {}", resourceClass));
                    }

                    {
                        auto status = content->createNamedChild<ui::TextLabel>("Status"_id);
                        status->customMargins(4, 0, 4, 0);
                        status->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                        status->expand();
                    }

                    {
                        auto time = content->createNamedChild<ui::TextLabel>("Time"_id);
                        time->customMargins(4, 0, 4, 0);
                        time->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                        time->expand();
                    }

                    {
                        auto fileName = content->createNamedChild<ui::TextLabel>("FileNameStatic"_id);
                        fileName->customMargins(4, 0, 4, 0);
                        fileName->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                        fileName->text(TempString("[i]{}", file->depotPath));
                        fileName->expand();
                    }
                }

                if (auto elem = content->findChildByName<ui::TextLabel>("Status"_id))
                    elem->text(ImportStatusToDisplayText(file->status));

                if (auto elem = content->findChildByName<ui::TextLabel>("Time"_id))
                {
                    if (file->time > 0)
                        elem->text(TempString("{}", TimeInterval(file->time)));
                    else
                        elem->text(TempString("[color:#888]---[/color]", TimeInterval(file->time)));
                }
            }
        }
    }

    bool AssetProcessingListModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex /*= 0*/) const
    {
        if (first.model() == this && second.model() == this)
        {
            const auto* firstFile = first.unsafe<FileData>();
            const auto* secondFile = second.unsafe<FileData>();
            if (firstFile && secondFile)
                return firstFile->depotPath < secondFile->depotPath;
        }

        return first < second;
    }

    bool AssetProcessingListModel::filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
    {
        if (id.model() == this)
            if (auto* file = id.unsafe<FileData>())
                if (filter.testString(file->depotPath))
                    return true;

        return false;
    }

    ///--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportMainTab);
    RTTI_END_TYPE();

    AssetImportMainTab::AssetImportMainTab()
        : ui::DockPanel("[img:cog] Asset Processing", "AssetImportMainTab")
        , m_updateTimer(this, "UpdateTimer"_id)
    {
        layoutVertical();

        //--

        actions().bindCommand("Import.Cancel"_id) = [this]() { cmdCancel(); };
        actions().bindFilter("Import.Cancel"_id) = [this]() { return hasImportInProgress() && !m_importCancelRequested; };

        //--

        m_toolbar = createChild<ui::ToolBar>();
        m_toolbar->createButton("Import.Cancel"_id, ui::ToolbarButtonSetup().icon("delete").caption("Cancel import").tooltip("Cancel import process"));

        //--

        {
            auto leftPanel = /*split->*/createChild<ui::IElement>();
            leftPanel->layoutVertical();
            leftPanel->expand();

            auto filter = leftPanel->createChild<ui::SearchBar>();

            {
                auto bar = leftPanel->createChild<ui::ColumnHeaderBar>();
                bar->addColumn("", 30.0f, true, false, false);
                bar->addColumn("Class", 180.0f, true, true, true);
                bar->addColumn("Status", 120.0f, true, true, false);
                bar->addColumn("Time", 90.0f, true, true, false);
                bar->addColumn("Depot path", 900.0f, false);
            }

            m_fileList = leftPanel->createChild<ui::ListView>();
            m_fileList->expand();
            m_fileList->columnCount(5);

            m_filesListModel = CreateSharedPtr<AssetProcessingListModel>();
            m_fileList->model(m_filesListModel);
            filter->bindItemView(m_fileList);

            m_fileList->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]()
            {
                updateSelection();
            };

            m_fileList->bind(ui::EVENT_ITEM_ACTIVATED) = [this]()
            {
                showSelectedFilesInBrowser();
            };

            m_fileList->bind(ui::EVENT_CONTEXT_MENU) = [this]()
            {
                showFilesContextMenu();
            };
        }

        //--

        m_updateTimer = [this]() { updateState(); };
        m_updateTimer.startRepeated(0.1f);

        //--
    }

    AssetImportMainTab::~AssetImportMainTab()
    {}

    void AssetImportMainTab::cmdCancel()
    {
        cancelAssetImport();
    }

    bool AssetImportMainTab::hasImportInProgress() const
    {
        return m_backgroundJob && m_backgroundJob->running();
    }

    bool AssetImportMainTab::startAssetImport(res::ImportListPtr files)
    {
        if (hasImportInProgress())
        {
            ui::PostNotificationMessage(this, ui::MessageType::Warning, "ImportProcess"_id, "Import process already in progress");
            return false;
        }

        if (!files || files->files().empty())
        {
            ui::PostNotificationMessage(this, ui::MessageType::Warning, "ImportProcess"_id, "Nothing to import");
            return false;
        }
        
        m_importCancelRequested = false;
        m_importKillRequested = false;

        m_filesListModel->clear();

        auto command = CreateSharedPtr<AssetImportCommand>(files, m_filesListModel);
        auto job = GetService<Editor>()->runBackgroundCommand(command);
        if (!job)
        {
            ui::PostNotificationMessage(this, ui::MessageType::Warning, "ImportProcess"_id, "Failed to start import job");
            return false;
        }

        m_backgroundJob = job;
        m_backgroundCommand = command;
        return true;
    }

    void AssetImportMainTab::cancelAssetImport()
    {
        if (!m_importCancelRequested)
        {
            ui::PostNotificationMessage(this, ui::MessageType::Info, "ImportProcess"_id, "Requested cancel of import process");

            if (m_backgroundJob)
                m_backgroundJob->requestCancel();

            m_importCancelRequested = true;
        }
    }

    void AssetImportMainTab::updateState()
    {
        if (m_backgroundJob)
        {
            if (!m_backgroundJob->running())
            {
                ui::PostNotificationMessage(this, ui::MessageType::Info, "ImportProcess"_id, "Import job finished");
                m_backgroundJob.reset();
                m_backgroundCommand.reset();
            }
        }
    }

    void AssetImportMainTab::showSelectedFilesInBrowser()
    {
        for (const auto& index : m_fileList->selection())
        {
            if (auto depotPath = m_filesListModel->fileDepotPath(index))
            {
                if (auto* managedFile = GetService<Editor>()->managedDepot().findManagedFile(depotPath))
                {
                    GetService<Editor>()->mainWindow().selectFile(managedFile);
                    break;
                }
            }
        }
    }

    void AssetImportMainTab::showFilesContextMenu()
    {
        Array<StringBuf> depotPaths;
        depotPaths.reserve(m_fileList->selection().size());
        for (const auto& index : m_fileList->selection())
            if (auto depotPath = m_filesListModel->fileDepotPath(index))
                depotPaths.emplaceBack(depotPath);

        auto menu = CreateSharedPtr<ui::MenuButtonContainer>();

        if (depotPaths.size() == 1)
        {
            if (auto* managedFile = GetService<Editor>()->managedDepot().findManagedFile(depotPaths[0]))
            {
                menu->createCallback("Show in depot...", "[img:zoom]") = [managedFile]() {
                    GetService<ed::Editor>()->mainWindow().selectFile(managedFile);
                };
            }

            menu->createSeparator();
        }

        if (m_backgroundCommand)
        {
            auto command = m_backgroundCommand;
            menu->createCallback("Cancel import", "[img:cancel]") = [command, depotPaths]() {
                for (const auto& path : depotPaths)
                    command->cancelSingleFile(path);
            };
        }

        menu->show(this);
    }

    void AssetImportMainTab::updateSelection()
    {

    }

    ///--

} // ed