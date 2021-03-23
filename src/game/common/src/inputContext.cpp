/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: input #]
*
***/

#include "build.h"
#include "inputMapping.h"
#include "inputContext.h"
#include "inputDefinitions.h"
#include "core/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE()

///--

RTTI_BEGIN_TYPE_CLASS(GameInputEvent);
    RTTI_PROPERTY(m_name);
    RTTI_PROPERTY(m_deltaValue);
    RTTI_PROPERTY(m_absoluteValue);
RTTI_END_TYPE();

///--

RTTI_BEGIN_TYPE_CLASS(GameInputContext);
RTTI_END_TYPE();

GameInputContext::GameInputContext()
{}

GameInputContext::GameInputContext(const InputDefinitionsPtr& defs)
{
    if (defs)
        m_table = defs->root();
}

GameInputContext::GameInputContext(const InputActionTablePtr& table)
    : m_table(table)
{
}

//--

bool GameInputContext::enterSubContext(StringID name)
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

void GameInputContext::leaveSubContext(StringID name)
{
    auto test = m_table.get();
    while (test)
    {
        auto parent = rtti_cast<GameInputActionTable>(m_table->parent());

        if (test->name() == name)
        {
            m_table = AddRef(parent);
            break;
        }

        test = parent;
    }
}

void GameInputContext::switchContext(StringView name)
{
    // TODO:
}

//--

static const GameInputAction* FindKeyAction(const GameInputActionTable& table, InputKey code)
{
    for (const auto& action : table.actions())
        if (action.type == InputActionType::Button && action.defaultKey == code && action.name)
            return &action;
        
    if (auto parent = rtti_cast<GameInputActionTable>(table.parent()))
        return FindKeyAction(*parent, code);

    return nullptr;
}

static const GameInputAction* FindAxisAction(const GameInputActionTable& table, InputAxis code)
{
    for (const auto& action : table.actions())
        if (action.type == InputActionType::Axis && action.defaultAxis == code && action.name)
            return &action;

    if (auto parent = rtti_cast<GameInputActionTable>(table.parent()))
        return FindAxisAction(*parent, code);

    return nullptr;
}

static bool IsDeltaAxis(InputAxis code)
{
    switch (code)
    {
    case InputAxis::AXIS_MOUSEX:
    case InputAxis::AXIS_MOUSEY:
    case InputAxis::AXIS_MOUSEZ:
        return true;
    }

    return false;
}

bool GameInputContext::handleInputEvent(const InputEvent& evt, GameInputEventPtr& outEvent)
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

                    outEvent = RefNew<GameInputEvent>();
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

                    outEvent = RefNew<GameInputEvent>();
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

                    outEvent = RefNew<GameInputEvent>();
                    outEvent->m_name = action->name;
                    outEvent->m_deltaValue = delta;
                    outEvent->m_absoluteValue = m_axisValues[action->name];
                }
                else
                {
                    auto prev = m_axisValues[action->name];
                    m_axisValues[action->name] = axisEvent->displacement();

                    const auto delta = action->invert ? -axisEvent->displacement() : axisEvent->displacement();

                    outEvent = RefNew<GameInputEvent>();
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
    
END_BOOMER_NAMESPACE()
