/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "base/ui/include/uiAbstractItemModel.h"
#include "base/ui/include/uiSimpleListModel.h"

#include "managedDepot.h"

namespace ed
{
    //--

    class AssetBrowserFileEntry;
    class AssetBrowserDirectoryEntry;

    //--
    
    // list model for directory content
    class AssetBrowserDirContentModel : public ui::IAbstractItemModel, public IManagedDepotListener
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserDirContentModel, ui::IAbstractItemModel);

    public:
        AssetBrowserDirContentModel();
        virtual ~AssetBrowserDirContentModel();

        //--

        INLINE uint32_t iconSize() { return m_iconSize; }
        INLINE void iconSize(uint32_t size) { m_iconSize = size; }

        INLINE ManagedDirectory* rootDir() const { return m_rootDir; }
        INLINE const base::HashSet<ManagedDirectory*>& observedDirs() const { return m_observedDirs; }

        //--

        ManagedItem* item(const ui::ModelIndex& index) const;
        ManagedDirectory* directory(const ui::ModelIndex& index) const;
        ManagedFile* file(const ui::ModelIndex& index) const;

        base::Array<ManagedFile*> files(const base::Array<ui::ModelIndex>& indices) const;
        base::Array<ManagedItem*> items(const base::Array<ui::ModelIndex>& indices) const;

        void collectItems(const base::Array<ui::ModelIndex>& indices, AssetItemList& outList, bool resursive) const;

        ui::ModelIndex index(const ManagedItem* ptr) const;

        //--

        void initializeFromDir(ManagedDirectory* dir, bool flat);

        //--

    private:
        // depot:IManagedDepotListener
        virtual void managedDepotEvent(ManagedItem* item, ManagedDepotEvent eventType) override final;

        // ui::IAbstractItemModel
        virtual uint32_t rowCount(const ui::ModelIndex& parent) const override final;
        virtual bool hasChildren(const ui::ModelIndex& parent) const override final;
        virtual bool hasIndex(int row, int col, const ui::ModelIndex& parent) const override final;
        virtual ui::ModelIndex parent(const ui::ModelIndex& item) const override final;
        virtual ui::ModelIndex index(int row, int column, const ui::ModelIndex& parent) const override final;
        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;
        virtual base::StringBuf displayContent(const ui::ModelIndex& item, int colIndex = 0) const override final;
        virtual ui::PopupPtr contextMenu(ui::AbstractItemView* view, const base::Array<ui::ModelIndex>& indices) const override final;
        virtual ui::DragDropDataPtr queryDragDropData(const base::input::BaseKeyFlags& keys, const ui::ModelIndex& item) override final;
        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;

        struct Entry : public base::IReferencable
        {
            int m_itemType;
            ManagedItem* m_item;
            base::StringBuf m_customName;
        };

        base::Array<base::RefPtr<Entry>> m_items;
        base::HashSet<ManagedDirectory*> m_observedDirs;
        ManagedDirectory* m_rootDir = nullptr;

        uint32_t m_iconSize = 128;

        bool canDisplayItem(const ManagedItem* item) const;
        bool canDisplayFile(const ManagedFile* file) const;
    };

    //--

    // general visualization element for file list
    class AssetBrowserFileVisItem : public ui::IElement
    {
    public:
        AssetBrowserFileVisItem(ManagedItem* item, uint32_t size, const base::StringView<char> customText = "");

        INLINE ManagedItem* item() const { return m_item; }

        void refreshFileData();
        void resizeIcon(uint32_t size);

    protected:
        ui::ImagePtr m_icon;
        ui::TextLabelPtr m_label;
        ui::TextLabelPtr m_ext;
        base::StringBuf m_name;

        ManagedItem* m_item;
        uint32_t m_size;
    };

    //--

    class AssetItemsSimpleListModel : public ui::SimpleListModel
    {
    public:
        AssetItemsSimpleListModel();
        virtual ~AssetItemsSimpleListModel();

        void clear();

        virtual uint32_t size() const override final;

        void removeItem(ManagedItem* item);
        void addItem(ManagedItem* item, bool checked = true, base::StringView<char> comment = "");
        void comment(ManagedItem* item, base::StringView<char> comment);

        bool checked(ManagedItem* item) const;
        void checked(ManagedItem* item, bool checked) const;

        bool checked(const ui::ModelIndex& item) const;
        void checked(const ui::ModelIndex& item, bool checked) const;

        Array<ManagedItem*> checked() const;

    private:
        struct Item
        {
            ManagedItem* item = nullptr;
            base::StringBuf name;
            bool checked = true;
            base::StringBuf comment;
        };

        base::Array<Item*> m_items;
        base::HashMap<ManagedItem*, int> m_itemMap;

        virtual base::StringBuf content(const ui::ModelIndex& id, int colIndex /*= 0*/) const override final;
        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;

        ui::ModelIndex index(int index) const;
        ui::ModelIndex index(ManagedItem* item) const;
    };

} // ed