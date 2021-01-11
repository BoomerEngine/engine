/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: model #]
***/

#pragma once

#include "uiAbstractItemModel.h"

namespace ui
{

    //--

    /// simple typed list model 
    template< typename T, typename RefT = const T& >
    class SimpleTypedListModel : public IAbstractItemModel
    {
    public:
        void clear()
        {
            m_elements.clear();
            reset();
        }

        ModelIndex add(RefT data)
        {
            auto holder = base::RefNew<ElemHolder>();
            holder->data = data;
            holder->displayIndex = m_displayIndex++;
            holder->index = ModelIndex(this, holder);
            m_elements.pushBack(holder);

            notifyItemAdded(ModelIndex(), holder->index);

            return holder->index;
        }

        ModelIndex index(RefT data) const
        {
            for (const auto& elem : m_elements)
                if (elem->data == data)
                    return elem->index;

            return ModelIndex();
        }

        uint32_t remove(RefT data, bool all = false)
        {
            uint32_t count = 0;

            for (auto i : m_elements.indexRange().reversed())
            {
                if (elem->data == data)
                {
                    auto index = elem->index;

                    m_elements.eraseUnordered(i);
                    count += 1;

                    notifyItemRemoved(ModelIndex(), index);

                    if (!all)
                        break;
                }
            }

            return count;
        }

        bool erase(const ModelIndex& index)
        {
            if (index.model() == this)
            {
                auto index = m_elements.find(index.unsafe());
                if (index != INDEX_NONE)
                {
                    auto removedElem = m_elements[index]; // keep alive
                    m_elements.eraseUnordered(index);
                    notifyItemRemoved(ModelIndex(), index);

                    return true;
                }
            }

            return false;
        }

        RefT data(const ModelIndex& id, RefT defaultData = T()) const
        {
            if (id.model() == this)
                if (const auto* data = id.unsafe<ElemHolder>())
                    return data->data;

            return defaultData;
        }

        const T* dataPtr(const ModelIndex& id) const
        {
            if (id.model() == this)
                if (const auto* data = id.unsafe<ElemHolder>())
                    return &data->data;

            return nullptr;
        }

        T* dataPtr(const ModelIndex& id)
        {
            if (id.model() == this)
                if (auto* data = id.unsafe<ElemHolder>())
                    return &data->data;

            return nullptr;
        }
        
        INLINE uint32_t size() const
        {
            return m_elements.size();
        }

        INLINE const T& operator[](uint32_t index) const
        {
            return m_elements[index]->data;
        }

        INLINE T& operator[](uint32_t index)
        {
            return m_elements[index]->data;
        }

        INLINE const T& at(uint32_t index) const
        {
            return m_elements[index]->data;
        }

        INLINE T& at(uint32_t index)
        {
            return m_elements[index]->data;
        }

        //--

        /// compare items by data
        virtual bool compare(RefT a, RefT b, int colIndex) const = 0;

        /// filter items by data
        virtual bool filter(RefT data, const SearchPattern& filter, int colIndex = 0) const = 0;

        // get display content for item
        virtual base::StringBuf content(RefT data, int colIndex = 0) const = 0;

    protected:
        virtual bool compare(const ModelIndex& first, const ModelIndex& second, int colIndex = 0) const override;
        virtual bool filter(const ModelIndex& id, const SearchPattern& filter, int colIndex = 0) const override;
        virtual void children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const override;
        virtual bool hasChildren(const ui::ModelIndex& parent) const override;
        virtual ModelIndex parent(const ModelIndex& item = ModelIndex()) const override final;
        virtual base::StringBuf displayContent(const ModelIndex& id, int colIndex = 0) const override;

    private:
        struct ElemHolder : public base::IReferencable
        {
            T data;
            ModelIndex index;
            int displayIndex = 0;
        };

        base::Array<base::RefPtr<ElemHolder>> m_elements;

        int m_displayIndex = 0;
    };

    //--

    template< typename T, typename RefT = const T& > 
    class SimpleTypedListModelAutoPrint : public SimpleTypedListModel<T, RefT>
    {
    public:
        virtual base::StringBuf content(RefT data, int colIndex = 0) const override
        {
            base::StringBuilder txt;
            txt.append(data);
            return txt.toString();
        }
    };

    //--

    template< typename T, typename TV >
    void SimpleTypedListModel<T, TV>::children(const ui::ModelIndex& parent, base::Array<ui::ModelIndex>& outChildrenIndices) const
    {
        if (!parent)
        {
            outChildrenIndices.reserve(m_elements.size());

            for (const auto& elem : m_elements)
                outChildrenIndices.emplaceBack(elem->index);
        }
    }

    template< typename T, typename TV >
    bool SimpleTypedListModel<T, TV>::hasChildren(const ui::ModelIndex& parent) const
    {
        return !parent.valid() && !m_elements.empty();
    }

    template< typename T, typename TV >
    bool SimpleTypedListModel<T, TV>::compare(const ModelIndex& first, const ModelIndex& second, int colIndex) const
    {
        if (colIndex == -1)
        {
            const auto* firstEntry = first.unsafe<ElemHolder>();
            const auto* secondEntry = second.unsafe<ElemHolder>();
            if (firstEntry && secondEntry)
                return firstEntry->displayIndex < secondEntry->displayIndex;

            return first < second;
        }
        else
        {
            const auto* firstEntry = dataPtr(first);
            const auto* secondEntry = dataPtr(second);
            if (firstEntry && secondEntry)
                compare(*firstEntry, *secondEntry, colIndex);

            return first < second;
        }
    }

    template< typename T, typename TV >
    bool SimpleTypedListModel<T, TV>::filter(const ModelIndex& id, const SearchPattern& filterText, int colIndex) const
    {
        const auto* data = dataPtr(id);
        if (data)
            return filter(*data, filterText, colIndex);
        return false;
    }

    template< typename T, typename TV >
    ModelIndex SimpleTypedListModel<T, TV>::parent(const ModelIndex& item) const
    {
        return ModelIndex();
    }

    template< typename T, typename TV >
    base::StringBuf SimpleTypedListModel<T, TV>::displayContent(const ModelIndex& id, int colIndex) const
    {
        if (const auto* entry = id.unsafe<ElemHolder>())
            return content(entry->data, colIndex);
        return base::StringBuf::EMPTY();
    }

    //--

} // ui
