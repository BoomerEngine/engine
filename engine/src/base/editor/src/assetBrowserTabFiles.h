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
    
    class AssetBrowserDirContentModel;
    struct AssetItemList;

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
        INLINE bool locked() const { return m_locked; } // locked tab

        INLINE ManagedDirectory* directory() const { return m_dir; }

        INLINE const base::Array<const ManagedFileFormat*>& filterFormats() const { return m_filterFormats.keys(); }
        INLINE const base::StringBuf& filterName() const { return m_filterName; }

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
        void filterName(base::StringView<char> txt);

        /// set active directory
        void directory(ManagedDirectory* dir, ManagedItem* autoSelectItem = nullptr);

        /// select item in the list
        void selectItem(ManagedItem* item);

        /// select specific items
        void selectItems(const base::Array<ManagedItem*>& items);

        /// get selected item (may be file/directory or something else)
        ManagedItem* selectedItem() const;

        /// get selected file
        ManagedFile* selectedFile() const;

        /// get all selected files
        base::Array<ManagedFile*> selectedFiles() const;

        /// get all selected items
        base::Array<ManagedItem*> selectedItems() const;

        /// collect selected items into a list
        void collectItems(AssetItemList& outList, bool resursive) const;

        //--

        void saveConfig(ConfigGroup& config) const;
        bool loadConfig(const ConfigGroup& config);

    private:
        AssetBrowserContext m_context;

        ManagedDirectory* m_dir = nullptr;
        bool m_locked = false;
        bool m_flat = false;
        bool m_list = false;
        uint32_t m_iconSize = 128;

        ui::ListView* m_files;
        base::RefPtr<AssetBrowserDirContentModel> m_filesModel;

        base::HashSet<const ManagedFileFormat*> m_filterFormats;
        base::StringBuf m_filterName;

        void refreshFileList();
        void updateTitle();
        void duplicateTab();

        bool showGenericContextMenu();
        void iconSize(uint32_t size);

        virtual void handleCloseRequest() override;
        virtual ui::IElement* handleFocusForwarding() override;
    };
     

    //--

} // ed
