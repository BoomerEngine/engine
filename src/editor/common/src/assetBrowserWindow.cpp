/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "assetBrowserWindow.h"
#include "assetDepotTreeModel.h"
#include "assetBrowserTabFiles.h"

#include "managedDirectory.h"
#include "managedDepot.h"

#include "engine/ui/include/uiTreeViewEx.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDockContainer.h"
#include "engine/ui/include/uiDockNotebook.h"
#include "engine/ui/include/uiSearchBar.h"
#include "engine/ui/include/uiElementConfig.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowser);
RTTI_END_TYPE();

///--

AssetBrowser::AssetBrowser()
    : ui::Window(ui::WindowFeatureFlagBit::DEFAULT_FRAME, "Boomer Engine - Asset Browser")
    , m_depotEvents(this)
{
    customMinSize(1200, 900);

    m_dockContainer = createChild<ui::DockContainer>();
    m_dockContainer->expand();

    {
        /*m_depotEvents.bind(depot->eventKey(), EVENT_MANAGED_DEPOT_DIRECTORY_DELETED) = [this](ManagedDirectoryPtr dir)
        {
            closeDirectoryTab(dir);
        };*/
    }

    {
        auto treePanel = RefNew<ui::DockPanel>("[img:database] Depot");// , "DepotTree");

        auto filter = treePanel->createChild<ui::SearchBar>();

        m_depotTree = treePanel->createChild<ui::TreeViewEx>();

        {
            m_engineRoot = RefNew<AssetDepotTreeEngineRoot>();
            m_depotTree->addRoot(m_engineRoot);
        }

        {
            m_projectRoot = RefNew<AssetDepotTreeProjectRoot>();
            m_depotTree->addRoot(m_projectRoot);
        }

        //filter->bindItemView(m_depotTree);

        m_depotTree->bind(ui::EVENT_ITEM_SELECTION_CHANGED) = [this]()
        {
            if (auto treeItem = m_depotTree->current<AssetDepotTreeDirectoryItem>())
                navigateToDirectory(treeItem->depotPath());
        };

        m_dockContainer->layout().left().attachPanel(treePanel);
    }
}

AssetBrowser::~AssetBrowser()
{
}

void AssetBrowser::handleExternalCloseRequest()
{
    requestHide();
}

AssetDepotTreeDirectoryItem* AssetBrowser::findDirectory(StringView depotPath, bool autoExpand) const
{
    InplaceArray<StringView, 12> parts;
    depotPath.slice("/", false, parts);

    if (parts.empty())
        return nullptr;

    AssetDepotTreeDirectoryItem* dir = nullptr;
    if (parts[0] == "engine")
        dir = m_engineRoot;
    else if (parts[1] == "project")
        dir = m_projectRoot;

    if (!dir)
        return nullptr;

    for (uint32_t i=1; i<parts.size(); ++i)
    {
        dir = dir->findChild(parts[i], autoExpand);
        if (!dir)
            break;
    }

    return dir;
}

void AssetBrowser::configLoad(const ui::ConfigBlock& block)
{
    // load window state
    TBaseClass::configLoad(block);

    // restore tabs
    uint32_t count = block.readOrDefault<uint32_t>("NumTabs", 0);
    for (uint32_t i = 0; i < count; ++i)
    {
        auto tabConfig = block.tag(TempString("Tab{}", i));
        auto tabType = tabConfig.readOrDefault<StringBuf>("Type", "");

        if (tabType == "Directory")
        {
            auto tabPath = tabConfig.readOrDefault<StringBuf>("Path", "");

            // create new tab
            auto newTab = RefNew<AssetBrowserTabFiles>(AssetBrowserContext::DirectoryTab);
            newTab->directory(tabPath);
            newTab->configLoad(tabConfig);

            // attach
            auto active = tabConfig.readOrDefault<bool>("Active", false);
            m_dockContainer->layout().attachPanel(newTab, active);
        }
    }

    // expand directories
    {
        auto expandedDirectories = block.readOrDefault<Array<StringBuf>>("ExpandedDirectories");
        for (auto& path : expandedDirectories)
            if (auto* dir = findDirectory(path, true))
                dir->expand();
    }

    // select directory
    {
        auto selectedDirectoryPath = block.readOrDefault<StringBuf>("SelectedDirectory");
        if (selectedDirectoryPath)
        {
            if (auto* dir = findDirectory(selectedDirectoryPath, true))
                m_depotTree->select(dir);
        }
    }

    // make sure something is displayed
    const bool hasFileTabs = m_dockContainer->iterateSpecificPanels<AssetBrowserTabFiles>([](AssetBrowserTabFiles* tab) { return true; });
    if (!hasFileTabs)
        navigateToDirectory("/engine/");

    // load docking layout (AFTER panels are created)
    m_dockContainer->configLoad(block);
}

void AssetBrowser::configSave(const ui::ConfigBlock& block) const
{
    // tab configuration
    InplaceArray<AssetBrowserTabFiles*, 10> fileTabs;
    m_dockContainer->collectSpecificPanels<AssetBrowserTabFiles>(fileTabs);
    InplaceArray<AssetBrowserTabFiles*, 10> activeFileTabs;
    m_dockContainer->collectSpecificPanels<AssetBrowserTabFiles>(activeFileTabs, ui::DockPanelIterationMode::ActiveOnly);

    block.write<uint32_t>("NumTabs", fileTabs.size());

    //config.write<int>("ActiveTab", m_tabs->selectedTab());
    for (uint32_t i=0; i<fileTabs.size(); ++i)
    {
        auto tab = fileTabs[i];

        // store tab type
        auto tabConfig = block.tag(TempString("Tab{}", i));
        tabConfig.write<StringBuf>("Type", "Directory");
        tabConfig.write<bool>("Active", activeFileTabs.contains(tab));
        tab->configSave(tabConfig);
    }

    // collect opened directories from the depot tree
    /*{
        Array<ui::ModelIndex> indices;
        m_depotTree->collectExpandedItems(indices);

        Array<StringBuf> expandedFolders;
        for (auto& index : indices)
        {
            StringBuf path;
            if (m_depotTreeModel->findDirectoryForID(index, path))
                expandedFolders.pushBack(path);
        }

        block.write("ExpandedDirectories", expandedFolders);
    }*/

    // get the focused folder
    if (auto selected = m_depotTree->current<AssetDepotTreeDirectoryItem>())
        block.write("SelectedDirectory", selected->depotPath());
    else
        block.write("SelectedDirectory", "");

    // docking layout
    m_dockContainer->configSave(block);

    // save window state
    TBaseClass::configSave(block);
}

StringBuf AssetBrowser::selectedFile() const
{
    StringBuf ret;

    m_dockContainer->iterateSpecificPanels<AssetBrowserTabFiles>([&ret](AssetBrowserTabFiles* tab)
        {
            ret = tab->selectedFile();
            return !ret.empty();
        }, ui::DockPanelIterationMode::ActiveOnly);

    return ret;
}

bool AssetBrowser::showFile(StringView depotPath)
{
    if (depotPath)
    {
        auto dirPath = depotPath.baseDirectory();
    
        RefPtr<AssetBrowserTabFiles> tab;
        navigateToDirectory(dirPath, &tab);

        if (tab)
            return tab->selectItem(depotPath.fileStem());
    }

    return false;
}

bool AssetBrowser::showDirectory(StringView depotPath, bool exploreContent)
{
    if (auto* dir = findDirectory(depotPath, true))
    {
        dir->expand();
        m_depotTree->select(dir, ui::ItemSelectionModeBit::Default, exploreContent);
        return true;
    }

    return false;
}

void AssetBrowser::navigateToDirectory(StringView depotPath, RefPtr<AssetBrowserTabFiles>* outTab)
{
    InplaceArray<AssetBrowserTabFiles*, 10> fileTabs;
    m_dockContainer->collectSpecificPanels<AssetBrowserTabFiles>(fileTabs);

    // switch to tab with that directory
    for (auto* tab : fileTabs)
    {
        if (tab->depotPath() == depotPath)
        {
            m_dockContainer->activatePanel(tab);

            if (outTab)
                *outTab = RefPtr<AssetBrowserTabFiles>(AddRef(tab));

            return;
        }
    }

    InplaceArray<AssetBrowserTabFiles*, 10> activeTabs;
    m_dockContainer->collectSpecificPanels<AssetBrowserTabFiles>(activeTabs, ui::DockPanelIterationMode::ActiveOnly);

    // get the active tab and if it's not locked use it
    for (auto* tab : activeTabs)
    {
        if (tab && !tab->tabLocked())
        {
            tab->directory(depotPath);

            if (outTab)
                *outTab = RefPtr<AssetBrowserTabFiles>(AddRef(tab));

            return;
        }
    }

    // get any non locked tab and change directory to new one
    for (auto* tab : fileTabs)
    {
        if (!tab->tabLocked())
        {
            tab->directory(depotPath);
            m_dockContainer->activatePanel(tab);

            if (outTab)
                *outTab = RefPtr<AssetBrowserTabFiles>(AddRef(tab));

            return;
        }
    }

    // create new tab
    auto newTab = RefNew<AssetBrowserTabFiles>(AssetBrowserContext::DirectoryTab);
    newTab->directory(depotPath);
        
    // attach it to the main area
    m_dockContainer->layout().attachPanel(newTab);
    if (outTab)
        *outTab = newTab;
}

/*void AssetBrowser::syncActiveDirectory()
{
    if (auto dir = activeDirectory())
    {
        if (auto index = m_depotTreeModel->findNodeForData(dir))
        {
            m_depotTree->ensureVisible(index);
            m_depotTree->select(index, ui::ItemSelectionModeBit::Default, false);
        }
    }
}*/

void AssetBrowser::closeDirectoryTab(StringView depotPath)
{
    InplaceArray<AssetBrowserTabFiles*, 10> fileTabs;
    m_dockContainer->collectSpecificPanels<AssetBrowserTabFiles>(fileTabs);

    for (auto fileTab : fileTabs)
        if (fileTab->depotPath() == depotPath)
            fileTab->close();
}

//--

END_BOOMER_NAMESPACE_EX(ed)
