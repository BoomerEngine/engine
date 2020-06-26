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

    /// simple list model
    class BASE_UI_API SimpleListModel : public IAbstractItemModel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SimpleListModel, IAbstractItemModel);

    public:
        SimpleListModel();
        virtual ~SimpleListModel();

        virtual uint32_t rowCount(const ModelIndex& parent = ModelIndex()) const override final;
        virtual bool hasChildren(const ModelIndex& parent = ModelIndex()) const override final;
        virtual bool hasIndex(int row, int col, const ModelIndex& parent = ModelIndex()) const  override final;
        virtual ModelIndex parent(const ModelIndex& item = ModelIndex()) const override final;
        virtual ModelIndex index(int row, int column, const ModelIndex& parent = ModelIndex()) const override final;
        virtual bool compare(const ModelIndex& first, const ModelIndex& second, int colIndex=0) const;
        virtual bool filter(const ModelIndex& id, const ui::SearchPattern& filter, int colIndex=0) const;
        virtual base::StringBuf displayContent(const ModelIndex& id, int colIndex = 0) const override;

        virtual uint32_t size() const = 0;
        virtual base::StringBuf content(const ModelIndex& id, int colIndex = 0) const = 0;
    };

    //--

    /// simple typed list model 
    template< typename T, typename RefT = const T& >
    class SimpleTypedListModel : public SimpleListModel
    {
    public:
        INLINE const base::Array<T>& elements() const { return m_elements; }

        void clear()
        {
            beingRemoveRows(ui::ModelIndex(), 0, m_elements.size());
            m_elements.clear();
            endRemoveRows();
        }

        void add(RefT data)
        {
            beingInsertRows(ui::ModelIndex(), m_elements.size(), 1);
            m_elements.pushBack(data);
            endInsertRows();
        }

        int find(RefT data) const
        {
            return m_elements.find(data);
        }

        ModelIndex index(RefT data) const
        {
            auto index = find(data);
            if (index != -1)
                return ModelIndex(this, index, 0);
            return ModelIndex();
        }

        void remove(RefT data)
        {
            erase(m_elements.find(data));
        }

        void erase(int index)
        {
            if (index >= 0 && index <= m_elements.size())
            {
                beingRemoveRows(ui::ModelIndex(), index, 1);
                m_elements.erase(index, 1);
                endRemoveRows();
            }
        }

        RefT data(const ui::ModelIndex& id, RefT defaultData = T()) const
        {
            if (id.row() >= 0 && id.row() <= m_elements.lastValidIndex())
                return m_elements[id.row()];
            return defaultData;
        }

        const T* dataPtr(const ui::ModelIndex& id) const
        {
            if (id.row() >= 0 && id.row() <= m_elements.lastValidIndex())
                return &m_elements[id.row()];
            return nullptr;
        }

        T* dataPtr(const ui::ModelIndex& id)
        {
            if (id.row() >= 0 && id.row() <= m_elements.lastValidIndex())
                return &m_elements[id.row()];
            return nullptr;
        }
        
        virtual uint32_t size() const override { return m_elements.size(); }

        virtual base::StringBuf content(const ModelIndex& id, int colIndex = 0) const override
        {
            if (id.row() >= 0 && id.row() <= m_elements.lastValidIndex())
                return content(m_elements[id.row()], colIndex);
            return base::StringBuf();
        }

        //--

        /// compare items by data
        virtual bool compare(RefT a, RefT b, int colIndex) const = 0;

        /// filter items by data
        virtual bool filter(RefT data, const ui::SearchPattern& filter, int colIndex = 0) const = 0;

        // get display content for item
        virtual base::StringBuf content(RefT data, int colIndex = 0) const = 0;

    protected:
        virtual bool compare(const ModelIndex& first, const ModelIndex& second, int colIndex = 0) const override;
        virtual bool filter(const ModelIndex& id, const ui::SearchPattern& filter, int colIndex = 0) const override;

    private:
        base::Array<T> m_elements;
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
    bool SimpleTypedListModel<T, TV>::compare(const ModelIndex& first, const ModelIndex& second, int colIndex) const
    {
        const auto* firstEntry = dataPtr(first);
        const auto* secondEntry = dataPtr(second);
        if (firstEntry && secondEntry)
            compare(*firstEntry, *secondEntry, colIndex);

        return first < second;
    }

    template< typename T, typename TV >
    bool SimpleTypedListModel<T, TV>::filter(const ModelIndex& id, const ui::SearchPattern& filterText, int colIndex) const
    {
        const auto* data = dataPtr(id);
        if (data)
            return filter(*data, filterText, colIndex);
        return false;
    }

    //--

} // ui
