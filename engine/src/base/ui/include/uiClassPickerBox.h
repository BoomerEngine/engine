/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#pragma once

#include "uiEventFunction.h"
#include "uiWindow.h"
#include "uiSimpleTreeModel.h"

namespace ui
{
    ///----

    // data model for listing all engine types
    class ClassTreeModel : public SimpleTreeModel<base::ClassType, base::ClassType>
    {
    public:
        ClassTreeModel(base::ClassType rootClass);

    private:
        virtual bool compare(base::ClassType a, base::ClassType b, int colIndex) const override;
        virtual bool filter(base::ClassType data, const SearchPattern& filter, int colIndex = 0) const override;
        virtual base::StringBuf displayContent(base::ClassType data, int colIndex = 0) const override;
    };

    ///----

    // helper dialog that allows to select a type from type list
    class ClassPickerBox : public PopupWindow
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ClassPickerBox, PopupWindow);

    public:
        ClassPickerBox(base::ClassType rootClass, base::ClassType initialType, bool allowAbstract, bool allowNull, base::StringView<char> caption="");

        // generated OnTypeSelected when selected and general OnClosed when window itself is closed

    private:
        TreeView* m_tree;
        base::RefPtr<ClassTreeModel> m_treeModel;

        bool m_allowAbstract;
        bool m_allowNull;

        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
        void closeWithType(base::ClassType value);
        void closeIfValidTypeSelected();
    };

    ///----

} // ui