/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: model #]
***/

#include "build.h"
#include "uiAbstractItemModel.h"
#include "uiTextLabel.h"
#include "uiWindow.h"
#include "uiButton.h"
#include "uiDragDrop.h"

namespace ui
{
    
    //---

    RTTI_BEGIN_CUSTOM_TYPE(ModelIndex);
        RTTI_BIND_NATIVE_CTOR_DTOR(ModelIndex);
        RTTI_BIND_NATIVE_COMPARE(ModelIndex);
        RTTI_BIND_NATIVE_COPY(ModelIndex);
        RTTI_BIND_NATIVE_PRINT(ModelIndex);
    RTTI_END_TYPE();

    //---

    uint32_t ModelIndex::CalcHash(const ui::ModelIndex& index)
    {
        base::CRC32 crc;
        crc << index.m_row;
        crc << index.m_col;
        crc << index.m_ref.unsafe();
        return crc.crc();
    }

    void ModelIndex::print(base::IFormatStream& f) const
    {
        if (!valid())
        {
            f << "empty";
        }
        else
        {
            f.appendf("row={}, col={}, ptr={}", m_row, m_col, m_ref.unsafe());
        }
    }

    //---

    IAbstractItemModelObserver::~IAbstractItemModelObserver()
    {}

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IAbstractItemModel);
    RTTI_END_TYPE();

    IAbstractItemModel::~IAbstractItemModel()
    {}

    DragDropHandlerPtr IAbstractItemModel::handleDragDropData(ui::AbstractItemView* view, const ModelIndex& item, const DragDropDataPtr& data, const Position& pos)
    {
        return nullptr;
    }

    bool IAbstractItemModel::handleDragDropCompletion(ui::AbstractItemView* view, const ModelIndex& item, const DragDropDataPtr& data)
    {
        return false;
    }

    DragDropDataPtr IAbstractItemModel::queryDragDropData(const base::input::BaseKeyFlags& keys, const ModelIndex& item)
    {
        return nullptr;
    }

    base::StringBuf IAbstractItemModel::displayContent(const ModelIndex& id, int colIndex /*= 0*/) const
    {
        return base::TempString("{}", id);
    }

    PopupPtr IAbstractItemModel::contextMenu(AbstractItemView* view, const base::Array<ModelIndex>& indices) const
    {
        return nullptr;
    }

    ui::ElementPtr IAbstractItemModel::tooltip(ui::AbstractItemView* view, ui::ModelIndex id) const
    {
        return nullptr;
    }

    void IAbstractItemModel::visualize(const ModelIndex& item, int columnCount, ElementPtr& content) const
    {
        if (columnCount <= 1)
        {
            auto txt = displayContent(item, 0);

            if (auto label = base::rtti_cast<TextLabel>(content))
                label->text(txt);
            else
                content = base::CreateSharedPtr<TextLabel>(txt);
        }
        else
        {
            if (!content || content->cls() != IElement::GetStaticClass() || content->layoutMode() != LayoutMode::Columns)
            {
                content = base::CreateSharedPtr<IElement>();
                content->layoutMode(LayoutMode::Columns);
            }

            int columnIndex = 0;
            for (auto it = content->childrenList(); it; ++it)
            {
                if (auto txt = base::rtti_cast<TextLabel>(*it))
                    txt->text(displayContent(item, columnIndex));
                columnIndex += 1;
            }

            while (columnIndex < columnCount)
            {
                auto txt = base::CreateSharedPtr<TextLabel>(displayContent(item, columnIndex));
                content->attachChild(txt);
                columnIndex += 1;
            }
        }
    }

    void IAbstractItemModel::requestItemUpdate(const ModelIndex& item, ItemUpdateMode updateMode /*= ItemUpdateMode::Item*/) const
    {
        for (auto* entry : m_observers)
            entry->modelItemUpdate(item, updateMode);
    }

    void IAbstractItemModel::reset()
    {
        if (m_observers.empty())
            return;

        ASSERT(m_updateState == UpdateState::None);
        m_updateState = UpdateState::Reset;

        for (auto* entry : m_observers)
            entry->modelReset();

        ASSERT(m_updateState == UpdateState::Reset);
        m_updateState = UpdateState::None;
    }

    void IAbstractItemModel::beingInsertRows(const ModelIndex& parent, int first, int count)
    {
        ASSERT(m_updateState == UpdateState::None);
        m_updateState = UpdateState::InsertRows;
        m_updateIndex = parent;
        m_updateArgFirst = first;
        m_updateArgCount = count;

        for (auto* entry : m_observers)
            entry->modelRowsAboutToBeAdded(parent, first, count);
    }

    void IAbstractItemModel::endInsertRows()
    {
        ASSERT(m_updateState == UpdateState::InsertRows);

        for (auto* entry : m_observers)
            entry->modelRowsAdded(m_updateIndex, m_updateArgFirst, m_updateArgCount);

        m_updateState = UpdateState::None;
        m_updateIndex = ModelIndex();
        m_updateArgFirst = INDEX_NONE;
        m_updateArgCount = INDEX_NONE;
    }

    void IAbstractItemModel::beingRemoveRows(const ModelIndex& parent, int first, int count)
    {
        ASSERT(m_updateState == UpdateState::None);
        m_updateState = UpdateState::RemoveRows;
        m_updateIndex = parent;
        m_updateArgFirst = first;
        m_updateArgCount = count;

        for (auto* entry : m_observers)
            entry->modelRowsAboutToBeRemoved(parent, first, count);
    }

    void IAbstractItemModel::endRemoveRows()
    {
        ASSERT(m_updateState == UpdateState::RemoveRows);

        for (auto* entry : m_observers)
            entry->modelRowsRemoved(m_updateIndex, m_updateArgFirst, m_updateArgCount);

        m_updateState = UpdateState::None;
        m_updateIndex = ModelIndex();
        m_updateArgFirst = INDEX_NONE;
        m_updateArgCount = INDEX_NONE;
    }

    bool IAbstractItemModel::compare(const ModelIndex& first, const ModelIndex& second, int colIndex /*=0*/) const
    {
        return first < second;
    }

    bool IAbstractItemModel::filter(const ModelIndex& id, const ui::SearchPattern& filter, int colIndex/*=0*/) const
    {
        return true;
    }

    //---

    void IAbstractItemModel::registerObserver(IAbstractItemModelObserver* observer)
    {
        m_observers.pushBack(observer);
    }

    void IAbstractItemModel::unregisterObserver(IAbstractItemModelObserver* observer)
    {
        m_observers.remove(observer);
    }

    //---    

} // ui