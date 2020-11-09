/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: model #]
***/

#pragma once

#include "base/input/include/inputStructures.h"

namespace ui
{
    //---

    /// non-persistent index in the model, based on Qt's idea
    /// NOTE: the model indices are transient and should not be stored as they may become invalid
    class BASE_UI_API ModelIndex
    {
    public:
        INLINE ModelIndex(); // empty
        ModelIndex(const IAbstractItemModel* model, const base::UntypedRefWeakPtr& ptr = nullptr, uint64_t uniqueIndex = 0); // non-indexed model index - marks actual object

        INLINE ModelIndex(const ModelIndex &other) = default;

        INLINE ModelIndex &operator=(const ModelIndex &other) = default;

        INLINE ModelIndex(ModelIndex &&other);

        INLINE ModelIndex &operator=(ModelIndex &&other);

        // get the model
        INLINE const IAbstractItemModel *model() const { return m_model; }

        // get globally unique index
        INLINE uint64_t index() const { return m_uniqueIndex; }

        // is this a valid item ?
        INLINE bool valid() const { return nullptr != m_model; }

        // get internal pointer, valid only for non indexed
        INLINE const base::UntypedRefWeakPtr &weakRef() const { return m_ref; }

        // lock internal pointer
        template<typename T> 
        INLINE base::RefPtr<T> lock() const { return NoAddRef(static_cast<T*>(m_ref.lock())); }

        // get internal pointer, unsafe if object can get deleted in the mean time
        template<typename T> 
        INLINE T *unsafe() const { return static_cast<T*>(m_ref.unsafe()); }

        //--

        // quick check, allows to use model index in ifs
        INLINE operator bool() const noexcept { return valid(); }

        // compare
        INLINE bool operator==(const ModelIndex &other) const noexcept { return (other.m_uniqueIndex == m_uniqueIndex); }

        INLINE bool operator!=(const ModelIndex &other) const noexcept { return !operator==(other); }

        // sorting
        INLINE bool operator<(const ModelIndex &other) const;

        // get parent item
        INLINE ModelIndex parent() const;

        //--

        // dump
        void print(base::IFormatStream &f) const;

        //--

        // hashing
        static uint32_t CalcHash(const ui::ModelIndex& index);

        // allocate unique index for future use
        static uint64_t AllocateUniqueIndex();

    private:
        const IAbstractItemModel *m_model = nullptr;
        base::UntypedRefWeakPtr m_ref;
        uint64_t m_uniqueIndex = 0; // globally unique index
    };
    
    //--

    /// observer of the model
    class BASE_UI_API IAbstractItemModelObserver : public base::NoCopy
    {
    public:
        virtual ~IAbstractItemModelObserver();

        /// data model was reset
        virtual void modelReset() = 0;

        /// items were added
        virtual void modelItemsAdded(const ModelIndex &parent, const base::Array<ModelIndex>& children) = 0;

        /// items were removed
        virtual void modelItemsRemoved(const ModelIndex &parent, const base::Array<ModelIndex>& children) = 0;

        /// item requires an update
        virtual void modelItemUpdate(const ModelIndex& index, ItemUpdateMode mode) = 0;
    };

    //--

    /// base model for stuff that contains items
    class BASE_UI_API IAbstractItemModel : public base::IReferencable
    {
        RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IAbstractItemModel);

    public:
        virtual ~IAbstractItemModel();

        /// get parent for given item
        virtual ui::ModelIndex parent(const ui::ModelIndex& item = ui::ModelIndex()) const = 0;

        /// check if we have children at given item
        virtual bool hasChildren(const ui::ModelIndex& parent) const = 0;

        /// get all children of given parent, used only if element is visualized for the first time, incremental updates depend on the notifications
        virtual void children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const = 0;

        /// compare for sorting
        virtual bool compare(const ui::ModelIndex& first, const ui::ModelIndex& second, int colIndex=0) const;

        /// filter for display
        virtual bool filter(const ui::ModelIndex& id, const ui::SearchPattern& filter, int colIndex=0) const;

        /// get the display content for the element, can contain markup, icons, etc
        virtual base::StringBuf displayContent(const ui::ModelIndex& id, int colIndex = 0) const;

        /// create context menu for given ITEMS
        virtual ui::PopupPtr contextMenu(ui::AbstractItemView* view, const base::Array<ui::ModelIndex>& indices) const;

        /// create tooltip content for this element
        virtual ui::ElementPtr tooltip(ui::AbstractItemView* view, ui::ModelIndex id) const;

        //--

        // generate drag&drop data for the item
        virtual DragDropDataPtr queryDragDropData(const base::input::BaseKeyFlags& keys, const ModelIndex& item);
        
        // handle drag & drop reception for given item
        virtual DragDropHandlerPtr handleDragDropData(ui::AbstractItemView* view, const ModelIndex& item, const DragDropDataPtr& data, const Position& pos);

        // handle drag&drop data
        virtual bool handleDragDropCompletion(ui::AbstractItemView* view, const ModelIndex& item, const DragDropDataPtr& data);

        //--

        /// create/update visualization for the item, called when visualization is missing or if model requested update
        /// columnCount indicates visualization mode == 1 - vertical list, >1 = vertical list with columns, 0 = icons
        /// NOTE: by default it creates a single text label with content sucked from displayContent
        virtual void visualize(const ModelIndex& item, int columnCount, ElementPtr& content) const;

        //--

        /// reset model at given item, all children are assume to be invalidated
        /// calling on root invalidates everything
        void reset();

        /// notify that new element was just added
        void notifyItemsAdded(const ModelIndex& parent, const base::Array<ModelIndex>& items);

        /// notify that new element was just added
        void notifyItemAdded(const ModelIndex& parent, const ModelIndex& item);

        /// notify that new element was just removed
        void notifyItemsRemoved(const ModelIndex& parent, const base::Array<ModelIndex>& items);

        /// notify that new element was just removed
        void notifyItemRemoved(const ModelIndex& parent, const ModelIndex& item);

        //--

        /// request "repaint" of current element, basically gives it a chance to visualize new content
        /// NOTE: this may be deferred until the item gets visible again unless the Force flag is specified
        void requestItemUpdate(const ModelIndex& item, ItemUpdateMode updateMode = ItemUpdateModeBit::Item) const;

        //--

        /// register an observer
        void registerObserver(IAbstractItemModelObserver* observer);

        /// unregister an observer
        void unregisterObserver(IAbstractItemModelObserver* observer);

    private:
        base::Array<IAbstractItemModelObserver*> m_observers;
    };

    //--

    INLINE ModelIndex::ModelIndex()
    {}

    INLINE ModelIndex::ModelIndex(ModelIndex&& other)
        : m_model(other.m_model)
        , m_uniqueIndex(other.m_uniqueIndex)
        , m_ref(other.m_ref)
    {
        other.m_model = nullptr;
        other.m_uniqueIndex = 0;
        other.m_ref.reset();
    }

    INLINE ModelIndex& ModelIndex::operator=(ModelIndex&& other)
    {
        if (this != &other)
        {
            m_model = other.m_model;
            m_uniqueIndex = other.m_uniqueIndex;
            m_ref = std::move(other.m_ref);
            other.m_model = nullptr;
            other.m_uniqueIndex = 0;
            other.m_ref.reset();
        }

        return *this;
    }

    INLINE bool ModelIndex::operator<(const ModelIndex& other) const
    {
        return m_uniqueIndex < other.m_uniqueIndex;
    }

    INLINE ModelIndex ModelIndex::parent() const
    {
        return m_model ? m_model->parent(*this) : ModelIndex();
    }

    //--

} // ui
