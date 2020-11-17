/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once

#include "base/script/include/scriptObject.h"

namespace game
{
    //---

    /// game input event
    class GAME_HOST_API InputEvent : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(InputEvent, base::IObject);

    public:
        base::StringID m_name;
        float m_deltaValue = 0.0f;
        float m_absoluteValue = 0.0f; 
    };

    //---

    /// context for game input handling
    class GAME_HOST_API InputContext : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(InputContext, base::script::ScriptedObject);

    public:
        InputContext();
        InputContext(const InputDefinitionsPtr& defs); // starts at root
        InputContext(const InputActionTablePtr& table);

        //--

        // enter an input sub-context, may fail if current context does not have specified context
        bool enterSubContext(base::StringID name);

        // leave input sub context
        void leaveSubContext(base::StringID name);

        // switch context to a totally different one, removes everything from the stack
        void switchContext(base::StringView name);

        //--

        // process input event, will eat it if there's an action for it
        bool handleInputEvent(const base::input::BaseEvent& evt, InputEventPtr& outEvent);

        //--

    public:
        InputActionTablePtr m_table; // current input table

        base::HashSet<int> m_consumedPresses;
        base::HashMap<base::StringID, float> m_buttonValues;
        base::HashMap<base::StringID, float> m_axisValues;
    };

    //---

} // game