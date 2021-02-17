/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "base/ui/include/uiSimpleListModel.h"

namespace ed
{
    //--

    // simple list model that lists files/directories with optional comments, files have check boxes so they can be individually selected
    class AssetItemsSimpleListModel : public ui::IAbstractItemModel
    {
    public:
        AssetItemsSimpleListModel();
        virtual ~AssetItemsSimpleListModel();

        //--

        // is the list empty ?
        INLINE bool empty() const { return m_items.empty(); }

        // remove all items from list
        void clear();

        //--

        // remove item from the list
        void removeItem(ManagedItem* item);

        // add item to the list with initial checked flag and comment string
        void addItem(ManagedItem* item, bool checked = true, StringView comment = "");

        // change item comment
        void comment(ManagedItem* item, StringView comment);

        //--

        // check if item is checked
        bool checked(ManagedItem* item) const;
        bool checked(const ui::ModelIndex& item) const;

        // change item check state
        void checked(ManagedItem* item, bool checked) const;
        void checked(const ui::ModelIndex& item, bool checked) const;

        //--

        // export list of items
        Array<ManagedItem*> exportList(bool checkedOnly = true) const;

        //--

    private:
        struct Item : public base::IReferencable
        {
            ui::ModelIndex index;
            ManagedItem* item = nullptr;
            StringBuf name;
            bool checked = true;
            StringBuf comment;
        };

        Array<base::RefPtr<Item>> m_items;
        HashMap<ManagedItem*, Item*> m_itemMap;

        virtual bool hasChildren(const ui::ModelIndex& parent) const override final;
        virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const override final;
        virtual void children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const override final;
        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;
        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;

        ui::ModelIndex index(ManagedItem* item) const;
    };

    //--

    // simple model for list of files (no directories) without checkboxes or anything else
    class AssetPlainFilesSimpleListModel : public ui::IAbstractItemModel
    {
    public:
        AssetPlainFilesSimpleListModel();
        virtual ~AssetPlainFilesSimpleListModel();

        //--

        // is the list empty ?
        INLINE bool empty() const { return m_items.empty(); }

        // remove all items from list
        void clear();

        //--

        // remove file from the list
        void removeFile(ManagedFile* item);

        // add item to the list with initial checked flag and comment string
        void addFile(ManagedFile* item, bool locked);

        //--

        // get file for index
        ManagedFile* file(const ui::ModelIndex& index) const;

        // get index for file
        ui::ModelIndex index(ManagedFile* item) const;

        //--

    private:
        struct Item : public base::IReferencable
        {
            ui::ModelIndex index;
            ManagedFile* file = nullptr;
            bool modified = false;
            bool locked = false;
            StringBuf name;
            StringBuf tags;
            StringBuf directory;
        };

        Array<base::RefPtr<Item>> m_items;
        HashMap<ManagedFile*, Item*> m_itemMap;

        virtual bool hasChildren(const ui::ModelIndex& parent) const override final;
        virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const override final;
        virtual void children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const override final;
        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;
        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;

    };

    //--

} // ed