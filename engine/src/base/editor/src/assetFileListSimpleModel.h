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

    // simple list model that lists files/directories with optional comments, files hanve checkboxes so they can be individually selected
    class AssetItemsSimpleListModel : public ui::SimpleListModel
    {
    public:
        AssetItemsSimpleListModel();
        virtual ~AssetItemsSimpleListModel();

        //--

        // remove all items from list
        void clear();

        // get number of elements in the list
        virtual uint32_t size() const override final;

        //--

        // remove item from the list
        void removeItem(ManagedItem* item);

        // add item to the list with initial checked flag and comment stirng
        void addItem(ManagedItem* item, bool checked = true, StringView<char> comment = "");

        // change item comment
        void comment(ManagedItem* item, StringView<char> comment);

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
        struct Item
        {
            ManagedItem* item = nullptr;
            StringBuf name;
            bool checked = true;
            StringBuf comment;
        };

        Array<Item*> m_items;
        HashMap<ManagedItem*, int> m_itemMap;

        virtual StringBuf content(const ui::ModelIndex& id, int colIndex /*= 0*/) const override final;
        virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;

        Item* itemForIndex(const ui::ModelIndex& index) const;

        ui::ModelIndex index(int index) const;
        ui::ModelIndex index(ManagedItem* item) const;
    };

    //--

} // ed