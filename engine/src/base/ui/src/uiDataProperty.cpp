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
#include "uiDataProperty.h"

#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiSplitter.h"
#include "uiGroup.h"
#include "uiButton.h"
#include "uiElement.h"
#include "uiTextLabel.h"
#include "uiDataGroup.h"
#include "uiClassPickerBox.h"

#include "base/input/include/inputStructures.h"
#include "base/object/include/rttiDataView.h"
#include "base/object/include/action.h"
#include "base/object/include/actionHistory.h"
#include "base/object/include/dataView.h"

namespace ui
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(DataProperty);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("DataProperty");
    RTTI_END_TYPE();

    DataProperty::DataProperty(DataInspector* inspector, DataInspectorNavigationItem* parent, uint8_t indent, const base::StringBuf& path, const base::StringBuf& caption, const base::rtti::DataViewInfo& info, bool parentReadOnly, int arrayIndex)
        : DataInspectorNavigationItem(inspector, parent, path, caption)
        , m_arrayIndex(arrayIndex)
        , m_indent(indent)
        , m_parentReadOnly(parentReadOnly)
        , m_viewInfo(info)
    {
        inspector->data()->attachObserver(path, this);
        initInterface(caption);
    }

    DataProperty::~DataProperty()
    {
        inspector()->data()->detachObserver(path(), this);

        if (m_classPicker)
        {
            m_classPicker->requestClose();
            m_classPicker.reset();
        }        
    }

    void DataProperty::toggleResetButton()
    {
        const auto resetable = m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::ResetableToBaseValue);
        if (m_viewDataResetableStyle != resetable)
        {
            m_viewDataResetableStyle = resetable;

            if (m_resetToBaseButton)
                m_resetToBaseButton->visibility(resetable);

            if (m_nameText)
                if (resetable)
                    m_nameText->addStyleClass("resetable"_id);
                else
                    m_nameText->removeStyleClass("resetable"_id);
        }
    }

    bool DataProperty::isReadOnly() const
    {
        return m_parentReadOnly || m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::ReadOnly) || inspector()->readOnly();
    }

    bool DataProperty::isDynamicArray() const
    {
        return m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::DynamicArray) && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray);
    }

    bool DataProperty::isArray() const
    {
        return m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray);
    }

    /*bool DataProperty::initViewInfo()
    {
        m_viewInfo = base::rtti::DataViewInfo();
        m_viewInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::PropertyMetadata;
        m_viewInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::TypeMetadata;

        const auto ret = inspector()->data()->describeDataView(path(), m_viewInfo);
        return ret.valid();
    }*/

    void DataProperty::initInterface(const base::StringBuf& caption)
    {
        // create the horizontal sizer for this item
        auto line = createChildWithType<ui::IElement>("DataPropertyLine"_id);
        line->layoutColumns();
        line->customHorizontalAligment(ElementHorizontalLayout::Expand);
        line->customVerticalAligment(ElementVerticalLayout::Expand);

        // name element
        auto name = line->createChildWithType<ui::IElement>("DataPropertyName"_id);
        auto margins = Offsets(20.0f * m_indent, 0.0f, 0.0f, 0.0f);
        name->customMargins(margins);
        name->layoutHorizontal();
        m_nameLine = name;
        
        // create the expand button
        name->attachChild(createExpandButton());

        // create the name text
        m_nameText = name->createChildWithType<TextLabel>("DataPropertyCaption"_id, caption);

        //--

        // value element
        auto value = line->createChildWithType<ui::IElement>("DataPropertyValue"_id);
        value->layoutHorizontal();
        m_valueLine = value;

        // create the value box
        if (m_valueBox = IDataBox::CreateForType(m_viewInfo))
        {
            m_valueBox->bindData(inspector()->data(), path(), isReadOnly());
            m_valueBox->bindActionHistory(inspector()->actionHistory());
            value->attachChild(m_valueBox);
        }

        // create a stub
        if (!m_valueBox)
        {
            m_valueText = value->createChildWithType<TextLabel>("DataPropertyStaticValue"_id);
            m_valueText->customVerticalAligment(ElementVerticalLayout::Middle);
            m_valueText->customHorizontalAligment(ElementHorizontalLayout::Expand);
            updateValueText();
        }

        //-- Buttons

        if (m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray) && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::DynamicArray))
        {
            {
                auto but = m_valueLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:table_sort] [size:-]Clear");
                but->tooltip("Remove all elements from array");
                but->bind(EVENT_CLICKED) = [this]() { arrayClear(); };
            }

            {
                auto but = m_valueLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:table_refresh] [size:-]New");
                but->tooltip("Add new element at the end");
                but->bind(EVENT_CLICKED) = [this]() { arrayAddNew(); };
            }
        }
        else if (m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeValue) && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::Inlined))
        {
            {
                auto but = m_valueLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:delete] [size:-]Clear");
                but->tooltip("Delete inlined object");
                but->bind(EVENT_CLICKED) = [this]() { inlineObjectClear(); };
            }

            {
                auto but = m_valueLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:add] [size:-]New");
                but->tooltip("Create new inlined object");
                but->bind(EVENT_CLICKED) = [this]() { inlineObjectNew(); };
            }
        }

        if (m_arrayIndex != -1 && parentProperty() && parentProperty()->isDynamicArray())
        {
            {
                auto but = m_nameLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:table_row_delete]");
                but->tooltip("Remove this element from table");
                but->bind(EVENT_CLICKED) = [this]() { arrayElementDelete(); };
            }

            {
                auto but = m_nameLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:table_row_insert]");
                but->tooltip("Insert new element before this element");
                but->bind(EVENT_CLICKED) = [this]() { arrayElementInsertBefore(); };
            }
        }

        {
            auto but = m_nameLine->createChildWithType<Button>("DataPropertyButton"_id);
            but->createChild<TextLabel>("[img:arrow_refresh]");
            but->tooltip("Reset to base value");
            but->bind(EVENT_CLICKED) = [this]() { resetToBaseValue(); };
            but->visibility(false);
            m_resetToBaseButton = but;
        }

        //--

        // update the expandable state
        updateExpandable();

        // update the reset button
        toggleResetButton();
    }

    void DataProperty::updateExpandable()
    {
        bool expandable = false;

        if (m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray))
        {
            if (m_viewInfo.arraySize > 0)
                expandable = true;
        }
        else if (m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeStruct))
        {
            expandable = true;
        }

        // if we are an array/structure with no defined value box than allow for expanding
        if (m_valueBox && !m_valueBox->canExpandChildren())
            expandable = false;

        // update the expandability
        changeExpandable(expandable);
    }

    void DataProperty::updateValueText()
    {
        if (m_valueText)
        {
            if (m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray))
            {
                if (m_viewInfo.arraySize == 0)
                    m_valueText->text(base::TempString("Empty"));
                else if (m_viewInfo.arraySize == 1)
                    m_valueText->text(base::TempString("1 element"));
                else
                    m_valueText->text(base::TempString("{} elements", m_viewInfo.arraySize));
            }
            else if (m_viewInfo.dataType.metaType() == base::rtti::MetaType::StrongHandle)
            {
                base::ObjectPtr ptr;
                const auto ret = inspector()->data()->readDataView(path(), &ptr, m_viewInfo.dataType);

                if (ret.code == base::DataViewResultCode::OK)
                {
                    if (ptr)
                    {
                        m_valueText->text(ptr->cls()->name().view());
                    }
                    else
                    {
                        const auto requiredClass = m_viewInfo.dataType.innerType();
                        m_valueText->text(base::TempString("Null ({})", requiredClass.name()));
                    }
                }
                else if (ret.code == base::DataViewResultCode::OK)
                {
                    m_valueText->text("[i]<multiple values>[/i]");
                }
                else
                {
                    m_valueText->text(base::TempString("[tag:#F00][img:error] {}[/tag]", ret));
                }
            }
            else
            {
                base::StringBuf displayText;
                static const auto displayTextType = base::reflection::GetTypeObject<base::StringBuf>();
                const auto ret = inspector()->data()->readDataView(base::TempString("{}.__text", path()), &displayText, displayTextType);
                
                if (ret.valid())
                    m_valueText->text(displayText);
                else
                    m_valueText->text(m_viewInfo.dataType->name().c_str());
            }
        }
    }

    void DataProperty::dispatchAction(const base::DataViewActionResult& action)
    {
        if (action.action)
        {
            const auto executed = inspector()->actionHistory()
                ? inspector()->actionHistory()->execute(action.action)
                : action.action->execute();

            if (!executed)
                ui::PostNotificationMessage(this, MessageType::Warning, "PropertyAction"_id, "Unable to perform action");
        }
        else
        {
            ui::PostNotificationMessage(this, MessageType::Warning, "PropertyAction"_id, base::TempString("DataError: {}", action));
        }
    }

    void DataProperty::inlineObjectClear()
    {
        dispatchAction(inspector()->data()->actionObjectClear(path()));
    }

    void DataProperty::inlineObjectNew()
    {
        if (!m_classPicker && m_viewInfo.dataType.metaType() == base::rtti::MetaType::StrongHandle && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::Inlined))
        {
            const auto baseClass = m_viewInfo.dataType.innerType().toClass();

            base::ClassType currentClass;

            base::ObjectPtr ptr;
            const auto ret = inspector()->data()->readDataView(path(), &ptr, m_viewInfo.dataType);
            if (ret.code == base::DataViewResultCode::OK || ret.code == base::DataViewResultCode::ErrorManyValues)
            {
                if (ptr)
                    currentClass = ptr->cls();

                auto selfRef = base::RefWeakPtr<DataProperty>(this);

                m_classPicker = base::CreateSharedPtr<ClassPickerBox>(baseClass, currentClass, false, false, "Select class for inlined object");

                m_classPicker->bind(EVENT_CLASS_SELECTED) = [selfRef](base::ClassType type)
                {
                    if (auto prop = selfRef.lock())
                    {
                        prop->inlineObjectNewWithClass(type);
                        prop->m_classPicker.reset();
                    }
                };

                m_classPicker->bind(EVENT_WINDOW_CLOSED) = [selfRef]()
                {
                    if (auto prop = selfRef.lock())
                        prop->m_classPicker.reset();
                };

                m_classPicker->show(this, ui::PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));
            }
        }
    }

    void DataProperty::inlineObjectNewWithClass(base::ClassType type)
    {
        dispatchAction(inspector()->data()->actionObjectNew(path(), type));
    }

    void DataProperty::arrayClear()
    {
        dispatchAction(inspector()->data()->actionArrayClear(path()));
    }

    void DataProperty::arrayAddNew()
    {
        dispatchAction(inspector()->data()->actionArrayNewElement(path()));
    }

    void DataProperty::arrayElementDelete()
    {
        if (auto* parent = parentProperty())
        {
            if (parent->isDynamicArray() && m_arrayIndex >= 0)
            {
                parent->dispatchAction(inspector()->data()->actionArrayRemoveElement(parent->path(), m_arrayIndex));
            }
        }
    }

    void DataProperty::arrayElementInsertBefore()
    {
        if (auto* parent = parentProperty())
        {
            if (parent->isDynamicArray() && m_arrayIndex >= 0)
            {
                parent->dispatchAction(inspector()->data()->actionArrayInsertElement(parent->path(), m_arrayIndex));
            }
        }
    }

    void DataProperty::handleSelectionLost()
    {
        TBaseClass::handleSelectionLost();

        if (m_valueBox)
            m_valueBox->cancelEdit();

        if (m_classPicker)
        {
            m_classPicker->requestClose();
            m_classPicker.reset();
        }
    }

    void DataProperty::handleSelectionGain(bool allowFocus)
    {
        TBaseClass::handleSelectionGain(allowFocus);

        if (allowFocus && m_valueBox)
            m_valueBox->enterEdit();
    }

    base::StringBuf MakeArrayElementPath(base::StringView path, uint32_t index)
    {
        return base::TempString("{}[{}]", path, index);
    }

    base::StringBuf MakeStructureElementPath(base::StringView path, base::StringView name)
    {
        if (path)
            return base::TempString("{}.{}", path, name);
        else
            return base::StringBuf(name);
    }

    void DataProperty::createChildren(base::Array<base::RefPtr<DataInspectorNavigationItem>>& outCreatedChildren)
    {
        const auto localReadOnly = m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::ReadOnly);

        if (m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray))
        {
            for (uint32_t i = 0; i < m_viewInfo.arraySize; ++i)
            {
                const auto childPath = MakeArrayElementPath(path(), i);

                base::rtti::DataViewInfo childInfo;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::PropertyMetadata;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::TypeMetadata;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::MemberList;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::CheckIfResetable;

                if (inspector()->describeDataView(childPath, childInfo).valid())
                {
                    const auto caption = base::StringBuf(base::TempString("[{}]", i));
                    const auto item = base::CreateSharedPtr<DataProperty>(inspector(), this, m_indent + 1, childPath, caption, childInfo, localReadOnly, i);
                    outCreatedChildren.pushBack(item);
                }
            }
        }
        else if (m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeStruct))
        {
            base::InplaceArray<base::StringID, 100> names;
            for (const auto& info : m_viewInfo.members)
                names.emplaceBack(info.name);

            if (inspector()->settings().sortAlphabetically)
                std::sort(names.begin(), names.end(), [](const base::StringID& a, const base::StringID& b) { return a.view() < b.view(); });

            for (const auto& childName : names)
            {
                const auto childPath = MakeStructureElementPath(path(), childName.view());

                base::rtti::DataViewInfo childInfo;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::PropertyMetadata;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::TypeMetadata;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::MemberList;
                childInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::CheckIfResetable;

                if (inspector()->describeDataView(childPath, childInfo).valid())
                {
                    const auto item = base::CreateSharedPtr<DataProperty>(inspector(), this, m_indent + 1, childPath, base::StringBuf(childName.view()), childInfo, localReadOnly);
                    outCreatedChildren.pushBack(item);
                }
            }
        }
    }

    void DataProperty::notifyDataChanged(bool recurseToChildren)
    {
        compareWithBase();

        if (m_valueBox)
            m_valueBox->handleValueChange();

        if (recurseToChildren)
        {
            for (const auto& child : childrenItems())
                if (auto* prop = base::rtti_cast<DataProperty>(child).get())
                    prop->notifyDataChanged(true);
        }
    }

    void DataProperty::handlePropertyChanged(base::StringView fullPath, bool parentNotification)
    {
        if (!parentNotification && fullPath == path())
        {
            updateValueText();
            updateExpandable();

            if (m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray) || m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::Inlined))
            {
                bool flag = expanded();

                collapse();

                if (flag)
                    expand();
            }

            notifyDataChanged(true);
        }
        else
        {
            notifyDataChanged(false);
        }
    }

    //---

    void DataProperty::compareWithBase()
    {
        bool resetable = false;

        base::rtti::DataViewInfo viewInfo;
        viewInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::CheckIfResetable;

        if (inspector()->data()->describeDataView(path(), viewInfo).valid())
            resetable = viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::ResetableToBaseValue);

        if (resetable)
            m_viewInfo.flags |= base::rtti::DataViewInfoFlagBit::ResetableToBaseValue;
        else
            m_viewInfo.flags -= base::rtti::DataViewInfoFlagBit::ResetableToBaseValue;

        toggleResetButton();
    }

    void DataProperty::resetToBaseValue()
    {
        if (m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::ResetableToBaseValue))
        {
            dispatchAction(inspector()->data()->actionValueReset(path()));
        }
    }

    //---

} // ui