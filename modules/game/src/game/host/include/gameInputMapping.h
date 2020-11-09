/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once
#include "base/input/include/inputStructures.h"

namespace game
{
    //---

    /// type of the input action
    enum class InputActionType : uint8_t
    {
        Button, // we receive +1 when pressed -1 when released, can be bound to keyboard, mouse buttons, mouse wheel and pad buttons, multiple keys can address the same action (button state is accumuated)
        Axis, // mouse axis/pad axis, we receive a delta every frame -inf to +inf, scaled by mouse sensitivity/acceleration, for pad values we output the absolute value instead
    };

    /// input action
    struct GAME_HOST_API InputAction
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(InputAction);

    public:
        base::StringID name;
        InputActionType type = InputActionType::Button;
        bool invert = false;

        base::input::KeyCode defaultKey = base::input::KeyCode::KEY_MAX; // default binding for key 
        base::input::AxisCode defaultAxis = base::input::AxisCode::AXIS_MAX; // default binding for axis

        base::StringBuf mappingName; // name of this action in user mapping, only if mappable
        base::StringBuf mappingGroup; // group for user mapping (ie. Driving, Movement, etc)

        InputAction();
    };

    //---

    /// table of input actions that are to be used together
    class GAME_HOST_API InputActionTable : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(InputActionTable, base::IObject);

    public:
        InputActionTable();

        // name of the context
        INLINE const base::StringID& name() const { return m_name; }

        // actions
        INLINE const base::Array<InputAction>& actions() const { return m_actions; }

        // child contexts
        INLINE const base::Array<InputActionTablePtr>& children() const { return m_children; }

    private:
        base::StringID m_name;
        base::Array<InputAction> m_actions;
        base::Array<InputActionTablePtr> m_children;
    };

    //---

} // game