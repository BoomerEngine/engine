/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: model #]
***/

#pragma once

namespace ui
{

    //---

    template< typename T, typename TV >
    SimpleTreeModel<T, TV>::SimpleTreeModel()
    {}

    template< typename T, typename TV >
    SimpleTreeModel<T, TV>::~SimpleTreeModel()
    {}

    //--

    template< typename T, typename TV >
    typename SimpleTreeModel<T, TV>::Entry* SimpleTreeModel<T, TV>::entryForNode(const ModelIndex& index) const
    {
        if (index.model() != this)
            return nullptr;

        return index.unsafe<Entry>();
    }

    template< typename T, typename TV >
    typename SimpleTreeModel<T, TV>::EntryChildList* SimpleTreeModel<T, TV>::entryChildListForNode(const ModelIndex& index) const
    {
        if (!index)
            return const_cast<SimpleTreeModel<T, TV>::EntryChildList *>(&m_roots);

        auto* entry = entryForNode(index);
        if (nullptr == entry)
            return  nullptr;

        return &entry->m_children;
    }
	
	template< typename T, typename TV >
    TV SimpleTreeModel<T, TV>::dataForNode(const ModelIndex& index, TV defaultValue) const
	{
		const auto* data = dataPtrForNode(index);
		return data ? *data : defaultValue;
	}

    template< typename T, typename TV >
    T* SimpleTreeModel<T, TV>::dataPtrForNode(const ModelIndex& index) const
    {
        auto* entry = entryForNode(index);
        if (entry == nullptr)
            return nullptr;

        return &entry->m_data;
    }

    template< typename T, typename TV >
    ModelIndex SimpleTreeModel<T, TV>::findNodeForData(TV data) const
    {
        return findNodeForData(m_roots, data);
    }

    template< typename T, typename TV >
    ModelIndex SimpleTreeModel<T, TV>::findNodeForData(const EntryChildList& childList, TV data) const
    {
        // local search
        for (uint32_t i = 0; i < childList.m_children.size(); ++i)
        {
            const auto& child = childList.m_children[i];
            if (child->m_data == data)
                return ui::ModelIndex(this, i, 0, child);
        }

        // recruse
        for (uint32_t i = 0; i < childList.m_children.size(); ++i)
        {
            const auto& child = childList.m_children[i];
            if (auto found = findNodeForData(child->m_children, data))
                return found;
        }

        return ModelIndex();
    }

    template< typename T, typename TV >
    ModelIndex SimpleTreeModel<T, TV>::findChildNodeForData(const ModelIndex& parent, TV data) const
    {
        const auto* children = entryChildListForNode(parent);
        if (nullptr == children)
            return ModelIndex();

        for (int i = 0; i <= children->m_children.lastValidIndex(); ++i)
            if (children->m_children[i]->m_data == data)
                return ModelIndex(this, i, 0, children->m_children[i]);

        return ModelIndex();
    }

    //---

    template< typename T, typename TV >
    uint32_t SimpleTreeModel<T, TV>::rowCount(const ModelIndex& parent) const
    {
        const auto* children = entryChildListForNode(parent);
        return children ? children->m_children.size() : 0;
    }

    template< typename T, typename TV >
    bool SimpleTreeModel<T, TV>::hasChildren(const ModelIndex& parent) const
    {
        const auto* children = entryChildListForNode(parent);
        return children ? !children->m_children.empty() : false;
    }

    template< typename T, typename TV >
    bool SimpleTreeModel<T, TV>::hasIndex(int row, int col, const ModelIndex& parent) const
    {
        if (row < 0 || col != 0)
            return false;

        const auto* children = entryChildListForNode(parent);
        return children ? (row <= children->m_children.lastValidIndex()) : false;
    }

    template< typename T, typename TV >
    ModelIndex SimpleTreeModel<T, TV>::parent(const ModelIndex& item) const
    {
        auto* entry = entryForNode(item);
        if (nullptr == entry)
            return ModelIndex();

        if (nullptr == entry->m_parent)
            return ui::ModelIndex();

        auto index = entry->m_parent->m_children.indexOf(entry);
        DEBUG_CHECK_EX(index == item.row(), "Invalid index of child in actual parent");

        auto parentIndex = 0;
        if (entry->m_parent->m_parent)
        {
            parentIndex = entry->m_parent->m_parent->m_children.indexOf(entry->m_parent);
            DEBUG_CHECK_EX(parentIndex != INDEX_NONE, "Item no longer in its parent's childrens list");
        }
        else
        {
            parentIndex = m_roots.indexOf(entry->m_parent);
            DEBUG_CHECK_EX(parentIndex != INDEX_NONE, "Item no longer in root list");
        }

        return ui::ModelIndex(this, parentIndex, 0, entry->m_parent);
    }

    template< typename T, typename TV >
    ModelIndex SimpleTreeModel<T, TV>::index(int row, int column, const ModelIndex& parent) const
    {
        const auto* children = entryChildListForNode(parent);
        if (nullptr == children)
            return ModelIndex();

        if (row > children->m_children.size())
            return ModelIndex();

        return ModelIndex(this, row, column, children->m_children[row]);
    }

    template< typename T, typename TV >
    bool SimpleTreeModel<T, TV>::compare(const ModelIndex& first, const ModelIndex& second, int colIndex) const
    {
        const auto* firstEntry = entryForNode(first);
        const auto* secondEntry = entryForNode(second);
        if (firstEntry && secondEntry)
            compare(firstEntry->m_data, secondEntry->m_data, colIndex);

        return first < second;
    }

    template< typename T, typename TV >
    bool SimpleTreeModel<T, TV>::filter(const ModelIndex& id, const ui::SearchPattern& filterText, int colIndex) const
    {
        auto* entry = entryForNode(id);
        if (nullptr == entry)
            return false;

        return filter(entry->m_data, filterText, colIndex);
    }
    
    template< typename T, typename TV >
    base::StringBuf SimpleTreeModel<T, TV>::displayContent(const ModelIndex& id, int colIndex /*= 0*/) const
    {
        auto* entry = entryForNode(id);
        if (nullptr != entry)
            return displayContent(entry->m_data, colIndex);
        return base::StringBuf::EMPTY();
    }

    template< typename T, typename TV >
    ui::ElementPtr SimpleTreeModel<T, TV>::tooltip(ui::AbstractItemView* view, ui::ModelIndex id) const
    {
        auto* entry = entryForNode(id);
        if (nullptr != entry)
            return tooltip(entry->m_data);
        return nullptr;
    }

    template< typename T, typename TV >
    ui::ElementPtr SimpleTreeModel<T, TV>::tooltip(TV data) const
    {
        return nullptr;
    }

    //--

    template< typename T, typename TV >
    void SimpleTreeModel<T, TV>::clearRoots()
    {
        m_roots = EntryChildList();
        reset();
    }

    template< typename T, typename TV >
    ModelIndex SimpleTreeModel<T, TV>::addRootNode(TV data)
    {
		return addChildNode(ModelIndex(), data);
    }

    template< typename T, typename TV >
    ModelIndex SimpleTreeModel<T, TV>::addChildNode(const ModelIndex& parent, TV data)
    {
		auto* parentEntry = entryForNode(parent);
		auto* parentChildrenList = entryChildListForNode(parent);
		if (!parentChildrenList)
			return ModelIndex();
		
		auto index = parentChildrenList->m_children.size();
		beingInsertRows(parent, index, 0);
		
		auto entry = base::CreateSharedPtr<Entry>();
		entry->m_parent = parentEntry;
		entry->m_data = data;		
		parentChildrenList->m_children.pushBack(entry);

		endInsertRows();
		
		return ModelIndex(this, index, 0, entry);
    }

    template< typename T, typename TV >
    void SimpleTreeModel<T, TV>::removeNode(const ModelIndex& index)
    {
		if (!index)
			return;

		auto* entry = entryForNode(index);
		if (!entry)
			return;
			
		auto parentChildrenList = entryChildListForNode(index.parent());
		if (!parentChildrenList)
			return;
			
        auto childIndex = parentChildrenList->indexOf(entry);
		if (childIndex == -1)
			return;
			
        beingRemoveRows(index.parent(), childIndex, 1);

        parentChildrenList->m_children.erase(childIndex, 1);

        endRemoveRows(); 
    }

    //--

} // ui