/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "engine/ui/include/uiDockPanel.h"
#include "engine/ui/include/uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

DECLARE_UI_EVENT(EVENT_DIRECTORY_CHANGED)

//--

// setup of how we render the files in the tab
struct AssetBrowserTabFilesSetup
{
    bool flat = false; // flat list, all sub dirs are merged into one list (technically can show millions of files, LOL)
    bool small = false; // show small icons
    bool allFiles = false; // show all files, not only the meta files
    bool list = false; // show as a classic list instead of icons
    bool thumbnails = true; // allow/disallow thumbnails
    int iconSize = 100; // size of the icons

    bool allowDirs = true; // allow directories to be displayed
    bool allowFileActions = false; // allow file actions (delete, create, etc), disabled for ad-hoc browsers

    void configLoad(const ui::ConfigBlock& block);
    void configSave(const ui::ConfigBlock& block) const;
};

//--
    
// classic list of files
class AssetBrowserTabFiles : public ui::DockPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserTabFiles, ui::DockPanel);

public:
    AssetBrowserTabFiles(const AssetBrowserTabFilesSetup& setup);
    virtual ~AssetBrowserTabFiles();

    //--

    INLINE const AssetBrowserTabFilesSetup& setup() const { return m_setup; }

    INLINE const StringBuf& depotPath() const { return m_depotPath; }

    //--

    void setup(const AssetBrowserTabFilesSetup& setup); // may refresh the whole list

    //--

    void directory(StringView depotPath);
    void select(StringView depotPath);

    StringBuf selectedFile() const;

    Array<StringBuf> selectedFiles(bool* outHasAlsoDirsSelected=nullptr) const; // NOTE: files only but we can get info if 
    Array<StringBuf> selectedDirs() const;
    Array<StringBuf> selectedItems() const; // NOTE: files and dirs, dirs are first

    //--

    virtual void configLoad(const ui::ConfigBlock& block);
    virtual void configSave(const ui::ConfigBlock& block) const;

    //--

private:
    StringBuf m_depotPath;

    AssetBrowserTabFilesSetup m_setup;

    ui::ListViewExPtr m_files;
    
    ui::ToolBarPtr m_toolbar;

    //--

    void initToolbar();
    void initList();
    void initShortcuts();

    void updateToolbar();

    void cmdRefreshFileList();
    void cmdLockTab();
    void cmdBookmarkTab();
    void cmdToggleListView();
    void cmdToggleFlatView();
    void cmdImportHere();
    void cmdCreateHere();
    void cmdNavigateBack();
    void cmdDuplicateTab();
    void cmdCloseTab();
    void cmdDeleteSelection();
    void cmdDuplicateAssets();
    void cmdCopySelection();
    void cmdCutSelection();
    void cmdPasteSelection();
    void cmdShowInFiles();
    void cmdShowSourceAsset();

    //--

    StringBuf m_itemToSelect;
    NativeTimePoint m_itemToSelectTimeout;

    ui::Timer m_internalTimer;

    void clearPendingSelectItem();
    void trySelectItem();

    //--

    void updateTitle();
    
    //--

    void refreshFileList();

    void createNewDirectoryPlaceholder();
    void createNewDirectory(StringView name);

    void createNewFilePlaceholder(ResourceClass format);
    bool createNewFile(StringView name, ResourceClass format);

    bool importNewFile(ResourceClass format);

    void buildNewAssetMenu(ui::MenuButtonContainer* menu);
    void buildImportAssetMenu(ui::MenuButtonContainer* menu);

    bool shouldShowFile(StringView depotPath) const;

    void collectFlatFileItems(StringView depotPath, ui::ListViewEx* list, HashSet<StringBuf>& outObservedDirs) const;
    void collectNormalFileItems(StringView depotPath, ui::ListViewEx* list, HashSet<StringBuf>& outObservedDirs) const;

    //--

    void buildContextMenu(ui::MenuButtonContainer& menu);
    bool buildFilesContextMenu(ui::MenuButtonContainer& menu);
    void buildGenericContextMenu(ui::MenuButtonContainer& menu);

    void buildSingleFileContextMenu(ui::MenuButtonContainer& menu, const StringBuf& depotPath);
    void buildSingleDirContextMenu(ui::MenuButtonContainer& menu, const StringBuf& depotPath);
    void buildManyFilesContextMenu(ui::MenuButtonContainer& menu, const Array<StringBuf>& depotPaths);
    void buildMixedItemsContextMenu(ui::MenuButtonContainer& menu);
    

    //--

    GlobalEventTable m_fileEvents;
    HashSet<StringBuf> m_observerDepotDirectories;

    void bindFileSystemEvents();

    void processEventFileAdded(StringBuf depotPath);
    void processEventFileRemoved(StringBuf depotPath);
    void processEventDirectoryAdded(StringBuf depotPath);
    void processEventDirectoryRemoved(StringBuf depotPath);

    void processEventDirectoryBookmarked(StringBuf depotPath);
    void processEventFileMarked(StringBuf depotPath);

    //--
};
     
//--

END_BOOMER_NAMESPACE_EX(ed)
