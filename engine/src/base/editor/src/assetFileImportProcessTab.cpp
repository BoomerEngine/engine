/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "editorConfig.h"
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
            beingRemoveRows(ui::ModelIndex(), 0, m_files.size());
            m_files.clear();
            m_fileMap.clear();
            endRemoveRows();
        }
    }

    void AssetProcessingListModel::setFileStatus(const StringBuf& depotFileName, res::ImportStatus status, float time /*= 0.0*/)
    {
        FileData* data = nullptr;
        if (m_fileMap.find(depotFileName, data))
        {
            if (data->m_status != status || data->m_time != time)
            {
                data->m_status = status;
                data->m_time = time;

                if (auto index = indexForFile(data))
                    requestItemUpdate(index);
            }
        }
        else
        {
            data = MemNew(FileData);
            data->m_depotPath = depotFileName;
            data->m_status = status;
            data->m_time = time;

            m_fileMap[depotFileName] = data;

            uint32_t index = m_files.size();

            beingInsertRows(ui::ModelIndex(), index, 1);
            m_files.pushBack(AddRef(data));
            endInsertRows();
        }
    }

    void AssetProcessingListModel::setFileProgress(const StringBuf& depotFileName, uint64_t count, uint64_t total, StringView<char> message)
    {

    }

    const base::StringBuf& AssetProcessingListModel::fileDepotPath(ui::ModelIndex index) const
    {
        if (auto* ptr = fileForIndexFast(index))
            return ptr->m_depotPath;
        return base::StringBuf::EMPTY();
    }

    AssetProcessingListModel::FileData* AssetProcessingListModel::fileForIndexFast(const ui::ModelIndex& index) const
    {
        return static_cast<AssetProcessingListModel::FileData*>(index.weakRef().lock());
    }

    RefPtr<AssetProcessingListModel::FileData> AssetProcessingListModel::fileForIndex(const ui::ModelIndex& index) const
    {
        auto* ptr = fileForIndexFast(index);
        return RefPtr<AssetProcessingListModel::FileData>(NoAddRef(ptr));
    }

    ui::ModelIndex AssetProcessingListModel::indexForFile(const FileData* entry) const
    {
        auto index = m_files.find(entry);
        if (index != -1)
            return ui::ModelIndex(this, index, 0, m_files[index]);
        return ui::ModelIndex();
    }

    uint32_t AssetProcessingListModel::rowCount(const ui::ModelIndex& parent) const
    {
        if (!parent.valid())
            return m_files.size();
        return 0;
    }

    bool AssetProcessingListModel::hasChildren(const ui::ModelIndex& parent) const
    {
        return !parent.valid();
    }

    bool AssetProcessingListModel::hasIndex(int row, int col, const ui::ModelIndex& parent /*= ui::ModelIndex()*/) const
    {
        return !parent.valid() && col == 0 && row >= 0 && row < (int)m_files.size();
    }

    ui::ModelIndex AssetProcessingListModel::parent(const ui::ModelIndex& item /*= ui::ModelIndex()*/) const
    {
        return ui::ModelIndex();
    }

    ui::ModelIndex AssetProcessingListModel::index(int row, int column, const ui::ModelIndex& parent) const
    {
        if (hasIndex(row, column, parent))
            return ui::ModelIndex(this, row, column, m_files[row]);
        return ui::ModelIndex();
    }

    static StringView<char> GetClassDisplayName(ClassType currentClass)
    {
        auto name = currentClass.name().view();
        name = name.afterLastOrFull("::");
        return name;
    }

    StringView<char> ImportStatusToDisplayText(res::ImportStatus status)
    {
        switch (status)
        {
            case res::ImportStatus::Pending: return "[b][color:#888]Pending[/color]";
            case res::ImportStatus::Processing: return "[b][color:#FF7]Processing...[/color]";
            case res::ImportStatus::Checking: return "[b][color:#AAA]Checking...[/color]";
            case res::ImportStatus::Canceled: return "[b][tag:#AAA]Canceled[/tag]";
            case res::ImportStatus::NotSupported: return "[b][tag:#F88][color:#000]Not supported[/tag]";
            case res::ImportStatus::MissingAssets: return "[b][tag:#F88][color:#000]Missing files[/tag]";
            case res::ImportStatus::InvalidAssets: return "[b][tag:#F88][color:#000]Invalid files[/tag]";
            case res::ImportStatus::UpToDate: return "[b][tag:#88F]Up to date[/tag]";
            case res::ImportStatus::NotUpToDate: return "[b][tag:#FF8][color:#000]Assets modified[/tag]";
            case res::ImportStatus::NewAssetImported: return "[b][tag:#8F8][color:#000]Imported[/tag]";
        }

        return "[color:#888][b]---[/color]";
    }


    void AssetProcessingListModel::visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const
    {
        if (auto file = fileForIndexFast(item))
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

                    const auto ext = file->m_depotPath.stringAfterLast(".");
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
                    fileName->text(TempString("[i]{}", file->m_depotPath));
                    fileName->expand();
                }
            }

            if (auto elem = content->findChildByName<ui::TextLabel>("Status"_id))
                elem->text(ImportStatusToDisplayText(file->m_status));

            if (auto elem = content->findChildByName<ui::TextLabel>("Time"_id))
            {
                if (file->m_time > 0)
                    elem->text(TempString("{}", TimeInterval(file->m_time)));
                else
                    elem->text(TempString("[color:#888]---[/color]", TimeInterval(file->m_time)));
            }
        }
    }

    bool AssetProcessingListModel::compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex /*= 0*/) const
    {
        const auto* firstFile = fileForIndexFast(first);
        const auto* secondFile = fileForIndexFast(second);
        if (firstFile && secondFile)
            return firstFile->m_depotPath < secondFile->m_depotPath;

        return first < second;
    }

    bool AssetProcessingListModel::filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex /*= 0*/) const
    {
        if (auto* file = fileForIndexFast(id))
        {
            if (filter.testString(file->m_depotPath))
                return true;
        }

        return false;
    }

    ///--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportMainTab);
    RTTI_END_TYPE();

    AssetImportMainTab::AssetImportMainTab()
        : ui::DockPanel("[img:cog] Asset Processing")
        , OnImportFinished(this, "OnImportFinished"_id)
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

            m_fileList->OnSelectionChanged = [this]()
            {
                updateSelection();
            };

            m_fileList->OnItemActivated = [this]()
            {
                showSelectedFilesInBrowser();
            };

            m_fileList->bind("OnContextMenu"_id) = [this]()
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
                    GetService<Editor>()->selectFile(managedFile);
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
                    GetService<ed::Editor>()->selectFile(managedFile);
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