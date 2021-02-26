/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: model #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(ui)

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

    return &entry->children;
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

    return &entry->data;
}

template< typename T, typename TV >
ModelIndex SimpleTreeModel<T, TV>::findNodeForData(TV data) const
{
    return findNodeForData(m_roots, data);
}

template< typename T, typename TV >
ModelIndex SimpleTreeModel<T, TV>::findNodeForData(const EntryChildList& childList, TV data) const
{
    for (const auto& child : childList)
        if (child->data == data)
            return child->index;

    for (const auto& child : childList)
        if (auto found = findNodeForData(child->children, data))
            return found;

    return ModelIndex();
}

template< typename T, typename TV >
ModelIndex SimpleTreeModel<T, TV>::findChildNodeForData(const ModelIndex& parent, TV data) const
{
    if (const auto* children = entryChildListForNode(parent))
        for (const auto& child : *children)
            if (child->data == data)
                return child->index;

    return ModelIndex();
}

//---

template< typename T, typename TV >
void SimpleTreeModel<T, TV>::children(const ModelIndex& parent, Array<ModelIndex>& outChildrenIndices) const
{
    if (const auto* children = entryChildListForNode(parent))
    {
        outChildrenIndices.reserve(children->size());
        for (const auto& item : *children)
            outChildrenIndices.pushBack(item->index);
    }
}

template< typename T, typename TV >
bool SimpleTreeModel<T, TV>::hasChildren(const ModelIndex& parent) const
{
    const auto* children = entryChildListForNode(parent);
    return children && !children->empty();
}

template< typename T, typename TV >
ModelIndex SimpleTreeModel<T, TV>::parent(const ModelIndex& item) const
{
    auto* entry = entryForNode(item);
    if (nullptr == entry)
        return ModelIndex();

    if (nullptr == entry->parent)
        return ModelIndex();

    return entry->parent->index;
}

template< typename T, typename TV >
bool SimpleTreeModel<T, TV>::compare(const ModelIndex& first, const ModelIndex& second, int colIndex) const
{
    const auto* firstEntry = entryForNode(first);
    const auto* secondEntry = entryForNode(second);
    if (firstEntry && secondEntry)
        compare(firstEntry->data, secondEntry->data, colIndex);

    return first < second;
}

template< typename T, typename TV >
bool SimpleTreeModel<T, TV>::filter(const ModelIndex& id, const SearchPattern& filterText, int colIndex) const
{
    auto* entry = entryForNode(id);
    if (nullptr == entry)
        return false;

    return filter(entry->data, filterText, colIndex);
}
    
template< typename T, typename TV >
StringBuf SimpleTreeModel<T, TV>::displayContent(const ModelIndex& id, int colIndex /*= 0*/) const
{
    auto* entry = entryForNode(id);
    if (nullptr != entry)
        return displayContent(entry->data, colIndex);
    return StringBuf::EMPTY();
}

template< typename T, typename TV >
ElementPtr SimpleTreeModel<T, TV>::tooltip(AbstractItemView* view, ModelIndex id) const
{
    auto* entry = entryForNode(id);
    if (nullptr != entry)
        return tooltip(entry->data);
    return nullptr;
}

template< typename T, typename TV >
ElementPtr SimpleTreeModel<T, TV>::tooltip(TV data) const
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
		
	auto entry = RefNew<Entry>();
    entry->index = ModelIndex(this, entry);
	entry->parent = parentEntry;
	entry->data = data;
	parentChildrenList->pushBack(entry);

    notifyItemAdded(parent, entry->index);
		
	return entry->index;
}

template< typename T, typename TV >
void SimpleTreeModel<T, TV>::removeNode(const ModelIndex& index)
{
	if (!index)
		return;

	auto* entry = entryForNode(index);
	if (!entry)
		return;
			
    auto& parentChildrenList = entry->parent ? entry->parent->children : m_roots;
    auto indexInParent = parentChildrenList.find(index.unsafe<Entry>());
    if (indexInParent == INDEX_NONE)
        return;

    auto elementRef = parentChildrenList[indexInParent];
    parentChildrenList.erase(indexInParent);

    notifyItemRemoved(entry->parent ? entry->parent->index : ModelIndex(), index);
}

//--

END_BOOMER_NAMESPACE_EX(ui)
