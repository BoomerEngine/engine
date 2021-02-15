/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#pragma once

#include "uiElement.h"
#include "uiDataGroup.h"
#include "base/object/include/rttiDataView.h"

namespace ui
{

    class ClassPickerBox;

    ///---

    /// wrapper for single data property
    class BASE_UI_API DataProperty : public DataInspectorNavigationItem, public base::IDataViewObserver
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataProperty, DataInspectorNavigationItem);

    public:
        DataProperty(DataInspector* inspector, DataInspectorNavigationItem* parent, uint8_t indent, const base::StringBuf& path, const base::StringBuf& caption, const base::rtti::DataViewInfo& info, bool parentReadOnly, int arrayIndex=-1);
        virtual ~DataProperty();

        inline DataProperty* parentProperty() const { return base::rtti_cast<DataProperty>(parentItem()); }

        bool isReadOnly() const;
        bool isArray() const;
        bool isDynamicArray() const;

    protected:
        uint8_t m_indent = 0;
        int32_t m_arrayIndex = -1;

        base::rtti::DataViewInfo m_viewInfo;
        bool m_viewDataResetableStyle = false;
        bool m_parentReadOnly = false;

        ElementPtr m_nameLine;
        ElementPtr m_valueLine;

        TextLabelPtr m_nameText;
        TextLabelPtr m_valueText;
        DataBoxPtr m_valueBox;
        ButtonPtr m_resetToBaseButton;

        base::RefPtr<ClassPickerBox> m_classPicker;

        void updateValueText();
        void updateExpandable();

        void compareWithBase();
        void toggleResetButton();

        virtual void handleSelectionLost() override;
        virtual void handleSelectionGain(bool focus) override;
        virtual void createChildren(base::Array<base::RefPtr<DataInspectorNavigationItem>>& outCreatedChildren) override;

        virtual void handlePropertyChanged(base::StringView fullPath, bool parentNotification) override;

        void notifyDataChanged(bool recurseToChildren);

        void initInterface(const base::StringBuf& caption);

        void arrayClear();
        void arrayAddNew();
        void arrayElementDelete();
        void arrayElementInsertBefore();

        void inlineObjectClear();
        void inlineObjectNew();
        void inlineObjectNewWithClass(base::ClassType type);

        void resetToBaseValue();

        void dispatchAction(const base::DataViewActionResult& action);
    };

    ///---

} // ui