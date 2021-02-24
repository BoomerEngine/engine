/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
*
***/

#include "build.h"
#include "gameInputMapping.h"

BEGIN_BOOMER_NAMESPACE(game)

///--

RTTI_BEGIN_TYPE_ENUM(InputActionType);
    RTTI_ENUM_OPTION(Button);
    RTTI_ENUM_OPTION(Axis);
RTTI_END_TYPE();

///--

RTTI_BEGIN_TYPE_CLASS(InputAction);
    RTTI_PROPERTY(name).editable();
    RTTI_PROPERTY(type).editable();
    RTTI_PROPERTY(invert).editable();
    RTTI_PROPERTY(defaultKey).editable();
    RTTI_PROPERTY(defaultAxis).editable();
    RTTI_PROPERTY(mappingName).editable();
    RTTI_PROPERTY(mappingGroup).editable();
RTTI_END_TYPE();

InputAction::InputAction()
{}

///--

RTTI_BEGIN_TYPE_CLASS(InputActionTable);
    RTTI_PROPERTY(m_name).editable();
    RTTI_PROPERTY(m_actions).editable();
    RTTI_PROPERTY(m_children).editable();
RTTI_END_TYPE();

InputActionTable::InputActionTable()
{}

///--
    
END_BOOMER_NAMESPACE(game)
