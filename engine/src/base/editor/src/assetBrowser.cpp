/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"
#include "assetBrowser.h"
#include "assetDepotTreeModel.h"
#include "assetBrowserTabFiles.h"

#include "managedDirectory.h"
#include "managedDepot.h"

#include "base/ui/include/uiTreeView.h"
#include "base/ui/include/uiDockLayout.h"
#include "base/ui/include/uiDockContainer.h"
#include "base/ui/include/uiDockNotebook.h"
#include "base/ui/include/uiSearchBar.h"

namespace ed
{

    RTTI_BEGIN_TYPE_NATIVE_CLASS(AssetBrowser);
    RTTI_END_TYPE();

    ///--

    AssetBrowser::AssetBrowser(ManagedDepot* depot)
        : ui::DockPanel("[img:database] Asset Browser")
        , m_depot(depot)
    {
        m_dockContainer = createChild<ui::DockContainer>();
        m_dockContainer->expand();

        {
            auto treePanel = base::CreateSharedPtr<ui::DockPanel>("[img:database] Depot");// , "DepotTree");

            m_depotTreeModel = base::CreateSharedPtr<AssetDepotTreeModel>(depot);

            auto filter = treePanel->createChild<ui::SearchBar>();

            m_depotTree = treePanel->createChild<ui::TreeView>();
            m_depotTree->expand();
            m_depotTree->sort(0, true);
            m_depotTree->model(m_depotTreeModel);
            filter->bindItemView(m_depotTree);

            m_depotTree->bind("OnSelectionChanged"_id, this) = [this]()
            {
                if (auto dir = base::rtti_cast<ManagedDirectory>(m_depotTreeModel->dataForNode(m_depotTree->current())))
                    navigateToDirectory(dir);
            };

            m_dockContainer->layout().left().attachPanel(treePanel);
        }
    }

    AssetBrowser::~AssetBrowser()
    {
    }

    void AssetBrowser::loadConfig(const ConfigGroup& config)
    {
        // restore tabs
        uint32_t count = config.readOrDefault<uint32_t>("NumTabs", 0);
        for (uint32_t i = 0; i < count; ++i)
        {
            auto tabConfig = config[base::TempString("Tab{}", i).c_str()];
            auto tabType = tabConfig.readOrDefault<base::StringBuf>("Type", "");
            auto tabID = tabConfig.readOrDefault<base::StringBuf>("ID", "");

            if (tabType == "Directory")
            {
                auto tabPath = tabConfig.readOrDefault<base::StringBuf>("Path", "");
                if (auto tabDirectory = m_depot->findPath(tabPath))
                {
                    // create new tab
                    auto newTab = base::CreateSharedPtr<AssetBrowserTabFiles>(AssetBrowserContext::DirectoryTab);
                    newTab->directory(tabDirectory);

                    // load config
                    if (newTab->loadConfig(tabConfig))
                    {
                        auto active = tabConfig.readOrDefault<bool>("Active", false);
                        m_dockContainer->layout().attachPanel(newTab, active);
                    }
                }
            }
        }

        // expand directories
        {
            auto expandedDirectories = config.readOrDefault<base::Array<base::StringBuf>>("ExpandedDirectories");
            for (auto& path : expandedDirectories)
            {
                if (auto dir = m_depot->findPath(path))
                    if (auto index = m_depotTreeModel->findNodeForData(dir))
                        m_depotTree->expandItem(index);
            }
        }

        // select directory
        {
            auto selectedDirectoryPath = config.readOrDefault<base::StringBuf>("SelectedDirectory");
            if (selectedDirectoryPath)
            {
                if (auto dir = m_depot->findPath(selectedDirectoryPath))
                {
                    if (auto index = m_depotTreeModel->findNodeForData(dir))
                    {
                        m_depotTree->ensureVisible(index);
                        m_depotTree->select(index);
                    }
                }
            }
        }

        // make sure something is displayed
        const bool hasFileTabs = m_dockContainer->iterateSpecificPanels<AssetBrowserTabFiles>([](AssetBrowserTabFiles* tab) { return true; });
        if (!hasFileTabs)
        {
            auto root = m_depot->root();
            navigateToDirectory(root);
        }
    }

    void AssetBrowser::saveConfig(ConfigGroup config) const
    {
        // tab configuration
        base::InplaceArray<AssetBrowserTabFiles*, 10> fileTabs;
        m_dockContainer->collectSpecificPanels<AssetBrowserTabFiles>(fileTabs);
        base::InplaceArray<AssetBrowserTabFiles*, 10> activeFileTabs;
        m_dockContainer->collectSpecificPanels<AssetBrowserTabFiles>(activeFileTabs, ui::DockPanelIterationMode::ActiveOnly);

        config.write<uint32_t>("NumTabs", fileTabs.size());
        //config.write<int>("ActiveTab", m_tabs->selectedTab());
        for (uint32_t i=0; i<fileTabs.size(); ++i)
        {
            auto tab = fileTabs[i];

            // store tab type
            auto tabId = base::StringBuf(base::TempString("Tab{}", i));
            auto tabConfig = config[tabId];
            tabConfig.write<base::StringBuf>("Type", "Directory");
            tabConfig.write<base::StringBuf>("ID", tabId);
            tabConfig.write<bool>("Active", activeFileTabs.contains(tab));
            tab->saveConfig(tabConfig);
        }

        // collect opened directories from the depot tree
        {
            base::Array<ui::ModelIndex> indices;
            m_depotTree->collectExpandedItems(indices);

            base::Array<base::StringBuf> expandedFolders;
            for (auto& index : indices)
                if (auto dir = m_depotTreeModel->dataForNode(index))
                    expandedFolders.pushBack(dir->depotPath());

            config.write("ExpandedDirectories", expandedFolders);
        }

        // get the focused folder
        if (auto selected = m_depotTree->current())
            if (auto dir = m_depotTreeModel->dataForNode(selected))
                config.write("SelectedDirectory", dir->depotPath());

        // UI
        //m_config.set("LeftSplitterFrac", m_splitter->splitFraction());
    }

    ManagedFile* AssetBrowser::selectedFile() const
    {
        ManagedFile* ret = nullptr;

        m_dockContainer->iterateSpecificPanels<AssetBrowserTabFiles>([&ret](AssetBrowserTabFiles* tab)
            {
                ret = tab->selectedFile();
                return ret != nullptr;
            }, ui::DockPanelIterationMode::ActiveOnly);

        return ret;
    }

    void AssetBrowser::showFile(ManagedFile* filePtr)
    {
        if (filePtr)
        {
            auto dir = filePtr->parentDirectory();
    
            base::RefPtr<AssetBrowserTabFiles> tab;
            navigateToDirectory(dir, tab);
            if (tab)
                tab->selectItem(filePtr);
        }
    }

    void AssetBrowser::showDirectory(ManagedDirectory* dir, bool exploreContent)
    {
        if (auto index = m_depotTreeModel->findNodeForData(dir))
        {
            m_depotTree->expandItem(index);
            m_depotTree->select(index, ui::ItemSelectionModeBit::Default, exploreContent);
        }
    }

    void AssetBrowser::navigateToDirectory(ManagedDirectory* dir)
    {
        base::RefPtr<AssetBrowserTabFiles> outTab;
        navigateToDirectory(dir, outTab);
    }

    void AssetBrowser::navigateToDirectory(ManagedDirectory* dir, base::RefPtr<AssetBrowserTabFiles>& outTab)
    {
        DEBUG_CHECK(dir != nullptr);
        if (dir == nullptr)
            return;

        base::InplaceArray<AssetBrowserTabFiles*, 10> fileTabs;
        m_dockContainer->collectSpecificPanels<AssetBrowserTabFiles>(fileTabs);

        // switch to tab with that directory
        for (auto* tab : fileTabs)
        {
            if (tab->directory() == dir)
            {
                m_dockContainer->activatePanel(tab);
                outTab = base::RefPtr< AssetBrowserTabFiles>(AddRef(tab));
                return;
            }
        }

        base::InplaceArray<AssetBrowserTabFiles*, 10> activeTabs;
        m_dockContainer->collectSpecificPanels<AssetBrowserTabFiles>(activeTabs, ui::DockPanelIterationMode::ActiveOnly);

        // get the active tab and if it's not locked use it
        for (auto* tab : activeTabs)
        {
            if (tab && !tab->locked())
            {
                tab->directory(dir);
                outTab = base::RefPtr<AssetBrowserTabFiles>(AddRef(tab));
                return;
            }
        }

        // get any non locked tab and change directory to new one
        for (auto* tab : fileTabs)
        {
            if (!tab->locked())
            {
                tab->directory(dir);
                m_dockContainer->activatePanel(tab);
                outTab = base::RefPtr< AssetBrowserTabFiles>(AddRef(tab));
                return;
            }
        }

        // create new tab
        auto newTab = base::CreateSharedPtr<AssetBrowserTabFiles>(AssetBrowserContext::DirectoryTab);
        newTab->directory(dir);
        
        // attach it to the main area
        m_dockContainer->layout().attachPanel(newTab);
        outTab = newTab;
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

    void AssetBrowser::closeDirectoryTab(ManagedDirectory* dir)
    {
        base::InplaceArray<AssetBrowserTabFiles*, 10> fileTabs;
        m_dockContainer->collectSpecificPanels<AssetBrowserTabFiles>(fileTabs);

        for (auto fileTab : fileTabs)
        {
            if (fileTab->directory() == dir)
                fileTab->close();
        }
    }

    void AssetBrowser::managedDepotEvent(ManagedItem* item, ManagedDepotEvent eventType)
    {
        if (eventType == ManagedDepotEvent::DirDeleted)
        {
            closeDirectoryTab(static_cast<ManagedDirectory*>(item));
        }
    }

    //--

} // ed