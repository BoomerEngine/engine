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
#include "uiClassPickerBox.h"

#include "base/input/include/inputStructures.h"
#include "base/object/include/rttiDataView.h"

namespace ui
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(DataProperty);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("DataProperty");
    RTTI_END_TYPE();

    DataProperty::DataProperty(DataInspector* inspector, DataInspectorNavigationItem* parent, uint8_t indent, const base::StringBuf& path, const base::StringBuf& caption, bool parentReadOnly, int arrayIndex)
        : DataInspectorNavigationItem(inspector, parent, path, caption)
        , m_arrayIndex(arrayIndex)
        , m_indent(indent)
        , m_parentReadOnly(parentReadOnly)
    {
        m_observerToken = inspector->data()->registerObserver(path, this);

        initViewInfo();
        initInterface(caption);
        compareWithBase();
    }

    DataProperty::~DataProperty()
    {
        if (nullptr != m_observerToken)
        {
            inspector()->data()->unregisterObserver(m_observerToken);
            m_observerToken = nullptr;
        }

        if (m_classPicker)
        {
            m_classPicker->requestClose();
            m_classPicker.reset();
        }        
    }

    void DataProperty::toggleResetButton(bool state)
    {
        if (m_viewDataResetable != state)
        {
            m_viewDataResetable = state;

            if (m_resetToBaseButton)
                m_resetToBaseButton->visibility(state);

            if (m_nameText)
                if (state)
                    m_nameText->addStyleClass("resetable"_id);
                else
                    m_nameText->removeStyleClass("resetable"_id);
        }
    }

    bool DataProperty::isReadOnly() const
    {
        return m_parentReadOnly | (m_viewInfoConformed && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::ReadOnly));
    }

    bool DataProperty::isDynamicArray() const
    {
        return m_viewInfoConformed && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::DynamicArray) && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray);
    }

    bool DataProperty::isArray() const
    {
        return m_viewInfoConformed && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray);
    }

    void DataProperty::initViewInfo()
    {
        m_viewInfoConformed = false; // we may fail
        m_viewInfo = base::rtti::DataViewInfo();

        if (const auto count = inspector()->data()->size()) // we need at least one object
        {
            m_viewInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::PropertyMetadata;
            m_viewInfo.requestFlags |= base::rtti::DataViewRequestFlagBit::TypeMetadata;

            m_viewInfoConformed = inspector()->data()->describe(0, path(), m_viewInfo);
            if (m_viewInfoConformed) // that has a valid info
            {
                // check with the other objects in the view
            }
        }
    }

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
        if (m_viewInfoConformed)
        {
            if (m_valueBox = IDataBox::CreateForType(m_viewInfo))
            {
                m_valueBox->bind(inspector()->data(), path(), isReadOnly());
                value->attachChild(m_valueBox);
            }
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

        if (m_viewInfoConformed && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray) && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::DynamicArray))
        {
            {
                auto but = m_valueLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:table_sort] [size:-]Clear");
                but->tooltip("Remove all elements from array");
                but->bind("OnClick"_id) = [this]() { arrayClear(); };
            }

            {
                auto but = m_valueLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:table_refresh] [size:-]New");
                but->tooltip("Add new element at the end");
                but->bind("OnClick"_id) = [this]() { arrayAddNew(); };
            }
        }
        else if (m_viewInfoConformed && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeValue) && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::Inlined))
        {
            {
                auto but = m_valueLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:delete] [size:-]Clear");
                but->tooltip("Delete inlined object");
                but->bind("OnClick"_id) = [this]() { inlineObjectClear(); };
            }

            {
                auto but = m_valueLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:add] [size:-]New");
                but->tooltip("Create new inlined object");
                but->bind("OnClick"_id) = [this]() { inlineObjectNew(); };
            }
        }

        if (m_arrayIndex != -1 && parentProperty() && parentProperty()->isDynamicArray())
        {
            {
                auto but = m_nameLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:table_row_delete]");
                but->tooltip("Remove this element from table");
                but->bind("OnClick"_id) = [this]() { arrayElementDelete(); };
            }

            {
                auto but = m_nameLine->createChildWithType<Button>("DataPropertyButton"_id);
                but->createChild<TextLabel>("[img:table_row_insert]");
                but->tooltip("Insert new element before this element");
                but->bind("OnClick"_id) = [this]() { arrayElementInsertBefore(); };
            }
        }

        {
            auto but = m_nameLine->createChildWithType<Button>("DataPropertyButton"_id);
            but->createChild<TextLabel>("[img:arrow_refresh]");
            but->tooltip("Reset to base value");
            but->bind("OnClick"_id) = [this]() { resetToBaseValue(); };
            but->visibility(false);
            m_resetToBaseButton = but;
        }

        //--

        // update the expandable state
        updateExpandable();

        // compare with base
        compareWithBase();
    }

    static void CollectConformedStructMembers(base::DataProxy* proxy, base::StringView<char> path, base::Array<base::StringID>& outMembers)
    {
        base::HashMap<base::StringID, uint32_t> names;

        const auto count = proxy->size();
        for (uint32_t i = 0; i < count; ++i)
        {
            base::rtti::DataViewInfo info;
            info.requestFlags |= base::rtti::DataViewRequestFlagBit::MemberList;

            if (proxy->describe(i, path, info))
            {
                for (const auto& member : info.members)
                    names[member.name] += 1;
            }
        }

        outMembers.reserve(names.size());

        for (uint32_t i = 0; i < names.size(); ++i)
        {
            if (names.values()[i] == count) // property was reported in all structures
            {
                outMembers.pushBack(names.keys()[i]);
            }
        }
    }

    void DataProperty::updateExpandable()
    {
        bool expandable = false;

        if (m_viewInfoConformed && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray))
        {
            if (m_viewInfo.arraySize > 0)
                expandable = true;
        }
        else if (m_viewInfoConformed && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeStruct))
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
        if (!m_valueText)
            return;

        if (m_viewInfoConformed)
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
                if (inspector()->data()->read(0, path(), &ptr, m_viewInfo.dataType))
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
                else
                {
                    m_valueText->text("[i]Undetermined object[/i]");
                }
            }
            else
            {
                m_valueText->text(m_viewInfo.dataType->name().c_str());
            }
        }
    }

    void DataProperty::sendViewCommand(const base::rtti::DataViewCommand& cmd)
    {
        bool success = true;

        const auto count = inspector()->data()->size();
        for (uint32_t i = 0; i < count; ++i)
            success &= inspector()->data()->write(i, path(), &cmd, base::rtti::DataViewCommand::GetStaticClass());

        if (!success)
        {
            // TODO: show error
        }
    }

    void DataProperty::inlineObjectClear()
    {
        if (m_viewInfoConformed && m_viewInfo.dataType.metaType() == base::rtti::MetaType::StrongHandle && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::Inlined))
        {
            base::rtti::DataViewCommand command;
            command.command = "clear"_id;
            sendViewCommand(command);
        }
    }

    void DataProperty::inlineObjectNew()
    {
        if (!m_classPicker && m_viewInfoConformed && m_viewInfo.dataType.metaType() == base::rtti::MetaType::StrongHandle && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::Inlined))
        {
            const auto baseClass = m_viewInfo.dataType.innerType().toClass();

            base::ClassType currentClass;

            base::ObjectPtr ptr;
            if (inspector()->data()->read(0, path(), &ptr, m_viewInfo.dataType) && ptr)
                currentClass = ptr->cls();

            m_classPicker = base::CreateSharedPtr<ClassPickerBox>(baseClass, currentClass, false, false, "Select class for inlined object");
            m_classPicker->bind("OnClassSelected"_id, this) = [](DataProperty* prop, base::ClassType type)
            {
                prop->inlineObjectNewWithClass(type);
                prop->m_classPicker.reset();
            };
            m_classPicker->bind("OnClosed"_id, this) = [](DataProperty* prop)
            {
                prop->m_classPicker.reset();
            };

            m_classPicker->show(this, ui::PopupWindowSetup().areaCenter().relativeToCursor().autoClose(true).interactive(true));
        }
    }

    void DataProperty::inlineObjectNewWithClass(base::ClassType type)
    {
        if (type)
        {
            base::rtti::DataViewCommand command;
            command.command = "new"_id;
            command.argT = type;
            sendViewCommand(command);

            expand();
        }
    }

    void DataProperty::arrayClear()
    {
        base::rtti::DataViewCommand command;
        command.command = "clear"_id;
        sendViewCommand(command);
    }

    void DataProperty::arrayAddNew()
    {
        base::rtti::DataViewCommand command;
        command.command = "new"_id;
        sendViewCommand(command);
    }

    void DataProperty::arrayElementDelete()
    {
        if (auto* parent = parentProperty())
        {
            if (parent->isDynamicArray() && m_arrayIndex >= 0)
            {
                base::rtti::DataViewCommand command;
                command.command = "delete"_id;
                command.arg0 = m_arrayIndex;
                parent->sendViewCommand(command);
            }
        }
    }

    void DataProperty::arrayElementInsertBefore()
    {
        if (auto* parent = parentProperty())
        {
            if (parent->isDynamicArray() && m_arrayIndex >= 0)
            {
                base::rtti::DataViewCommand command;
                command.command = "insert"_id;
                command.arg0 = m_arrayIndex;
                parent->sendViewCommand(command);
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

    void DataProperty::createChildren(base::Array<base::RefPtr<DataInspectorNavigationItem>>& outCreatedChildren)
    {
        if (m_viewInfoConformed && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray))
        {
            for (uint32_t i = 0; i < m_viewInfo.arraySize; ++i)
            {
                const auto fullPath = base::StringBuf(base::TempString("{}[{}]", path(), i));
                const auto caption = base::StringBuf(base::TempString("[{}]", i));
                const auto item = base::CreateSharedPtr<DataProperty>(inspector(), this, m_indent + 1, fullPath, caption, isReadOnly(), i);
                outCreatedChildren.pushBack(item);
            }
        }
        else if (m_viewInfoConformed && m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeStruct))
        {
            base::InplaceArray<base::StringID, 100> names;
            CollectConformedStructMembers(inspector()->data(), path(), names);

            // TODO: sort members by name

            for (const auto& name : names)
            {
                const auto fullPath = base::StringBuf(base::TempString("{}{}{}", path(), path().empty() ? "" : ".", name));
                const auto item = base::CreateSharedPtr<DataProperty>(inspector(), this, m_indent + 1, fullPath, name.c_str(), isReadOnly());
                outCreatedChildren.pushBack(item);
            }
        }
    }

    void DataProperty::notifyDataChanged(bool recurseToChildren)
    {
        if (m_valueBox)
            m_valueBox->handleValueChange();

        compareWithBase();

        if (recurseToChildren)
        {
            for (const auto& child : childrenItems())
                if (auto* prop = base::rtti_cast<DataProperty>(child).get())
                    prop->notifyDataChanged(true);
        }
    }

    void DataProperty::dataProxyValueChanged(base::StringView<char> fullPath, bool parentNotification)
    {
        if (!parentNotification && fullPath == path())
        {
            initViewInfo();

            updateValueText();
            updateExpandable();

            if (m_viewInfoConformed && (m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::LikeArray) || m_viewInfo.flags.test(base::rtti::DataViewInfoFlagBit::Inlined)))
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

        const auto size = inspector()->data()->size();
        for (uint32_t i = 0; i < size; ++i)
        {
            base::rtti::DataViewBaseValue val;

            if (inspector()->data()->read(i, path(), &val, base::rtti::DataViewBaseValue::GetStaticClass()))
                resetable |= val.differentThanBase;
        }

        toggleResetButton(resetable);
    }

    void DataProperty::resetToBaseValue()
    {
        base::rtti::DataViewCommand command;
        command.command = "reset"_id;

        const auto size = inspector()->data()->size();
        for (uint32_t i = 0; i < size; ++i)
            inspector()->data()->write(i, path(), &command, base::rtti::DataViewCommand::GetStaticClass());
    }

    //---

} // ui