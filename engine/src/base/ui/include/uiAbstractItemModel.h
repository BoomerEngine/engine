/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: model #]
***/

#pragma once

#include "base/containers/include/mutableArray.h"
#include "base/input/include/inputStructures.h"

namespace ui
{
    //---

    /// non-persistent index in the model, based on Qt's idea
    class BASE_UI_API ModelIndex
    {
    public:
        INLINE ModelIndex(); // -1 -1
        INLINE ModelIndex(const IAbstractItemModel *model, int rowIndex, int colIndex, const base::UntypedRefWeakPtr &ptr = nullptr);

        INLINE ModelIndex(const ModelIndex &other) = default;

        INLINE ModelIndex &operator=(const ModelIndex &other) = default;

        INLINE ModelIndex(ModelIndex &&other);

        INLINE ModelIndex &operator=(ModelIndex &&other);

        // get the model
        INLINE const IAbstractItemModel *model() const { return m_model; }

        // is this a valid item ?
        INLINE bool valid() const { return (m_row >= 0) && (m_col >= 0) && (nullptr != m_model); }

        // get row element index
        INLINE int row() const { return m_row; }

        // get column element index
        INLINE int column() const { return m_row; }

        // get internal pointer
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
        INLINE bool operator==(const ModelIndex &other) const noexcept { return (other.m_row == m_row) && (other.m_col == m_col) && (other.m_model == m_model) && (other.m_ref == m_ref); }

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

    private:
        const IAbstractItemModel *m_model = nullptr;
        int m_row = INDEX_NONE;
        int m_col = INDEX_NONE;
        base::UntypedRefWeakPtr m_ref;
    };
    
    //--

    /// observer of the model
    class BASE_UI_API IAbstractItemModelObserver : public base::NoCopy
    {
    public:
        virtual ~IAbstractItemModelObserver();

        /// data model was reset
        virtual void modelReset() = 0;

        /// rows about to be added
        virtual void modelRowsAboutToBeAdded(const ModelIndex &parent, int first, int count) = 0;

        /// rows added
        virtual void modelRowsAdded(const ModelIndex &parent, int first, int count) = 0;

        /// rows about to be removed
        virtual void modelRowsAboutToBeRemoved(const ModelIndex &parent, int first, int count) = 0;

        /// rows removed
        virtual void modelRowsRemoved(const ModelIndex &parent, int first, int count) = 0;

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

        /// get number of rows in the model
        virtual uint32_t rowCount(const ui::ModelIndex& parent = ui::ModelIndex()) const = 0;

        /// check if we have children at given item
        virtual bool hasChildren(const ui::ModelIndex& parent = ui::ModelIndex()) const = 0;

        /// check if given item has given child
        virtual bool hasIndex(int row, int col, const ui::ModelIndex& parent = ui::ModelIndex()) const = 0;

        /// get parent for given item
        virtual ui::ModelIndex parent(const ui::ModelIndex &item = ui::ModelIndex()) const = 0;

        /// get child index
        virtual ui::ModelIndex index(int row, int column, const ui::ModelIndex &parent = ui::ModelIndex()) const = 0;

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

        /// begin adding items
        void beingInsertRows(const ModelIndex& parent, int first, int count);

        /// finish adding items
        void endInsertRows();

        /// begin removing items
        void beingRemoveRows(const ModelIndex& parent, int first, int count);

        /// finish removing items
        void endRemoveRows();

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
        enum class UpdateState
        {
            None,
            Reset,
            InsertRows,
            RemoveRows,
        };

        ModelIndex m_updateIndex;
        UpdateState m_updateState = UpdateState::None;
        int m_updateArgFirst = INDEX_NONE;
        int m_updateArgCount = INDEX_NONE;

        base::MutableArray<IAbstractItemModelObserver*> m_observers;
    };

    //--

    INLINE ModelIndex::ModelIndex()
    {}

    INLINE ModelIndex::ModelIndex(const IAbstractItemModel* model, int rowIndex, int colIndex, const base::UntypedRefWeakPtr& ptr)
        : m_model(model)
        , m_row(rowIndex)
        , m_col(colIndex)
        , m_ref(ptr)
    {
        ASSERT(nullptr != model);
        ASSERT(rowIndex >= 0);
        ASSERT(colIndex >= 0);
    }

    INLINE ModelIndex::ModelIndex(ModelIndex&& other)
        : m_model(other.m_model)
        , m_row(other.m_row)
        , m_col(other.m_col)
        , m_ref(other.m_ref)
    {
        other.m_model = nullptr;
        other.m_row = INDEX_NONE;
        other.m_col = INDEX_NONE;
        other.m_ref.reset();
    }

    INLINE ModelIndex& ModelIndex::operator=(ModelIndex&& other)
    {
        if (this != &other)
        {
            m_model = other.m_model;
            m_row = other.m_row;
            m_col = other.m_col;
            m_ref = std::move(other.m_ref);
            other.m_model = nullptr;
            other.m_row = INDEX_NONE;
            other.m_col = INDEX_NONE;
            other.m_ref.reset();
        }

        return *this;
    }

    INLINE bool ModelIndex::operator<(const ModelIndex& other) const
    {
        if (m_row != other.m_row)
            return m_row < other.m_row;

        if (m_col != other.m_col)
            return m_col < other.m_col;

        if (m_ref != other.m_ref)
            return m_ref.unsafe() < other.m_ref.unsafe();

        return (void*)m_model < (void*)other.m_model;
    }

    INLINE ModelIndex ModelIndex::parent() const
    {
        return m_model ? m_model->parent(*this) : ModelIndex();
    }

    //--

    enum class ReindexerResult : uint8_t
    {
        NotChanged,
        Changed,
        Removed,
    };

    // helper class to reindex elements when adding new stuff
    class ModelIndexReindexerInsert : public base::NoCopy
    {
    public:
        ModelIndexReindexerInsert(const ModelIndex& parent, int first, int count)
            : m_parent(parent)
            , m_first(first)
            , m_count(count)
        {}

        INLINE const ModelIndex& parent() const { return m_parent; }
        INLINE int first() const { return m_first; }
        INLINE int count() const { return m_count; }

        INLINE ReindexerResult reindex(const ModelIndex& cur, ModelIndex& outIndex) const
        {
            if (cur.parent() == m_parent)
            {
                if (cur.row() >= m_first)
                {
                    outIndex = ModelIndex(cur.model(), cur.row() + m_count, cur.column(), cur.weakRef());
                    return ReindexerResult::Changed;
                }
            }

            return ReindexerResult::NotChanged;
        }

    private:
        ModelIndex m_parent;
        int m_first;
        int m_count;
    };

    // helper class to reindex elements when removing stuff
    class ModelIndexReindexerRemove : public base::NoCopy
    {
    public:
        ModelIndexReindexerRemove(const ModelIndex& parent, int first, int count)
            : m_parent(parent)
            , m_first(first)
            , m_count(count)
        {}

        INLINE const ModelIndex& parent() const { return m_parent; }
        INLINE int first() const { return m_first; }
        INLINE int count() const { return m_count; }

        INLINE ReindexerResult reindex(const ModelIndex& cur, ModelIndex& outIndex) const
        {
            if (cur.parent() == m_parent)
            {
                if (cur.row() >= (m_count + m_first))
                {
                    outIndex = ModelIndex(cur.model(), cur.row() - m_count, cur.column(), cur.weakRef());
                    return ReindexerResult::Changed;
                }
                else if (cur.row() >= m_first)
                {
                    return ReindexerResult::Removed;
                }
            }

            return ReindexerResult::NotChanged;
        }

    private:
        ModelIndex m_parent;
        int m_first;
        int m_count;
    };

    //--

    /// simple list model
    class ISimpleListModel : public IAbstractItemModel
    {
    public:
        RTTI_DECLARE_VIRTUAL_CLASS(ISimpleListModel, IAbstractItemModel);

    public:
        ISimpleListModel(uint32_t size);
        virtual ~ISimpleListModel();

        virtual uint32_t rowCount(const ModelIndex& parent = ModelIndex()) const override final;
        virtual bool hasChildren(const ModelIndex& parent = ModelIndex()) const override final;
        virtual bool hasIndex(int row, int col, const ModelIndex& parent = ModelIndex()) const  override final;
        virtual ModelIndex parent(const ModelIndex& item = ModelIndex()) const  override final;
        virtual ModelIndex index(int row, int column, const ModelIndex& parent = ModelIndex()) const override final;

    private:
        uint32_t m_size;
    };

    //--

} // ui
