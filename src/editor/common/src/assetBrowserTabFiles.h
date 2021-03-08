/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "managedDepot.h"

#include "engine/ui/include/uiDockPanel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

DECLARE_UI_EVENT(EVENT_DIRECTORY_CHANGED)

//--
    
class AssetBrowserDirContentModel;

// classic list of files
class AssetBrowserTabFiles : public ui::DockPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserTabFiles, ui::DockPanel);

public:
    AssetBrowserTabFiles(AssetBrowserContext env = AssetBrowserContext::DirectoryTab);
    virtual ~AssetBrowserTabFiles();

    //--

    INLINE AssetBrowserContext context() const { return m_context; }

    INLINE bool list() const { return m_list; } // display items in a flat list
    INLINE bool flat() const { return m_flat; } // flat view (flatten subdirectories)

    INLINE const StringBuf& depotPath() const { return m_depotPath; }
    INLINE const StringBuf& filterName() const { return m_filterName; }

    //---

    void flat(bool isFlattened);
    void list(bool isList);

    void filterFormat(ClassType cls, bool toggle);
    void filterName(StringView txt);

    void directory(StringView depotPath, StringView autoSelectName = "");

    bool selectItem(StringView depotPath);

    StringBuf selectedFile() const;

    void duplicateFile(StringView depotPath);

    //--

    virtual void configLoad(const ui::ConfigBlock& block);
    virtual void configSave(const ui::ConfigBlock& block) const;

    //--

private:
    AssetBrowserContext m_context;

    StringBuf m_depotPath;

    bool m_flat = false;
    bool m_list = false;
    uint32_t m_iconSize = 128;

    ui::ListViewExPtr m_files;

    HashSet<ClassType> m_filterFormats;
    StringBuf m_filterName;

    GlobalEventTable m_fileEvents;

    void updateTitle();
    void refreshFileList();
    void duplicateTab();

    bool showGenericContextMenu();
    void iconSize(uint32_t size);

    void createNewDirectoryPlaceholder();
    void createNewDirectory(StringView name);

    void createNewFilePlaceholder(const ManagedFileFormat* format);
    void createNewFile(StringView name, const ManagedFileFormat* format);

    bool importNewFile(const ManagedFileFormat* format);

    void buildNewAssetMenu(ui::MenuButtonContainer* menu);
    void buildImportAssetMenu(ui::MenuButtonContainer* menu);
};
     
//--

extern bool ImportNewFiles(ui::IElement* owner, StringView depotPath, const ManagedFileFormat* format);

//--

END_BOOMER_NAMESPACE_EX(ed)
