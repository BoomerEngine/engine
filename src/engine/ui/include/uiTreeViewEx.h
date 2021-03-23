/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\view #]
***/

#pragma once

#include "uiCollectionView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

/// item in the tree view
class ENGINE_UI_API ITreeItem : public ICollectionItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(ITreeItem, ICollectionItem);

public:
    ITreeItem();
    virtual ~ITreeItem();

    //--

    INLINE bool expanded() const { return m_expanded; }

    INLINE const Array<TreeItemPtr>& children() const { return m_children; }

    INLINE RefWeakPtr<ITreeItem> parent() const { return m_parent; }

    void toggleExpand();

    void expand();
    void collapse();

    void expandAll();
    void collapseAll();

    //--

    void addChild(ITreeItem* child);
    void removeChild(ITreeItem* child);

    void removeAllChildren();

    //--

    virtual bool handleItemCanExpand() const;
    virtual void handleItemExpand();
    virtual void handleItemCollapse();

    virtual bool handleKeyEvent(const InputKeyEvent& evt) override;

    //--

    template< typename T >
    INLINE void visit(const std::function<void(T*)>& func)
    {
        if (auto item = rtti_cast<T>(this))
            func(item);

        if (m_expanded)
            for (const auto& child : m_children)
                child->visit(func);
    }

    /// find first item matching given predicate
    template< typename T >
    INLINE RefPtr<T> find(const std::function<bool(T*)>& func) const
    {
        if (auto item = rtti_cast<T>(this))
            if (!func || func((T*)item))
                return AddRef(item);

        for (const auto& child : m_children)
            if (auto ret = child->find(func))
                return ret;

        return nullptr;
    }

private:
    bool m_expanded = false;
    ButtonPtr m_expandButton;

    Array<TreeItemPtr> m_children;
    RefWeakPtr<ITreeItem> m_parent;

    int m_displayDepth = 0;

    void updateDisplayDepth(int depth);

    friend class TreeViewEx;

protected:
    virtual void updateButtonState();
};

//---

/// tree view, used for items that can have children
class ENGINE_UI_API TreeViewEx : public ICollectionView
{
    RTTI_DECLARE_VIRTUAL_CLASS(TreeViewEx, ICollectionView);

public:
    TreeViewEx();

    ///---

    /// remove all roots
    void clear();

    /// add root item
    void addRoot(ITreeItem* root);

    /// remove root item
    void removeRoot(ITreeItem* root);

    /// remove particular item
    void remove(ITreeItem* item);

    ///---

    /// find first item matching given predicate
    template< typename T >
    INLINE RefPtr<T> find(const std::function<bool(T*)>& func) const
    {
        for (const auto* root : m_roots)
            if (auto ret = static_cast<ITreeItem*>(root->item.get())->find<T>(func))
                return ret;

        return nullptr;
    }
    
    /// visit (pre order) all expended items
    template< typename T >
    INLINE void visit(const std::function<void(T*)>& func) const
    {
        for (const auto* root : m_roots)
            static_cast< ITreeItem*>(root->item.get())->visit<T>(func);
    }

    ///---

private:
    Array<ViewItem*> m_roots;

    friend class ITreeItem;

    virtual void internalAttachItem(ViewItem* item) override;
    virtual void internalDetachItem(ViewItem* item) override;

    virtual void rebuildDisplayList() override;

    virtual bool processActivation() override;

    bool removeParticularViewItemRecursive(ITreeItem* cur, ITreeItem* parent, ITreeItem* itemToRemove);

    void collectViewItems(ViewItem* item, int depth, Array<ViewItem*>& outItems);
};

//---

END_BOOMER_NAMESPACE_EX(ui)

