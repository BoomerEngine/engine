/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once

#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// game input event
class GAME_COMMON_API InputEvent : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(InputEvent, IObject);

public:
    StringID m_name;
    float m_deltaValue = 0.0f;
    float m_absoluteValue = 0.0f; 
};

//---

/// context for game input handling
class GAME_COMMON_API InputContext : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(InputContext, IObject);

public:
    InputContext();
    InputContext(const InputDefinitionsPtr& defs); // starts at root
    InputContext(const InputActionTablePtr& table);

    //--

    // enter an input sub-context, may fail if current context does not have specified context
    bool enterSubContext(StringID name);

    // leave input sub context
    void leaveSubContext(StringID name);

    // switch context to a totally different one, removes everything from the stack
    void switchContext(StringView name);

    //--

    // process input event, will eat it if there's an action for it
    bool handleInputEvent(const input::BaseEvent& evt, InputEventPtr& outEvent);

    //--

public:
    InputActionTablePtr m_table; // current input table

    HashSet<int> m_consumedPresses;
    HashMap<StringID, float> m_buttonValues;
    HashMap<StringID, float> m_axisValues;
};

//---

END_BOOMER_NAMESPACE()
