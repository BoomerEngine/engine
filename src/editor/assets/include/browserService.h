/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "engine/ui/include/uiDragDrop.h"
#include "engine/ui/include/uiWindow.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///--

DECLARE_GLOBAL_EVENT(EVENT_DEPOT_DIRECTORY_BOOKMARKED, StringBuf)
DECLARE_GLOBAL_EVENT(EVENT_DEPOT_ASSET_MARKED, StringBuf)

class IResourceContainerWindow;

///---

/// drag&drop data with file
class EDITOR_ASSETS_API AssetBrowserFileDragDrop : public ui::IDragDropData
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserFileDragDrop, ui::IDragDropData);

public:
    AssetBrowserFileDragDrop(StringView depotPath);

    INLINE const StringBuf& depotPath() const { return m_depotPath; }

private:
    virtual ui::ElementPtr createPreview() const override;

    StringBuf m_depotPath;
};

//---

/// drag&drop data with file
class EDITOR_ASSETS_API AssetBrowserDirectoryDragDrop : public ui::IDragDropData
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserDirectoryDragDrop, ui::IDragDropData);

public:
    AssetBrowserDirectoryDragDrop(StringView depotPath);

    INLINE const StringBuf& depotPath() const { return m_depotPath; }

private:
    virtual ui::ElementPtr createPreview() const override;

    StringBuf m_depotPath;
};

///--

/// assert browser tab
class EDITOR_ASSETS_API AssetBrowserService : public IService
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserService, IService);

public:
    AssetBrowserService();
    virtual ~AssetBrowserService();

    //--

    // get selected file
    StringBuf selectedFile() const;

    // get current directory
    StringBuf currentDirectory() const;

    //--

    // show asset browser window without showing anything else
    void showWindow(bool activate=true);

    // show file in the asset browser
    void showFile(StringView depotPath);

    // show directory in the depot tree and possible also as a file list
    void showDirectory(StringView depotDirPath);

    //--

    // is this directory bookmarked ?
    bool checkDirectoryBookmark(StringView depotPath) const;

    // toggle bookmark on a directory
    // NOTE: emits EVENT_DIRECTORY_BOOKMARKED
    void toggleDirectoryBookmark(StringView depotPath, bool state);

    //--

    // clear marked files
    void clearMarkedFiles();

    // mark following files for copy action
    void markFiles(const Array<StringBuf>& paths, int tag);

    // get all marked files
    void markedFiles(Array<StringBuf>& outFiles, int& outTag);

    // check if file is marked
    bool checkFileMark(StringView path, int& outTag) const;

    //--

    void configLoad(const ui::ConfigBlock& block);
    void configSave(const ui::ConfigBlock& block) const;

    //--

    // find opened editor for a given file
    ResourceEditorPtr findEditor(StringView depotPath) const;

    // open editor for given file
    ResourceEditorPtr openEditor(ui::IElement* owner, StringView depotPath, bool focus=true);

    // collect all opened resource editors
    Array<ResourceEditorPtr> collectEditors(bool modifiedOnly = false) const;
        
    //--

private:
    GlobalEventTable m_depotEvents;

    //--

    HashSet<StringBuf> m_bookmarkedDirectories;

    HashSet<StringBuf> m_markedDepotFiles;
    int m_markedDepotFilesTag = 0;

    ui::ConfigBlock config();

    //--

    virtual bool onInitializeService(const CommandLine& cmdLine) override;
    virtual void onShutdownService() override;
    virtual void onSyncUpdate() override;
};

///--

END_BOOMER_NAMESPACE_EX(ed)


