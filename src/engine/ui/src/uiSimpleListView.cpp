/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: model #]
***/

#include "build.h"
#include "uiSimpleListView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISimpleListViewElement);
RTTI_END_TYPE();

ISimpleListViewElement::ISimpleListViewElement()
{
}

ISimpleListViewElement::~ISimpleListViewElement()
{}

bool ISimpleListViewElement::compare(const ISimpleListViewElement* other) const
{
    return m_listIndex < other->m_listIndex;
}

bool ISimpleListViewElement::filter(const SearchPattern& filter, int colIndex) const
{
    return true;
}

//---

class SimpleListViewModel : public IAbstractItemModel
{
public:
    virtual ModelIndex parent(const ModelIndex& item) const override
    {
        return ModelIndex();
    }

    INLINE const Array<RefPtr<ISimpleListViewElement>>& elements() const
    {
        return m_elements;
    }

    virtual bool hasChildren(const ModelIndex& parent) const override
    {
        return !parent.valid() && !m_elements.empty();
    }

    virtual void children(const ModelIndex& parent, Array<ModelIndex>& outChildrenIndices) const override
    {
        if (!parent)
        {
            outChildrenIndices.reserve(m_elements.size());

            for (const auto& elem : m_elements)
                outChildrenIndices.emplaceBack(elem->m_index);
        }
    }

    virtual bool compare(const ModelIndex& first, const ModelIndex& second, int colIndex /*= 0*/) const
    {
        if (first.model() == this && second.model() == this)
        {
            const auto* firstElem = first.unsafe<ISimpleListViewElement>();
            const auto* secondElem = second.unsafe<ISimpleListViewElement>();

            if (firstElem && secondElem)
                return firstElem->compare(secondElem);
        }

        return first < second;
    }

    virtual bool filter(const ModelIndex& id, const SearchPattern& filter, int colIndex /*= 0*/) const
    {
        if (id.model() == this)
        {
            if (const auto* elem = id.unsafe<ISimpleListViewElement>())
            {
                return elem->filter(filter, colIndex);
            }
        }

        return false;
    }

    virtual StringBuf displayContent(const ModelIndex& id, int colIndex /*= 0*/) const override
    {
        return "";
    }

    virtual PopupPtr contextMenu(AbstractItemView* view, const Array<ModelIndex>& indices) const override
    {
        return nullptr;
    }

    virtual ElementPtr tooltip(AbstractItemView* view, ModelIndex id) const override
    {
        return nullptr;
    }

    virtual void visualize(const ModelIndex& id, int columnCount, ElementPtr& content) const override
    {
        if (id.model() == this)
        {
            if (auto* elem = id.unsafe<ISimpleListViewElement>())
            {
                content = AddRef(static_cast<IElement*>(elem));
            }
        }
    }

    //--

    void removeAllElements()
    {
        while (!m_elements.empty())
        {
            auto elem = m_elements.back();
            m_elements.popBack();

            auto index = elem->m_index;
            elem->m_index = ui::ModelIndex();
            elem->m_listIndex = -1;

            notifyItemRemoved(ModelIndex(), index);
        }
    }

    void addElement(ISimpleListViewElement* element)
    {
        DEBUG_CHECK_RETURN_EX(element, "Invalid element");
        DEBUG_CHECK_RETURN_EX(element->parentElement() == nullptr, "Added element should not be parented");
        DEBUG_CHECK_RETURN_EX(!element->index().valid(), "Added element should not be in another list");

        element->m_index = ui::ModelIndex(this, element);
        element->m_listIndex = m_elements.size();
        m_elements.pushBack(AddRef(element));

        notifyItemAdded(ModelIndex(), element->m_index);
    }

    void removeElement(ISimpleListViewElement* element)
    {
        DEBUG_CHECK_RETURN_EX(element, "Invalid element");
        DEBUG_CHECK_RETURN_EX(element->index().valid(), "Removed element should be in this list");
        DEBUG_CHECK_RETURN_EX(element->m_index.model() == this, "Removed element should be in this list");

        if (m_elements.remove(element))
        {
            auto index = element->m_index;
            element->m_index = ModelIndex();
            element->m_listIndex = -1;
            notifyItemRemoved(ModelIndex(), index);

            for (auto i : m_elements.indexRange())
                m_elements[i]->m_listIndex = i;
        }
    }

private:
    Array<RefPtr<ISimpleListViewElement>> m_elements;
};

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(SimpleListView);
RTTI_END_TYPE();

SimpleListView::SimpleListView()
{
    m_container = RefNew<SimpleListViewModel>();
    model(m_container);
}

void SimpleListView::clear()
{
    m_container->removeAllElements();
}

const Array<RefPtr<ISimpleListViewElement>>& SimpleListView::elements() const
{
    return m_container->elements();
}

void SimpleListView::add(ISimpleListViewElement* element)
{
    m_container->addElement(element);
}

void SimpleListView::remove(ISimpleListViewElement* element)
{
    m_container->removeElement(element);
}

void SimpleListView::select(ISimpleListViewElement* element, bool postEvent /*= true*/)
{
    ModelIndex index;
    if (element && element->index().model() == m_container)
        index = element->index();

    TBaseClass::select(index, ItemSelectionModeBit::Default, postEvent);
}

ISimpleListViewElement* SimpleListView::selected() const
{
    if (auto id = TBaseClass::selectionRoot())
        if (id.model() == m_container)
            return id.unsafe<ISimpleListViewElement>();

    return nullptr;
}

//---

END_BOOMER_NAMESPACE_EX(ui)
