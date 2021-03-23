/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

///---

class DataInspector;
class DataProperty;

///---

/// data inspector navigation item
class ENGINE_UI_API DataInspectorNavigationItem : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataInspectorNavigationItem, IElement);

public:
    DataInspectorNavigationItem(DataInspector* inspector, DataInspectorNavigationItem* parentNavigation, const StringBuf& path, const StringBuf& caption);

    //--

    // get the path so far in the view
    INLINE const StringBuf& path() const { return m_path; };
        
    // get the display caption
    INLINE const StringBuf& caption() const { return m_caption; }

    // get the parent navigation item
    INLINE DataInspectorNavigationItem* parentItem() const { return m_parentItem; }

    // get list of child items
    INLINE const Array<RefPtr<DataInspectorNavigationItem>>& childrenItems() const { return m_children; }

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
    void collect(Array<DataInspectorNavigationItem*>& outItems);

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
    Array<RefPtr<DataInspectorNavigationItem>> m_children;

    StringBuf m_path;
    StringBuf m_caption;
    bool m_selected = false;
    bool m_expanded = false;
    bool m_expandable = false;

protected:
    virtual void createChildren(Array<RefPtr< DataInspectorNavigationItem>>& outCreatedChildren);
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt) override;

    ButtonPtr createExpandButton();
    void updateExpandStyle();
    void changeExpandable(bool expandable);

    ButtonPtr m_expandButton;
};

///---

/// big group
class ENGINE_UI_API DataInspectorGroup : public DataInspectorNavigationItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataInspectorGroup, DataInspectorNavigationItem);

public:
    DataInspectorGroup(DataInspector* inspector, DataInspectorNavigationItem* parent, const StringBuf& path, const StringBuf& name);
};

///---

/// big group of properties of the same category
class ENGINE_UI_API DataInspectorObjectCategoryGroup : public DataInspectorGroup
{
    RTTI_DECLARE_VIRTUAL_CLASS(DataInspectorObjectCategoryGroup, DataInspectorGroup);

public:
    DataInspectorObjectCategoryGroup(DataInspector* inspector, DataInspectorNavigationItem* parent, const StringBuf& path, StringID category);

private:
    StringID m_category;
        
    virtual void createChildren(Array<RefPtr<DataInspectorNavigationItem>>& outCreatedChildren) override;
};

///---

END_BOOMER_NAMESPACE_EX(ui)
