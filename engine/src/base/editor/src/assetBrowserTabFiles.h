/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "managedDepot.h"

#include "base/ui/include/uiDockPanel.h"

namespace ed
{

    //--

    DECLARE_UI_EVENT(EVENT_DIRECTORY_CHANGED)

    //--
    
    class AssetBrowserDirContentModel;

    // classic list of files
    class AssetBrowserTabFiles : public ui::DockPanel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserTabFiles, ui::DockPanel);

    public:
        AssetBrowserTabFiles(ManagedDepot* depot, AssetBrowserContext env = AssetBrowserContext::DirectoryTab);
        virtual ~AssetBrowserTabFiles();

        //--

        INLINE AssetBrowserContext context() const { return m_context; }

        INLINE bool list() const { return m_list; } // display items in a flat list
        INLINE bool flat() const { return m_flat; } // flat view (flatten subdirectories)
        INLINE bool locked() const { return m_locked; } // locked tab

        INLINE ManagedDepot* depot() const { return m_depot; }

        INLINE ManagedDirectory* directory() const { return m_dir; }

        INLINE const Array<const ManagedFileFormat*>& filterFormats() const { return m_filterFormats.keys(); }
        INLINE const StringBuf& filterName() const { return m_filterName; }

        //---

        /// toggle the tab lock
        void locked(bool isLocked);

        /// toggle the list flatten
        void flat(bool isFlattened);

        /// toggle the list mode
        void list(bool isList);

        /// set file format filter for resources
        void filterFormat(const ManagedFileFormat* filterFormat, bool toggle);

        /// change the name filter
        void filterName(StringView txt);

        /// set active directory
        void directory(ManagedDirectory* dir, ManagedItem* autoSelectItem = nullptr);

        /// select item in the list
        bool selectItem(ManagedItem* item);

        /// select specific items
        bool selectItems(const Array<ManagedItem*>& items);

        /// get selected item (may be file/directory or something else)
        ManagedItem* selectedItem() const;

        /// get selected file
        ManagedFile* selectedFile() const;

        /// get all selected files
        Array<ManagedFile*> selectedFiles() const;

        /// get all selected items
        Array<ManagedItem*> selectedItems() const;

        //--

        virtual void configLoad(const ui::ConfigBlock& block);
        virtual void configSave(const ui::ConfigBlock& block) const;

    private:
        AssetBrowserContext m_context;

        ManagedDepot* m_depot = nullptr;
        ManagedDirectory* m_dir = nullptr;

        bool m_locked = false;
        bool m_flat = false;
        bool m_list = false;
        uint32_t m_iconSize = 128;

        ui::ListView* m_files;
        RefPtr<AssetBrowserDirContentModel> m_filesModel;

        HashSet<const ManagedFileFormat*> m_filterFormats;
        StringBuf m_filterName;

        Array<ManagedFilePlaceholderPtr> m_filePlaceholders;
        Array<ManagedDirectoryPlaceholderPtr> m_directoryPlaceholders;
        GlobalEventTable m_fileEvents;

        void refreshFileList();
        void updateTitle();
        void duplicateTab();

        bool showGenericContextMenu();
        void iconSize(uint32_t size);

        void createNewDirectory();
        void createNewFile(const ManagedFileFormat* format);
        bool importNewFile(const ManagedFileFormat* format);

        void finishFilePlaceholder(ManagedFilePlaceholderPtr ptr);
        void cancelFilePlaceholder(ManagedFilePlaceholderPtr ptr);
        void finishDirPlaceholder(ManagedDirectoryPlaceholderPtr ptr);
        void cancelDirPlaceholder(ManagedDirectoryPlaceholderPtr ptr);

        void buildNewAssetMenu(ui::MenuButtonContainer* menu);
        void buildImportAssetMenu(ui::MenuButtonContainer* menu);

        virtual void handleCloseRequest() override;        
    };
     
    //--

    extern bool ImportNewFiles(ui::IElement* owner, const ManagedFileFormat* format, ManagedDirectory* parentDir);

    //--

} // ed
