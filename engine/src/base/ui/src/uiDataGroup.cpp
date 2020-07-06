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

#include "base/input/include/inputStructures.h"
#include "base/object/include/rttiDataView.h"
#include "uiInputAction.h"

namespace ui
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(DataInspectorNavigationItem);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("DataInspectorNavigationItem");
    RTTI_END_TYPE();

    DataInspectorNavigationItem::DataInspectorNavigationItem(DataInspector* inspector, DataInspectorNavigationItem* parentNavigation, const base::StringBuf& path, const base::StringBuf& caption)
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

            base::Array<base::RefPtr< DataInspectorNavigationItem>> createdChildren;
            createChildren(createdChildren);

            for (const auto& child : createdChildren)
            {
                m_children.pushBack(child);
                attachChild(child);
            }
        }
    }

    void DataInspectorNavigationItem::createChildren(base::Array<base::RefPtr<DataInspectorNavigationItem>>& outCreatedChildren)
    {

    }

    InputActionPtr DataInspectorNavigationItem::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
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

    void DataInspectorNavigationItem::collect(base::Array<DataInspectorNavigationItem*>& outItems)
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
            m_expandButton = base::CreateSharedPtr<Button>();
            m_expandButton->styleType("DataInspectorExpandButton"_id);
            m_expandButton->createChild<TextLabel>();
            m_expandButton->bind("OnClick"_id) = [this]()
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

    DataInspectorGroup::DataInspectorGroup(DataInspector* inspector, DataInspectorNavigationItem* parent, const base::StringBuf& path, const base::StringBuf& name)
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

    DataInspectorObjectCategoryGroup::DataInspectorObjectCategoryGroup(DataInspector* inspector, DataInspectorNavigationItem* parent, const base::StringBuf& path, base::StringID category)
        : DataInspectorGroup(inspector, parent, path, base::StringBuf(category.view()))
        , m_category(category)
    {
        changeExpandable(true);
    }

    extern base::Array<base::StringID> GatherPropertyNames(bool sortNames, const base::rtti::DataViewInfo& info, base::StringID category);
    extern base::StringBuf MakeStructureElementPath(base::StringView<char> path, base::StringView<char> name);

    void DataInspectorObjectCategoryGroup::createChildren(base::Array<base::RefPtr<DataInspectorNavigationItem>>& outCreatedChildren)
    {
        base::rtti::DataViewInfo info;
        info.requestFlags |= base::rtti::DataViewRequestFlagBit::MemberList;
        info.categoryFilter = m_category;

        if (inspector()->data()->describeDataView(path(), info).valid())
        {
            const auto names = GatherPropertyNames(inspector()->settings().sortAlphabetically, info, m_category);
            for (const auto& name : names)
            {
                const auto childPath = MakeStructureElementPath(path(), name.view());

                base::rtti::DataViewInfo childInfo;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::PropertyMetadata;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::TypeMetadata;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::MemberList;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::CheckIfResetable;

                if (inspector()->data()->describeDataView(childPath, childInfo).valid())
                {
                    auto prop = base::CreateSharedPtr<DataProperty>(inspector(), this, 0, childPath, childPath, childInfo, false);
                    outCreatedChildren.pushBack(prop);
                }
            }
        }
    }

    //---

} // ui