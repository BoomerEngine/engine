/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#pragma once

#include "uiElement.h"

namespace ui
{
    ///---

    class DataInspector;
    class DataProperty;

    ///---

    /// data inspector navigation item
    class BASE_UI_API DataInspectorNavigationItem : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataInspectorNavigationItem, IElement);

    public:
        DataInspectorNavigationItem(DataInspector* inspector, DataInspectorNavigationItem* parentNavigation, const base::StringBuf& path, const base::StringBuf& caption);

        //--

        // get the path so far in the view
        INLINE const base::StringBuf& path() const { return m_path; };
        
        // get the display caption
        INLINE const base::StringBuf& caption() const { return m_caption; }

        // get the parent navigation item
        INLINE DataInspectorNavigationItem* parentItem() const { return m_parentItem; }

        // get list of child items
        INLINE const base::Array<base::RefPtr<DataInspectorNavigationItem>>& childrenItems() const { return m_children; }

        // get the data inspector we belong to
        INLINE DataInspector* inspector() const { return m_inspector; }

        /// is this expandable item ?
        INLINE bool expandable() const { return m_expandable; }

        /// is this item currently expanded ?
        INLINE bool expanded() const { return m_expanded; }

        //--

        /// collapse element
        void collapse();

        /// expand element
        void expand();

        /// collect items (hierarchy)
        void collect(base::Array<DataInspectorNavigationItem*>& outItems);

        //--

        /// hit test
        DataInspectorNavigationItem* itemAtPosition(const Position& pos);

        //--

        /// change selection state
        virtual void handleSelectionLost();
        virtual void handleSelectionGain(bool focus);

    private:
        DataInspector* m_inspector;

        DataInspectorNavigationItem* m_parentItem;
        base::Array<base::RefPtr<DataInspectorNavigationItem>> m_children;

        base::StringBuf m_path;
        base::StringBuf m_caption;
        bool m_selected = false;
        bool m_expanded = false;
        bool m_expandable = false;

    protected:
        virtual void createChildren(base::Array<base::RefPtr< DataInspectorNavigationItem>>& outCreatedChildren);
        virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;

        ButtonPtr createExpandButton();
        void updateExpandStyle();
        void changeExpandable(bool expandable);

        ButtonPtr m_expandButton;
    };

    ///---

    /// big group
    class BASE_UI_API DataInspectorGroup : public DataInspectorNavigationItem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataInspectorGroup, DataInspectorNavigationItem);

    public:
        DataInspectorGroup(DataInspector* inspector, DataInspectorNavigationItem* parent, const base::StringBuf& path, const base::StringBuf& name);
    };

    ///---

    /// big group of properties of the same category
    class BASE_UI_API DataInspectorObjectCategoryGroup : public DataInspectorGroup
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataInspectorObjectCategoryGroup, DataInspectorGroup);

    public:
        DataInspectorObjectCategoryGroup(DataInspector* inspector, DataInspectorNavigationItem* parent, const base::StringBuf& path, base::StringID category);

    private:
        base::StringID m_category;
        
        virtual void createChildren(base::Array<base::RefPtr<DataInspectorNavigationItem>>& outCreatedChildren) override;
    };

    ///---

} // ui