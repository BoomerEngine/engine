/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#include "build.h"

#include "browserService.h"
#include "browserPanel.h"
#include "browserTreeModel.h"
#include "browserFiles.h"
#include "browserDialogs.h"

#include "engine/ui/include/uiTreeViewEx.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDockContainer.h"
#include "engine/ui/include/uiDockNotebook.h"
#include "engine/ui/include/uiSearchBar.h"
#include "engine/ui/include/uiElementConfig.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

RTTI_BEGIN_TYPE_CLASS(AssetBrowserPanel);
RTTI_END_TYPE();

///--

AssetBrowserPanel::AssetBrowserPanel()
    : IEditorPanel("[img:database] Assets Browser", "AssetBrowser")
    , m_depotEvents(this)
{
    customMinSize(1200, 900);

    m_dockArea = createChild<ui::DockContainer>();
    m_dockArea->layout().notebook()->styleType("FileNotebook"_id);

    {
        m_depotEvents.bind(GetService<DepotService>()->eventKey(), EVENT_DEPOT_DIRECTORY_REMOVED) = [this](StringBuf depotPath)
        {
            closeDirectoryTab(depotPath);
        };
    }

    {
        m_treePanel = RefNew<AssetBrowserTreePanel>();
        m_dockArea->layout().left(0.3f).attachPanel(m_treePanel);

        m_treePanel->bind(EVENT_DEPOT_ACTIVE_DIRECTORY_CHANGED) = [this](StringBuf depotPath)
        {
            navigateToDirectory(depotPath);
        };
    }    
}

AssetBrowserPanel::~AssetBrowserPanel()
{
}

void AssetBrowserPanel::configLoad(const ui::ConfigBlock& block)
{
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

            // load setup
            AssetBrowserTabFilesSetup setup;
            setup.configLoad(tabConfig);
            setup.allowDirs = true;
            setup.allowFileActions = true;

            // create new tab
            auto newTab = RefNew<AssetBrowserTabFiles>(setup);
            newTab->directory(tabPath);
            newTab->configLoad(tabConfig);

            // attach
            auto active = tabConfig.readOrDefault<bool>("Active", false);
            m_dockArea->layout().attachPanel(newTab, active);
        }
    }

    // restore tree
    m_treePanel->configLoad(block.tag("Tree"));

    // make sure something is displayed
    const bool hasFileTabs = m_dockArea->layout().hasPanels<AssetBrowserTabFiles>();
    if (!hasFileTabs)
        navigateToDirectory("/engine/");
}

void AssetBrowserPanel::configSave(const ui::ConfigBlock& block) const
{
    TBaseClass::configSave(block);

    // save depot tree
    m_treePanel->configSave(block.tag("Tree"));

    // tab configuration
    auto fileTabs = m_dockArea->layout().collectPanels<AssetBrowserTabFiles>();
    auto activeTabs = m_dockArea->layout().collectActivePanels<AssetBrowserTabFiles>();

    block.write<uint32_t>("NumTabs", fileTabs.size());
    for (uint32_t i=0; i<fileTabs.size(); ++i)
    {
        auto tab = fileTabs[i];

        // store tab type
        auto tabConfig = block.tag(TempString("Tab{}", i));
        tabConfig.write<StringBuf>("Type", "Directory");
        tabConfig.write<bool>("Active", activeTabs.contains(tab));
        tab->configSave(tabConfig);
    }
}

StringBuf AssetBrowserPanel::selectedFile() const
{
    StringBuf ret;
    m_dockArea->layout().iterateActivePanels<AssetBrowserTabFiles>([&ret](AssetBrowserTabFiles* tab)
        {
            if (!ret && tab->selectedFile())
                ret = tab->selectedFile();
        });

    return ret;
}

StringBuf AssetBrowserPanel::currentDirectory() const
{
    StringBuf ret;
    m_dockArea->layout().iterateActivePanels<AssetBrowserTabFiles>([&ret](AssetBrowserTabFiles* tab)
        {
            if (!ret && tab->depotPath())
                ret = tab->depotPath();
        });

    return ret;
}

bool AssetBrowserPanel::showFile(StringView depotPath)
{
    if (depotPath)
    {
        auto dirPath = depotPath.baseDirectory();
    
        RefPtr<AssetBrowserTabFiles> tab;
        navigateToDirectory(dirPath, &tab);

        if (tab)
            tab->select(depotPath);

        return true;
    }

    return false;
}

void AssetBrowserPanel::showDirectory(StringView depotPath)
{
    m_treePanel->directory(depotPath);
}

void AssetBrowserPanel::navigateToDirectory(StringView depotPath, RefPtr<AssetBrowserTabFiles>* outTab)
{
    // switch to tab with that directory
    auto fileTabs = m_dockArea->layout().collectPanels<AssetBrowserTabFiles>();
    for (const auto& tab : fileTabs)
    {
        if (tab->depotPath() == depotPath)
        {
            m_dockArea->activatePanel(tab);

            if (outTab)
                *outTab = RefPtr<AssetBrowserTabFiles>(tab);

            return;
        }
    }

    // get the active tab and if it's not locked use it
    auto activeTabs = m_dockArea->layout().collectActivePanels<AssetBrowserTabFiles>();
    for (const auto& tab : activeTabs)
    {
        if (tab && !tab->tabLocked())
        {
            tab->directory(depotPath);

            if (outTab)
                *outTab = RefPtr<AssetBrowserTabFiles>(tab);
            return;
        }
    }

    // get any non locked tab and change directory to new one
    for (const auto& tab : fileTabs)
    {
        if (!tab->tabLocked())
        {
            tab->directory(depotPath);
            m_dockArea->activatePanel(tab);

            if (outTab)
                *outTab = RefPtr<AssetBrowserTabFiles>(tab);
            return;
        }
    }

    AssetBrowserTabFilesSetup setup;
    setup.allowDirs = true;
    setup.allowFileActions = true;

    // copy some setup from current tab
    if (!activeTabs.empty())
    {
        const auto& activeSetup = activeTabs[0]->setup();
        setup.list = activeSetup.list;
        setup.iconSize = activeSetup.iconSize;
        setup.thumbnails = activeSetup.thumbnails;
    }

    // create new tab
    auto newTab = RefNew<AssetBrowserTabFiles>(setup);
    newTab->directory(depotPath);
        
    // attach it to the main area
    m_dockArea->layout().attachPanel(newTab);
    if (outTab)
        *outTab = newTab;
}

bool AssetBrowserPanel::handleEditorClose()
{
    // collect files that are modified
    auto modifiedEditors = GetService<AssetBrowserService>()->collectEditors(true);

    // if we have modified files make sure we have a chance to save them, this is also last change to cancel the close
    if (!modifiedEditors.empty())
        if (!SaveEditors(this, modifiedEditors))
            return false; // closing canceled

    // we can close now
    return true;
}

void AssetBrowserPanel::closeDirectoryTab(StringView depotPath)
{
    m_dockArea->layout().iterateAllPanels<AssetBrowserTabFiles>([depotPath](AssetBrowserTabFiles* tab)
        {
            if (tab->depotPath() == depotPath)
                tab->close();
        });
}

//--

END_BOOMER_NAMESPACE_EX(ed)
