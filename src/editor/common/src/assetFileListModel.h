/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "engine/ui/include/uiAbstractItemModel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

// list model for directory content
class AssetBrowserDirContentModel : public ui::IAbstractItemModel
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserDirContentModel, ui::IAbstractItemModel);

public:
    AssetBrowserDirContentModel(ManagedDepot* depot);
    virtual ~AssetBrowserDirContentModel();

    //--

    INLINE ManagedDepot* depot() { return m_depot; }

    INLINE uint32_t iconSize() { return m_iconSize; }
    INLINE void iconSize(uint32_t size) { m_iconSize = size; }

    INLINE ManagedDirectory* rootDir() const { return m_rootDir; }
    INLINE const HashSet<ManagedDirectory*>& observedDirs() const { return m_observedDirs; }

    //--

    ManagedItem* item(const ui::ModelIndex& index) const;
    ManagedDirectory* directory(const ui::ModelIndex& index) const;
    ManagedFile* file(const ui::ModelIndex& index) const;

    Array<ManagedFile*> files(const Array<ui::ModelIndex>& indices) const;
    Array<ManagedItem*> items(const Array<ui::ModelIndex>& indices) const;

    ui::ModelIndex first() const;
    ui::ModelIndex index(const ManagedItem* ptr) const;

    //--

    void initializeFromDir(ManagedDirectory* dir, bool flat);

    //--

    ui::ModelIndex addAdHocElement(ManagedItem* item);
    void removeAdHocElement(ManagedItem* item);

private:
    // ui::IAbstractItemModel
    virtual bool hasChildren(const ui::ModelIndex& parent) const override final;
    virtual void children(const ui::ModelIndex& parent, Array<ui::ModelIndex>& outChildrenIndices) const override final;
    virtual ui::ModelIndex parent(const ui::ModelIndex& item) const override final;
    virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex = 0) const override final;
    virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override final;
    virtual StringBuf displayContent(const ui::ModelIndex& item, int colIndex = 0) const override final;
    virtual ui::PopupPtr contextMenu(ui::AbstractItemView* view, const Array<ui::ModelIndex>& indices) const override final;
    virtual ui::DragDropDataPtr queryDragDropData(const input::BaseKeyFlags& keys, const ui::ModelIndex& item) override final;
    virtual void visualize(const ui::ModelIndex& item, int columnCount, ui::ElementPtr& content) const override final;

    static const int TYPE_PARENT_DIRECTORY = 1;
    static const int TYPE_DIRECTORY = 2;
    static const int TYPE_ADHOC_DIRECTORY = 3;
    static const int TYPE_FILE = 4;
    static const int TYPE_ADHOC_FILE = 5;

    static int TypeFromItem(const ManagedItem* item);

    struct Entry : public IReferencable
    {
        int itemType = 0;
        ui::ModelIndex index;
        ManagedItem* item;
        StringBuf customName;
    };

    Array<RefPtr<Entry>> m_items;
    HashSet<ManagedDirectory*> m_observedDirs;
    ManagedDirectory* m_rootDir = nullptr;

    ManagedDepot* m_depot = nullptr;
    GlobalEventTable m_depotEvents;

    Array<ManagedItemPtr> m_adHocElements;

    uint32_t m_iconSize = 128;

    ui::ModelIndex handleCreateItemRepresentation(const ManagedItemPtr& item);
    void handleDestroyItemRepresentation(const ManagedItemPtr& item);

    bool canDisplayItem(const ManagedItem* item) const;
    bool canDisplayFile(const ManagedFile* file) const;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
