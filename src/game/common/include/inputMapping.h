/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
***/

#pragma once
#include "core/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// type of the input action
enum class InputActionType : uint8_t
{
    Button, // we receive +1 when pressed -1 when released, can be bound to keyboard, mouse buttons, mouse wheel and pad buttons, multiple keys can address the same action (button state is accumuated)
    Axis, // mouse axis/pad axis, we receive a delta every frame -inf to +inf, scaled by mouse sensitivity/acceleration, for pad values we output the absolute value instead
};

/// input action
struct GAME_COMMON_API GameInputAction
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(GameInputAction);

public:
    StringID name;
    InputActionType type = InputActionType::Button;
    bool invert = false;

    InputKey defaultKey = InputKey::KEY_MAX; // default binding for key
    InputAxis defaultAxis = InputAxis::AXIS_MAX; // default binding for axis

    StringBuf mappingName; // name of this action in user mapping, only if mappable
    StringBuf mappingGroup; // group for user mapping (ie. Driving, Movement, etc)

    GameInputAction();
};

//---

/// table of input actions that are to be used together
class GAME_COMMON_API GameInputActionTable : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(GameInputActionTable, IObject);

public:
    GameInputActionTable();

    // name of the context
    INLINE const StringID& name() const { return m_name; }

    // actions
    INLINE const Array<GameInputAction>& actions() const { return m_actions; }

    // child contexts
    INLINE const Array<InputActionTablePtr>& children() const { return m_children; }

private:
    StringID m_name;
    Array<GameInputAction> m_actions;
    Array<InputActionTablePtr> m_children;
};

//---

END_BOOMER_NAMESPACE()
