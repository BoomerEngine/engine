/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: model #]
***/

#pragma once

#include "uiAbstractItemModel.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

/// a tree model for a simple tree
template< typename T, typename TV = const T& >
class SimpleTreeModel : public IAbstractItemModel
{
public:
    SimpleTreeModel();
    ~SimpleTreeModel();

    //--

    /// resolve data in the item, return null if index is not valid
    T* dataPtrForNode(const ModelIndex& index) const;

    /// resolve data in the item, return null if index is not valid
    TV dataForNode(const ModelIndex& index, TV defaultValue = T()) const;
		
    /// get index for model entry, only works if the data is unique
    /// NOTE: generic version is very slow, as it goes over the whole tree, can be override to provide faster access, if possible implement the getParentData
    ModelIndex findNodeForData(TV data) const;

    /// find a child with given data
    ModelIndex findChildNodeForData(const ModelIndex& parent, TV data) const;
		
	//---
		
	/// resolve parent data for given data, ie - a parent directory for a directory, etc, allows some operations to run faster but it's not necessary
	virtual bool parentData(const ModelIndex& index, TV data, T& outParentData) const { return false; }
		
		
    //--

    /// compare items by data
    virtual bool compare(TV a, TV b, int colIndex) const = 0;

    /// filter items by data
    virtual bool filter(TV data, const SearchPattern& filter, int colIndex = 0) const = 0;

    /// get display content for element
    virtual StringBuf displayContent(TV data, int colIndex = 0) const = 0;

    /// tooltip element
    virtual ElementPtr tooltip(TV data) const;

    //--

    /// remove all roots
    void clearRoots();

    /// add root to the tree, returns model index (NOTE: model indices should not be cached)
    ModelIndex addRootNode(TV data);

    /// add child node
    ModelIndex addChildNode(const ModelIndex& parent, TV data);

    /// remove node and all children
    void removeNode(const ModelIndex& index);

protected:
    virtual bool hasChildren(const ModelIndex& parent) const override;
    virtual ModelIndex parent(const ModelIndex& item) const override;
    virtual void children(const ModelIndex& parent, Array<ModelIndex>& outChildrenIndices) const override;
    virtual bool compare(const ModelIndex& first, const ModelIndex& second, int colIndex = 0) const override;
    virtual bool filter(const ModelIndex& id, const SearchPattern& filter, int colIndex = 0) const override;
    virtual StringBuf displayContent(const ModelIndex& id, int colIndex = 0) const override;
    virtual ElementPtr tooltip(AbstractItemView* view, ModelIndex id) const override;

    struct Entry;

    typedef Array<RefPtr<Entry>> EntryChildList;

    struct Entry : public IReferencable
    {
        T data;
        ModelIndex index;
        Entry* parent = nullptr;
        EntryChildList children;
    };

    EntryChildList m_roots;

    EntryChildList* entryChildListForNode(const ModelIndex& index) const;
    Entry* entryForNode(const ModelIndex& index) const;
    ModelIndex findNodeForData(const EntryChildList& childList, TV data) const;
};

END_BOOMER_NAMESPACE_EX(ui)

#include "uiSimpleTreeModel.inl"