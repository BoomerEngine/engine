/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "browserService.h"

#include "browserFiles.h"
#include "browserFilesModel.h"

#include "core/resource/include/depot.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiTrackBar.h"
#include "engine/ui/include/uiMenuBar.h"
#include "engine/ui/include/uiDockNotebook.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiListViewEx.h"
#include "core/resource/include/metadata.h"
#include "core/resource_compiler/include/sourceAssetService.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

void AssetBrowserTabFiles::initToolbar()
{
    auto toolbar = createChild<ui::ToolBar>();
    m_toolbar = toolbar;

    toolbar->createButton(ui::ToolBarButtonInfo().caption("Force refresh", "arrow_refresh").tooltip("Refresh folder structure (rescan physical directory)"))
        = [this]() { cmdRefreshFileList(); };
    toolbar->createSeparator();

    toolbar->createButton(ui::ToolBarButtonInfo("LockIcon"_id).caption("Lock", "lock").tooltip("Prevent current tab from being closed or replaced"))
        = [this]() { cmdLockTab(); };
    toolbar->createButton(ui::ToolBarButtonInfo("BookmarkIcon"_id).caption("Bookmark", "star").tooltip("Bookmark current directory so it's easier to find"))
        = [this]() { cmdBookmarkTab(); };

    toolbar->createSeparator();
    toolbar->createButton(ui::ToolBarButtonInfo("ListMode"_id).caption("List", "list_bullet").tooltip("Toggle simple list view ")) =
        [this]() { cmdToggleListView(); };
    toolbar->createButton(ui::ToolBarButtonInfo("FlatMode"_id).caption("Flat", "table").tooltip("Toggle flat view (include all sub directories)")) =
        [this]() { cmdToggleFlatView(); };
    toolbar->createSeparator();

    toolbar->createButton(ui::ToolBarButtonInfo().caption("[tag:#45C]Import[/tag]", "file_go").tooltip("Import assets here")) =
        [this]() { cmdImportHere(); };
    toolbar->createButton(ui::ToolBarButtonInfo().caption("[tag:#5A5]Create[/tag]", "file_add").tooltip("Create assets here")) =
        [this]() { cmdCreateHere(); };
    toolbar->createSeparator();

    if (auto bar = toolbar->createChild<ui::TrackBar>())
    {
        bar->range(16, 256);
        bar->resolution(0);
        bar->allowEditBox(false);
        bar->customStyle<float>("width"_id, 200.0f);
        bar->value(m_setup.iconSize);
        bar->bind(ui::EVENT_TRACK_VALUE_CHANGED) = [this](double value){
            auto size = (int)std::clamp<double>(value, 10, 512);
            if (m_setup.iconSize != size)
            {
                m_setup.iconSize = size;

                m_files->iterate<IAssetBrowserVisItem>([size](IAssetBrowserVisItem* item)
                    {
                        item->resizeIcon(size);
                    });
            }
        };
        toolbar->createSeparator();
    }
}

void AssetBrowserTabFiles::updateToolbar()
{
    if (m_depotPath)
    {
        const auto bookmarked = GetService<AssetBrowserService>()->checkDirectoryBookmark(m_depotPath);
        m_toolbar->updateButtonCaption("BookmarkIcon"_id, "Bookmark", (bookmarked ? "star" : "star_gray"));
    }

    {
        m_toolbar->updateButtonCaption("LockIcon"_id, "Lock", (tabLocked() ? "lock" : "lock_open"));
    }

    m_toolbar->toggleButton("FlatMode"_id, m_setup.flat);
    m_toolbar->toggleButton("ListMode"_id, m_setup.list);
}

void AssetBrowserTabFiles::initShortcuts()
{
    bindShortcut("Ctrl+L") = [this]() { cmdLockTab(); };
    bindShortcut("Ctrl+K") = [this]() { cmdToggleFlatView(); };
    bindShortcut("Ctrl+T") = [this]() { cmdDuplicateTab(); };
    bindShortcut("Ctrl+F4") = [this]() { cmdCloseTab(); };
    bindShortcut("Ctrl+B") = [this]() { cmdCloseTab(); };
    bindShortcut("F5") = [this]() { cmdRefreshFileList(); };
    bindShortcut("Backspace") = [this]() { cmdNavigateBack(); };
    bindShortcut("Delete") = [this]() { cmdDeleteSelection(); };
    //bindShortcut("Insert") = [this]() { cmdDeleteSelection(); };

    bindShortcut("Ctrl+C") = [this]() { cmdCopySelection(); };
    bindShortcut("Ctrl+X") = [this]() { cmdCutSelection(); };
    bindShortcut("Ctrl+V") = [this]() { cmdPasteSelection(); };

    bindShortcut("Ctrl+Shift+D") = [this]() { createNewDirectoryPlaceholder(); };
    bindShortcut("Ctrl+D") = [this]() { cmdDuplicateAssets(); };

    bindShortcut("F3") = [this]() { cmdShowInFiles(); };
    bindShortcut("Shift+F3") = [this]() { cmdShowSourceAsset(); };    
}

//--

void AssetBrowserTabFiles::cmdShowInFiles()
{
    if (const auto& elem = m_files->current<IAssetBrowserDepotVisItem>())
    {
        StringBuf absolutePath;
        if (GetService<DepotService>()->queryFileAbsolutePath(elem->depotPath(), absolutePath))
            ShowFileExplorer(absolutePath);
    }
}

void AssetBrowserTabFiles::cmdShowSourceAsset()
{
    if (const auto file = selectedFile())
    {
        if (const auto metadata = GetService<DepotService>()->loadFileToXMLObject<ResourceMetadata>(file))
        {
            if (!metadata->importDependencies.empty())
            {
                const auto& sourcePath = metadata->importDependencies[0].importPath;

                StringBuf absolutePath;
                if (GetService<SourceAssetService>()->resolveContextPath(sourcePath, absolutePath))
                    ShowFileExplorer(absolutePath);
            }
        }
    }
}

void AssetBrowserTabFiles::cmdRefreshFileList()
{
    refreshFileList();
}

void AssetBrowserTabFiles::cmdLockTab()
{
    tabLocked(!tabLocked());
    updateToolbar();
}

void AssetBrowserTabFiles::cmdBookmarkTab()
{
    if (m_depotPath)
    {
        const auto bookmarked = GetService<AssetBrowserService>()->checkDirectoryBookmark(m_depotPath);
        GetService<AssetBrowserService>()->toggleDirectoryBookmark(m_depotPath, !bookmarked);
    }
}

void AssetBrowserTabFiles::cmdToggleListView()
{
    m_setup.list = !m_setup.list;
    refreshFileList();
    updateToolbar();
}

void AssetBrowserTabFiles::cmdToggleFlatView()
{
    m_setup.flat = !m_setup.flat;
    refreshFileList();
    updateToolbar();
}

void AssetBrowserTabFiles::cmdImportHere()
{
    auto menu = RefNew<ui::MenuButtonContainer>();
    buildImportAssetMenu(menu);
    menu->show(this);
}

void AssetBrowserTabFiles::cmdCreateHere()
{
    auto menu = RefNew<ui::MenuButtonContainer>();
    buildNewAssetMenu(menu);
    menu->show(this);
}

void AssetBrowserTabFiles::cmdNavigateBack()
{
    if (m_depotPath)
    {
        if (auto parentDirectory = m_depotPath.view().parentDirectory())
        {
            const auto currentDirectory = m_depotPath;
            directory(parentDirectory);
            select(currentDirectory); // highlight the directory we are coming from
        }
    }
}

void AssetBrowserTabFiles::cmdDuplicateTab()
{
    if (m_depotPath)
    {
        auto copyTab = RefNew<AssetBrowserTabFiles>(m_setup);
        copyTab->tabLocked(tabLocked());
        copyTab->directory(m_depotPath);
        copyTab->select(selectedFile());

        if (auto notebook = findParent<ui::DockNotebook>())
        {
            if (auto node = notebook->layoutNode())
                node->attachPanel(copyTab, true);
        }
    }
}

void AssetBrowserTabFiles::cmdDuplicateAssets()
{
    /*
const auto initialFileName = m_dir->adjustFileName(TempString("{}_copy", sourceFile->name().view().fileStem()));
if (const auto addHocFile = RefNew<ManagedFilePlaceholder>(depot(), m_dir, initialFileName, &sourceFile->fileFormat()))
{
    if (auto index = m_filesModel->addAdHocElement(addHocFile))
    {
        selectItem(addHocFile);

        auto addHocFileRef = addHocFile.weak();

        m_filePlaceholders.pushBack(addHocFile);
        m_fileEvents.bind(addHocFile->eventKey(), EVENT_MANAGED_PLACEHOLDER_ACCEPTED) = [this, addHocFileRef, sourceFile]()
        {
            if (auto file = addHocFileRef.lock())
                finishFileDuplicate(file, sourceFile);
        };
        m_fileEvents.bind(addHocFile->eventKey(), EVENT_MANAGED_PLACEHOLDER_DISCARDED) = [this, addHocFileRef]()
        {
            if (auto file = addHocFileRef.lock())
                cancelFilePlaceholder(file);
        };
    }
}*/
}

void AssetBrowserTabFiles::cmdCloseTab()
{
    handleCloseRequest();
}

void AssetBrowserTabFiles::cmdDeleteSelection()
{
    auto files = selectedItems();
    //DeleteDepotItems(this, files);
}

void AssetBrowserTabFiles::cmdCopySelection()
{
    auto files = selectedItems();
    GetService<AssetBrowserService>()->markFiles(files, FILE_MARK_COPY);
}

void AssetBrowserTabFiles::cmdCutSelection()
{
    auto files = selectedItems();
    GetService<AssetBrowserService>()->markFiles(files, FILE_MARK_CUT);
}

void AssetBrowserTabFiles::cmdPasteSelection()
{

}

//--

END_BOOMER_NAMESPACE_EX(ed)
