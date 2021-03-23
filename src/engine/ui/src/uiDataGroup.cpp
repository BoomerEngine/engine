/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"

#include "uiDataBox.h"
#include "uiDataInspector.h"
#include "uiDataGroup.h"
#include "uiDataProperty.h"

#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiSplitter.h"
#include "uiGroup.h"
#include "uiButton.h"
#include "uiElement.h"
#include "uiTextLabel.h"

#include "core/input/include/inputStructures.h"
#include "core/object/include/rttiDataView.h"
#include "uiInputAction.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataInspectorNavigationItem);
    RTTI_METADATA(ElementClassNameMetadata).name("DataInspectorNavigationItem");
RTTI_END_TYPE();

DataInspectorNavigationItem::DataInspectorNavigationItem(DataInspector* inspector, DataInspectorNavigationItem* parentNavigation, const StringBuf& path, const StringBuf& caption)
    : m_parentItem(parentNavigation)
    , m_inspector(inspector)
    , m_path(path)
    , m_caption(caption)
{
    hitTest(true);
    enableAutoExpand(true, false);
    allowFocusFromKeyboard(false); // handled at the inspector
    allowFocusFromClick(false); // handled at the inspector
}

void DataInspectorNavigationItem::updateExpandStyle()
{
    if (m_expandButton)
    {
        if (!expandable())
        {
            m_expandButton->addStyleClass("nochildren"_id);
        }
        else if (expanded())
        {
            m_expandButton->removeStyleClass("nochildren"_id);
            m_expandButton->addStyleClass("expanded"_id);
        }
        else
        {
            m_expandButton->removeStyleClass("nochildren"_id);
            m_expandButton->removeStyleClass("expanded"_id);
        }
    }
}

void DataInspectorNavigationItem::changeExpandable(bool expandable)
{
    if (m_expandable != expandable)
    {
        m_expandable = expandable;

        if (!expandable && expanded())
            collapse();
        else
            updateExpandStyle();
    }
}
    
void DataInspectorNavigationItem::collapse()
{
    if (m_expanded)
    {
        m_expanded = false;
        updateExpandStyle();

        for (const auto& child : m_children)
            detachChild(child);
        m_children.reset();
    }
}

void DataInspectorNavigationItem::expand()
{
    if (!m_expanded)
    {
        m_expanded = true;
        updateExpandStyle();

        Array<RefPtr< DataInspectorNavigationItem>> createdChildren;
        createChildren(createdChildren);

        for (const auto& child : createdChildren)
        {
            m_children.pushBack(child);
            attachChild(child);
        }
    }
}

void DataInspectorNavigationItem::createChildren(Array<RefPtr<DataInspectorNavigationItem>>& outCreatedChildren)
{

}

InputActionPtr DataInspectorNavigationItem::handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt)
{
    if (evt.leftDoubleClicked() && expandable())
    {
        if (!expanded())
            expand();
        else
            collapse();
        return IInputAction::CONSUME();
    }

    return TBaseClass::handleMouseClick(area, evt);
}

DataInspectorNavigationItem* DataInspectorNavigationItem::itemAtPosition(const Position& pos)
{
    if (cachedDrawArea().contains(pos))
    {
        for (const auto& child : m_children)
            if (auto ret = child->itemAtPosition(pos))
                return ret;

        return this;
    }

    return nullptr;
}

void DataInspectorNavigationItem::collect(Array<DataInspectorNavigationItem*>& outItems)
{
    outItems.pushBack(this);

    if (m_expanded)
    {
        for (const auto& child : m_children)
            child->collect(outItems);
    }
}

void DataInspectorNavigationItem::handleSelectionLost()
{
    removeStyleClass("selected"_id);
}

void DataInspectorNavigationItem::handleSelectionGain(bool shouldFocus)
{
    addStyleClass("selected"_id);

    if (shouldFocus)
        focus();
}

ButtonPtr DataInspectorNavigationItem::createExpandButton()
{
    if (!m_expandButton)
    {
        m_expandButton = RefNew<Button>();
        m_expandButton->styleType("DataInspectorExpandButton"_id);
        m_expandButton->createChild<TextLabel>();
        m_expandButton->bind(EVENT_CLICKED) = [this]()
        {
            if (expanded())
                collapse();
            else if (expandable())
                expand();
        };

        updateExpandStyle();
    }

    return m_expandButton;
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataInspectorGroup);
RTTI_END_TYPE();

DataInspectorGroup::DataInspectorGroup(DataInspector* inspector, DataInspectorNavigationItem* parent, const StringBuf& path, const StringBuf& name)
    : DataInspectorNavigationItem(inspector, parent, path, name)
{
    auto bar = createChildWithType<IElement>("DataInspectorGroup"_id);
    bar->layoutHorizontal();
    bar->attachChild(createExpandButton());
    bar->createChild<TextLabel>(name);
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataInspectorObjectCategoryGroup);
RTTI_END_TYPE();

DataInspectorObjectCategoryGroup::DataInspectorObjectCategoryGroup(DataInspector* inspector, DataInspectorNavigationItem* parent, const StringBuf& path, StringID category)
    : DataInspectorGroup(inspector, parent, path, StringBuf(category.view()))
    , m_category(category)
{
    changeExpandable(true);
}

extern Array<StringID> GatherPropertyNames(bool sortNames, const DataViewInfo& info, StringID category);
extern StringBuf MakeStructureElementPath(StringView path, StringView name);

void DataInspectorObjectCategoryGroup::createChildren(Array<RefPtr<DataInspectorNavigationItem>>& outCreatedChildren)
{
    DataViewInfo info;
    info.requestFlags |= DataViewRequestFlagBit::MemberList;
    info.categoryFilter = m_category;

    if (inspector()->data()->describeDataView(path(), info).valid())
    {
        const auto names = GatherPropertyNames(inspector()->settings().sortAlphabetically, info, m_category);
        for (const auto& name : names)
        {
            const auto childPath = MakeStructureElementPath(path(), name.view());

            DataViewInfo childInfo;
            childInfo.requestFlags |= DataViewRequestFlagBit::PropertyEditorData;
            childInfo.requestFlags |= DataViewRequestFlagBit::TypeMetadata;
            childInfo.requestFlags |= DataViewRequestFlagBit::MemberList;
            childInfo.requestFlags |= DataViewRequestFlagBit::CheckIfResetable;

            if (inspector()->data()->describeDataView(childPath, childInfo).valid())
            {
                auto prop = RefNew<DataProperty>(inspector(), this, 0, childPath, childPath, childInfo, false);
                outCreatedChildren.pushBack(prop);
            }
        }
    }
}

//---

END_BOOMER_NAMESPACE_EX(ui)
