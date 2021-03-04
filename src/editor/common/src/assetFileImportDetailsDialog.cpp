/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "assetFileImportDetailsDialog.h"

#include "engine/ui/include/uiImage.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiWindow.h"
#include "engine/ui/include/uiAbstractItemView.h"
#include "engine/ui/include/uiCheckBox.h"
#include "engine/ui/include/uiStyleValue.h"
#include "engine/ui/include/uiEditBox.h"
#include "engine/ui/include/uiComboBox.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiSplitter.h"
#include "engine/ui/include/uiListView.h"
#include "engine/ui/include/uiColumnHeaderBar.h"
#include "engine/ui/include/uiNotebook.h"
#include "engine/ui/include/uiDockNotebook.h"
#include "engine/ui/include/uiDataInspector.h"
#include "core/resource_compiler/include/importInterface.h"
#include "core/resource_compiler/include/importFileService.h"
#include "core/io/include/io.h"
#include "core/resource/include/resourceMetadata.h"
#include "core/resource_compiler/include/importFileList.h"
#include "engine/ui/include/uiSearchBar.h"
#include "editorService.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

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
        auto data = RefNew<FileData>();
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

const StringBuf& AssetProcessingListModel::fileDepotPath(ui::ModelIndex index) const
{
    if (index.model() == this)
        if (auto* ptr = index.unsafe<AssetProcessingListModel::FileData>())
            return ptr->depotPath;

    return StringBuf::EMPTY();
}

ui::ModelIndex AssetProcessingListModel::parent(const ui::ModelIndex& item /*= ui::ModelIndex()*/) const
{
    return ui::ModelIndex();
}

bool AssetProcessingListModel::hasChildren(const ui::ModelIndex& parent) const
{
    return !parent.valid();
}

void AssetProcessingListModel::children(const ui::ModelIndex& parent, Array<ui::ModelIndex>& outChildrenIndices) const
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
                content = RefNew<ui::IElement>();
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
            return firstFile->depotPath.view() < secondFile->depotPath.view();
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

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetImportDetailsDialog);
RTTI_END_TYPE();

AssetImportDetailsDialog::AssetImportDetailsDialog(AssetProcessingListModel* listModel)
    : m_updateTimer(this, "UpdateTimer"_id)
    , m_filesListModel(AddRef(listModel))
{
    layoutVertical();

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
        m_fileList->customInitialSize(800, 600);

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

AssetImportDetailsDialog::~AssetImportDetailsDialog()
{}

void AssetImportDetailsDialog::updateState()
{

}

void AssetImportDetailsDialog::showSelectedFilesInBrowser()
{
    for (const auto& index : m_fileList->selection())
    {
        if (auto depotPath = m_filesListModel->fileDepotPath(index))
        {
            if (auto* managedFile = GetEditor()->managedDepot().findManagedFile(depotPath))
            {
                GetEditor()->showFile(managedFile);
                break;
            }
        }
    }
}

void AssetImportDetailsDialog::showFilesContextMenu()
{
    Array<StringBuf> depotPaths;
    depotPaths.reserve(m_fileList->selection().size());
    for (const auto& index : m_fileList->selection())
        if (auto depotPath = m_filesListModel->fileDepotPath(index))
            depotPaths.emplaceBack(depotPath);

    auto menu = RefNew<ui::MenuButtonContainer>();

    if (depotPaths.size() == 1)
    {
        if (auto* managedFile = GetEditor()->managedDepot().findManagedFile(depotPaths[0]))
        {
            menu->createCallback("Show in depot...", "[img:zoom]") = [managedFile]() {
                ed::GetEditor()->showFile(managedFile);
            };
        }

        menu->createSeparator();
    }

    /*if (m_backgroundCommand)
    {
        auto command = m_backgroundCommand;
        menu->createCallback("Cancel import", "[img:cancel]") = [command, depotPaths]() {
            for (const auto& path : depotPaths)
                command->cancelSingleFile(path);
        };
    }*/

    menu->show(this);
}

void AssetImportDetailsDialog::updateSelection()
{

}

///--

END_BOOMER_NAMESPACE_EX(ed)
