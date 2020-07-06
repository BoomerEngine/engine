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
#include "gameInputContext.h"
#include "base/input/include/inputStructures.h"

namespace game
{
    ///--

    RTTI_BEGIN_TYPE_CLASS(InputEvent);
        RTTI_PROPERTY(m_name);
        RTTI_PROPERTY(m_deltaValue);
        RTTI_PROPERTY(m_absoluteValue);
    RTTI_END_TYPE();

    ///--

    RTTI_BEGIN_TYPE_CLASS(InputContext);
    RTTI_END_TYPE();

    InputContext::InputContext()
    {}

    InputContext::InputContext(const InputDefinitionsPtr& defs)
    {
        if (defs)
            m_table = defs->root();
    }

    InputContext::InputContext(const InputActionTablePtr& table)
        : m_table(table)
    {
    }

    //--

    bool InputContext::enterSubContext(base::StringID name)
    {
        if (m_table)
        {
            for (const auto& child : m_table->children())
            {
                if (child->name() == name)
                {
                    m_table = child;
                    return true;
                }
            }
        }

        return false;
    }

    void InputContext::leaveSubContext(base::StringID name)
    {
        auto test = m_table.get();
        while (test)
        {
            auto parent = base::rtti_cast<InputActionTable>(m_table->parent());

            if (test->name() == name)
            {
                m_table = AddRef(parent);
                break;
            }

            test = parent;
        }
    }

    void InputContext::switchContext(base::StringView<char> name)
    {
        // TODO:
    }

    //--

    static const InputAction* FindKeyAction(const InputActionTable& table, base::input::KeyCode code)
    {
        for (const auto& action : table.actions())
            if (action.type == InputActionType::Button && action.defaultKey == code && action.name)
                return &action;
        
        if (auto parent = base::rtti_cast<InputActionTable>(table.parent()))
            return FindKeyAction(*parent, code);

        return nullptr;
    }

    static const InputAction* FindAxisAction(const InputActionTable& table, base::input::AxisCode code)
    {
        for (const auto& action : table.actions())
            if (action.type == InputActionType::Axis && action.defaultAxis == code && action.name)
                return &action;

        if (auto parent = base::rtti_cast<InputActionTable>(table.parent()))
            return FindAxisAction(*parent, code);

        return nullptr;
    }

    static bool IsDeltaAxis(base::input::AxisCode code)
    {
        switch (code)
        {
        case base::input::AxisCode::AXIS_MOUSEX:
        case base::input::AxisCode::AXIS_MOUSEY:
        case base::input::AxisCode::AXIS_MOUSEZ:
            return true;
        }

        return false;
    }

    bool InputContext::handleInputEvent(const base::input::BaseEvent& evt, InputEventPtr& outEvent)
    {
        if (m_table)
        {
            // keyboard/mouse buttons
            if (const auto* keyEvent = evt.toKeyEvent())
            {
                const auto key = keyEvent->keyCode();
                const auto keyCode = (int)key;

                if (keyEvent->pressed())
                {
                    if (const auto* action = FindKeyAction(*m_table, key))
                    {
                        const auto value = action->invert ? -1.0f : 1.0f;

                        DEBUG_CHECK(!m_consumedPresses.contains(keyCode));
                        if (m_consumedPresses.insert(keyCode))
                            m_buttonValues[action->name] += value;

                        outEvent = base::CreateSharedPtr<InputEvent>();
                        outEvent->m_name = action->name;
                        outEvent->m_deltaValue = value;
                        outEvent->m_absoluteValue = m_buttonValues[action->name];
                        return true;
                    }
                }
                else if (keyEvent->released())
                {
                    if (const auto* action = FindKeyAction(*m_table, key))
                    {
                        const auto value = action->invert ? 1.0f : -1.0f;

                        DEBUG_CHECK(m_consumedPresses.contains(keyCode));
                        if (m_consumedPresses.remove(keyCode))
                            m_buttonValues[action->name] += value;

                        outEvent = base::CreateSharedPtr<InputEvent>();
                        outEvent->m_name = action->name;
                        outEvent->m_deltaValue = value;
                        outEvent->m_absoluteValue = m_buttonValues[action->name];
                        return true;
                    }
                    else
                    {
                        m_consumedPresses.remove(keyCode);
                    }
                }
            }

            // axis
            if (const auto* axisEvent = evt.toAxisEvent())
            {
                const auto axis = axisEvent->axisCode();
                if (const auto* action = FindAxisAction(*m_table, axis))
                {
                    if (IsDeltaAxis(axis))
                    {
                        const auto delta = action->invert ? -axisEvent->displacement() : axisEvent->displacement();

                        m_axisValues[action->name] += delta;

                        outEvent = base::CreateSharedPtr<InputEvent>();
                        outEvent->m_name = action->name;
                        outEvent->m_deltaValue = delta;
                        outEvent->m_absoluteValue = m_axisValues[action->name];
                    }
                    else
                    {
                        auto prev = m_axisValues[action->name];
                        m_axisValues[action->name] = axisEvent->displacement();

                        const auto delta = action->invert ? -axisEvent->displacement() : axisEvent->displacement();

                        outEvent = base::CreateSharedPtr<InputEvent>();
                        outEvent->m_name = action->name;
                        outEvent->m_deltaValue = prev - axisEvent->displacement();
                        outEvent->m_absoluteValue = axisEvent->displacement();
                    }

                    return true;
                }
            }
        }

        return false;
    }


    //--
    
} // game
