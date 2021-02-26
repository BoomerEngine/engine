/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#pragma once

#include "uiEventFunction.h"
#include "uiSimpleTreeModel.h"
#include "uiWindowPopup.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

///----

DECLARE_UI_EVENT(EVENT_CLASS_SELECTED, ClassType)

///----

// data model for listing all engine types
class ClassTreeModel : public SimpleTreeModel<ClassType, ClassType>
{
public:
    ClassTreeModel(ClassType rootClass);

private:
    virtual bool compare(ClassType a, ClassType b, int colIndex) const override;
    virtual bool filter(ClassType data, const SearchPattern& filter, int colIndex = 0) const override;
    virtual StringBuf displayContent(ClassType data, int colIndex = 0) const override;
};

///----

// helper dialog that allows to select a type from type list
class ENGINE_UI_API ClassPickerBox : public PopupWindow
{
    RTTI_DECLARE_VIRTUAL_CLASS(ClassPickerBox, PopupWindow);

public:
    ClassPickerBox(ClassType rootClass, ClassType initialType, bool allowAbstract, bool allowNull, StringView caption="", bool showButtons=true);

    // generated OnTypeSelected when selected and general OnClosed when window itself is closed

private:
    TreeView* m_tree;
    RefPtr<ClassTreeModel> m_treeModel;

    bool m_allowAbstract;
    bool m_allowNull;
    bool m_hasButtons;

    virtual bool handleKeyEvent(const input::KeyEvent& evt) override;
    void closeWithType(ClassType value);
    void closeIfValidTypeSelected();
};

///----

END_BOOMER_NAMESPACE_EX(ui)
