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

#include "core/input/include/inputStructures.h"
#include "core/object/include/rttiDataView.h"
#include "core/object/include/action.h"
#include "core/object/include/actionHistory.h"
#include "core/object/include/dataView.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(DataProperty);
    RTTI_METADATA(ElementClassNameMetadata).name("DataProperty");
RTTI_END_TYPE();

DataProperty::DataProperty(DataInspector* inspector, DataInspectorNavigationItem* parent, uint8_t indent, const StringBuf& path, const StringBuf& caption, const DataViewInfo& info, bool parentReadOnly, int arrayIndex)
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
    const auto resetable = m_viewInfo.flags.test(DataViewInfoFlagBit::ResetableToBaseValue);
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
    return m_parentReadOnly || m_viewInfo.flags.test(DataViewInfoFlagBit::ReadOnly) || inspector()->readOnly();
}

bool DataProperty::isDynamicArray() const
{
    return m_viewInfo.flags.test(DataViewInfoFlagBit::DynamicArray) && m_viewInfo.flags.test(DataViewInfoFlagBit::LikeArray);
}

bool DataProperty::isArray() const
{
    return m_viewInfo.flags.test(DataViewInfoFlagBit::LikeArray);
}

/*bool DataProperty::initViewInfo()
{
    m_viewInfo = DataViewInfo();
    m_viewInfo.requestFlags |= DataViewRequestFlagBit::PropertyMetadata;
    m_viewInfo.requestFlags |= DataViewRequestFlagBit::TypeMetadata;

    const auto ret = inspector()->data()->describeDataView(path(), m_viewInfo);
    return ret.valid();
}*/

void DataProperty::initInterface(const StringBuf& caption)
{
    // create the horizontal sizer for this item
    auto line = createChildWithType<IElement>("DataPropertyLine"_id);
    line->layoutColumns();
    line->customHorizontalAligment(ElementHorizontalLayout::Expand);
    line->customVerticalAligment(ElementVerticalLayout::Expand);

    // name element
    auto name = line->createChildWithType<IElement>("DataPropertyName"_id);
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
    auto value = line->createChildWithType<IElement>("DataPropertyValue"_id);
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

    if (m_viewInfo.flags.test(DataViewInfoFlagBit::LikeArray) && m_viewInfo.flags.test(DataViewInfoFlagBit::DynamicArray))
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
    else if (m_viewInfo.flags.test(DataViewInfoFlagBit::LikeValue) && m_viewInfo.flags.test(DataViewInfoFlagBit::Inlined))
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

    if (m_viewInfo.flags.test(DataViewInfoFlagBit::LikeArray))
    {
        if (m_viewInfo.arraySize > 0)
            expandable = true;
    }
    else if (m_viewInfo.flags.test(DataViewInfoFlagBit::LikeStruct))
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
        if (m_viewInfo.flags.test(DataViewInfoFlagBit::LikeArray))
        {
            if (m_viewInfo.arraySize == 0)
                m_valueText->text(TempString("Empty"));
            else if (m_viewInfo.arraySize == 1)
                m_valueText->text(TempString("1 element"));
            else
                m_valueText->text(TempString("{} elements", m_viewInfo.arraySize));
        }
        else if (m_viewInfo.dataType.metaType() == MetaType::StrongHandle)
        {
            ObjectPtr ptr;
            const auto ret = inspector()->data()->readDataView(path(), &ptr, m_viewInfo.dataType);

            if (ret.code == DataViewResultCode::OK)
            {
                if (ptr)
                {
                    m_valueText->text(ptr->cls()->name().view());
                }
                else
                {
                    const auto requiredClass = m_viewInfo.dataType.innerType();
                    m_valueText->text(TempString("Null ({})", requiredClass.name()));
                }
            }
            else if (ret.code == DataViewResultCode::OK)
            {
                m_valueText->text("[i]<multiple values>[/i]");
            }
            else
            {
                m_valueText->text(TempString("[tag:#F00][img:error] {}[/tag]", ret));
            }
        }
        else
        {
            StringBuf displayText;
            static const auto displayTextType = GetTypeObject<StringBuf>();
            const auto ret = inspector()->data()->readDataView(TempString("{}.__text", path()), &displayText, displayTextType);
                
            if (ret.valid())
                m_valueText->text(displayText);
            else
                m_valueText->text(m_viewInfo.dataType->name().c_str());
        }
    }
}

void DataProperty::dispatchAction(const DataViewActionResult& action)
{
    if (action.action)
    {
        const auto executed = inspector()->actionHistory()
            ? inspector()->actionHistory()->execute(action.action)
            : action.action->execute();

        if (!executed)
            PostNotificationMessage(this, MessageType::Warning, "PropertyAction"_id, "Unable to perform action");
    }
    else
    {
        PostNotificationMessage(this, MessageType::Warning, "PropertyAction"_id, TempString("DataError: {}", action));
    }
}

void DataProperty::inlineObjectClear()
{
    dispatchAction(inspector()->data()->actionObjectClear(path()));
}

void DataProperty::inlineObjectNew()
{
    if (!m_classPicker && m_viewInfo.dataType.metaType() == MetaType::StrongHandle && m_viewInfo.flags.test(DataViewInfoFlagBit::Inlined))
    {
        const auto baseClass = m_viewInfo.dataType.innerType().toClass();

        ClassType currentClass;

        ObjectPtr ptr;
        const auto ret = inspector()->data()->readDataView(path(), &ptr, m_viewInfo.dataType);
        if (ret.code == DataViewResultCode::OK || ret.code == DataViewResultCode::ErrorManyValues)
        {
            if (ptr)
                currentClass = ptr->cls();

            auto selfRef = RefWeakPtr<DataProperty>(this);

            m_classPicker = RefNew<ClassPickerBox>(baseClass, currentClass, false, false, "Select class for inlined object");

            m_classPicker->bind(EVENT_CLASS_SELECTED) = [selfRef](ClassType type)
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

            m_classPicker->show(this, PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));
        }
    }
}

void DataProperty::inlineObjectNewWithClass(ClassType type)
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

StringBuf MakeArrayElementPath(StringView path, uint32_t index)
{
    return TempString("{}[{}]", path, index);
}

StringBuf MakeStructureElementPath(StringView path, StringView name)
{
    if (path)
        return TempString("{}.{}", path, name);
    else
        return StringBuf(name);
}

void DataProperty::createChildren(Array<RefPtr<DataInspectorNavigationItem>>& outCreatedChildren)
{
    const auto localReadOnly = m_viewInfo.flags.test(DataViewInfoFlagBit::ReadOnly);

    if (m_viewInfo.flags.test(DataViewInfoFlagBit::LikeArray))
    {
        for (uint32_t i = 0; i < m_viewInfo.arraySize; ++i)
        {
            const auto childPath = MakeArrayElementPath(path(), i);

            DataViewInfo childInfo;
            childInfo.requestFlags |= DataViewRequestFlagBit::PropertyEditorData;
            childInfo.requestFlags |= DataViewRequestFlagBit::TypeMetadata;
            childInfo.requestFlags |= DataViewRequestFlagBit::MemberList;
            childInfo.requestFlags |= DataViewRequestFlagBit::CheckIfResetable;

            if (inspector()->describeDataView(childPath, childInfo).valid())
            {
                const auto caption = StringBuf(TempString("[{}]", i));
                const auto item = RefNew<DataProperty>(inspector(), this, m_indent + 1, childPath, caption, childInfo, localReadOnly, i);
                outCreatedChildren.pushBack(item);
            }
        }
    }
    else if (m_viewInfo.flags.test(DataViewInfoFlagBit::LikeStruct))
    {
        InplaceArray<StringID, 100> names;
        for (const auto& info : m_viewInfo.members)
            names.emplaceBack(info.name);

        if (inspector()->settings().sortAlphabetically)
            std::sort(names.begin(), names.end(), [](const StringID& a, const StringID& b) { return a.view() < b.view(); });

        for (const auto& childName : names)
        {
            const auto childPath = MakeStructureElementPath(path(), childName.view());

            DataViewInfo childInfo;
            childInfo.requestFlags |= DataViewRequestFlagBit::PropertyEditorData;
            childInfo.requestFlags |= DataViewRequestFlagBit::TypeMetadata;
            childInfo.requestFlags |= DataViewRequestFlagBit::MemberList;
            childInfo.requestFlags |= DataViewRequestFlagBit::CheckIfResetable;

            if (inspector()->describeDataView(childPath, childInfo).valid())
            {
                const auto item = RefNew<DataProperty>(inspector(), this, m_indent + 1, childPath, StringBuf(childName.view()), childInfo, localReadOnly);
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
            if (auto* prop = rtti_cast<DataProperty>(child).get())
                prop->notifyDataChanged(true);
    }
}

void DataProperty::handlePropertyChanged(StringView fullPath, bool parentNotification)
{
    if (!parentNotification && fullPath == path())
    {
        updateValueText();
        updateExpandable();

        if (m_viewInfo.flags.test(DataViewInfoFlagBit::LikeArray) || m_viewInfo.flags.test(DataViewInfoFlagBit::Inlined))
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

    DataViewInfo viewInfo;
    viewInfo.requestFlags |= DataViewRequestFlagBit::CheckIfResetable;

    if (inspector()->data()->describeDataView(path(), viewInfo).valid())
        resetable = viewInfo.flags.test(DataViewInfoFlagBit::ResetableToBaseValue);

    if (resetable)
        m_viewInfo.flags |= DataViewInfoFlagBit::ResetableToBaseValue;
    else
        m_viewInfo.flags -= DataViewInfoFlagBit::ResetableToBaseValue;

    toggleResetButton();
}

void DataProperty::resetToBaseValue()
{
    if (m_viewInfo.flags.test(DataViewInfoFlagBit::ResetableToBaseValue))
    {
        dispatchAction(inspector()->data()->actionValueReset(path()));
    }
}

//---

END_BOOMER_NAMESPACE_EX(ui)
