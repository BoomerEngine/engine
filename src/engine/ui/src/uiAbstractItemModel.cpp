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
#include "uiInputAction.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)
    
//---

RTTI_BEGIN_CUSTOM_TYPE(ModelIndex);
    RTTI_BIND_NATIVE_CTOR_DTOR(ModelIndex);
    RTTI_BIND_NATIVE_COMPARE(ModelIndex);
    RTTI_BIND_NATIVE_COPY(ModelIndex);
    RTTI_BIND_NATIVE_PRINT(ModelIndex);
RTTI_END_TYPE();

//---

static std::atomic<uint64_t> GModelIndexGlobalUniqueCounter = 1;

uint64_t ModelIndex::AllocateUniqueIndex()
{
    return GModelIndexGlobalUniqueCounter++;
}

ModelIndex::ModelIndex(const IAbstractItemModel* model, const UntypedRefWeakPtr& ptr, uint64_t uniqueIndex)
    : m_model(model)
    , m_ref(ptr)
{
    ASSERT(nullptr != model);
    m_uniqueIndex = uniqueIndex ? uniqueIndex : AllocateUniqueIndex();
}

uint32_t ModelIndex::CalcHash(const ModelIndex& index)
{
    return (uint32_t)index.m_uniqueIndex;
}

void ModelIndex::print(IFormatStream& f) const
{
    if (!valid())
        f << "empty";
    else
        f.appendf("ModelIndex(#{})", m_uniqueIndex);
}

//---

IAbstractItemModelObserver::~IAbstractItemModelObserver()
{}

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IAbstractItemModel);
RTTI_END_TYPE();

IAbstractItemModel::~IAbstractItemModel()
{}

DragDropHandlerPtr IAbstractItemModel::handleDragDropData(AbstractItemView* view, const ModelIndex& item, const DragDropDataPtr& data, const Position& pos)
{
    return nullptr;
}

bool IAbstractItemModel::handleDragDropCompletion(AbstractItemView* view, const ModelIndex& item, const DragDropDataPtr& data)
{
    return false;
}

bool IAbstractItemModel::handleIconClick(const ModelIndex& item, int columnIndex) const
{
    return false;
}

DragDropDataPtr IAbstractItemModel::queryDragDropData(const BaseKeyFlags& keys, const ModelIndex& item)
{
    return nullptr;
}

StringBuf IAbstractItemModel::displayContent(const ModelIndex& id, int colIndex /*= 0*/) const
{
    return TempString("{}", id);
}

PopupPtr IAbstractItemModel::contextMenu(AbstractItemView* view, const Array<ModelIndex>& indices) const
{
    return nullptr;
}

ElementPtr IAbstractItemModel::tooltip(AbstractItemView* view, ModelIndex id) const
{
    return nullptr;
}

class AbstractItemModelTextLabel : public TextLabel
{
    RTTI_DECLARE_VIRTUAL_CLASS(AbstractItemModelTextLabel, TextLabel);

public:
    AbstractItemModelTextLabel(StringView text, const ModelIndex& item, const IAbstractItemModel* model, int columnIndex)
        : TextLabel(text)
        , m_model(model)
        , m_columnIndex(columnIndex)
        , m_item(item)
    {
        hitTest(text.beginsWith("["));
    }

    StringBuf text() const
    {
        return TBaseClass::text();
    }

    void text(StringView txt)
    {
        TBaseClass::text(txt);
        hitTest(txt.beginsWith("["));
    }

    virtual InputActionPtr handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt) override
    {
        if (evt.leftClicked() && text().beginsWith("["))
        {
            m_model->handleIconClick(m_item, m_columnIndex);
            return IInputAction::CONSUME();
        }

        return TBaseClass::handleMouseClick(area, evt);
    }

private:
    const IAbstractItemModel* m_model = nullptr;
    ModelIndex m_item;
    char m_columnIndex = 0;
};

RTTI_BEGIN_TYPE_NATIVE_CLASS(AbstractItemModelTextLabel);
RTTI_END_TYPE();

void IAbstractItemModel::visualize(const ModelIndex& item, int columnCount, ElementPtr& content) const
{
    if (columnCount <= 1)
    {
        auto txt = displayContent(item, 0);

        if (auto label = rtti_cast<TextLabel>(content))
            label->text(txt);
        else
            content = RefNew<TextLabel>(txt);
    }
    else
    {
        if (!content || content->cls() != IElement::GetStaticClass() || content->layoutMode() != LayoutMode::Columns)
        {
            content = RefNew<IElement>();
            content->layoutMode(LayoutMode::Columns);
        }

        int columnIndex = 0;
        for (auto it = content->childrenList(); it; ++it)
        {
            if (auto txt = rtti_cast<AbstractItemModelTextLabel>(*it))
                txt->text(displayContent(item, columnIndex));

            columnIndex += 1;
        }

        while (columnIndex < columnCount)
        {
            auto txt = RefNew<AbstractItemModelTextLabel>(displayContent(item, columnIndex), item, this, columnIndex);
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
    for (auto* entry : m_observers)
        if (entry)
            entry->modelReset();

    m_observers.removeAll(nullptr);
}

void IAbstractItemModel::notifyItemAdded(const ModelIndex& parent, const ModelIndex& item)
{
    if (item)
    {
        InplaceArray<ModelIndex, 1> items;
        items.pushBack(item);

        notifyItemsAdded(parent, items);
    }
}

void IAbstractItemModel::notifyItemsAdded(const ModelIndex& parent, const Array<ModelIndex>& items)
{
    for (auto* entry : m_observers)
        if (entry)
            entry->modelItemsAdded(parent, items);

    m_observers.removeAll(nullptr);
}

void IAbstractItemModel::notifyItemRemoved(const ModelIndex& parent, const ModelIndex& item)
{
    if (item)
    {
        InplaceArray<ModelIndex, 1> items;
        items.pushBack(item);

        notifyItemsRemoved(parent, items);
    }
}

void IAbstractItemModel::notifyItemsRemoved(const ModelIndex& parent, const Array<ModelIndex>& items)
{
    for (auto* entry : m_observers)
        if (entry)
            entry->modelItemsRemoved(parent, items);

    m_observers.removeAll(nullptr);
}

bool IAbstractItemModel::compare(const ModelIndex& first, const ModelIndex& second, int colIndex /*=0*/) const
{
    return first < second;
}

bool IAbstractItemModel::filter(const ModelIndex& id, const SearchPattern& filter, int colIndex/*=0*/) const
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
    m_observers.replace(observer, nullptr);
}

//---    

END_BOOMER_NAMESPACE_EX(ui)
