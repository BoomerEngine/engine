/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity\components\input #]
*
***/

#include "build.h"
#include "worldEntity.h"
#include "worldInputComponent.h"
#include "world.h"
#include "worldInputSystem.h"
#include "game/host/include/gameInputContext.h"
#include "game/host/include/gameInputMapping.h"

namespace game
{
    //--

    RTTI_BEGIN_TYPE_CLASS(InputComponent);
        RTTI_PROPERTY(m_definitions).editable();
        RTTI_PROPERTY(m_initialContextName).editable();
        RTTI_PROPERTY(m_exclusive).editable();
        RTTI_PROPERTY(m_priority).editable();
    RTTI_END_TYPE();

    //--

    InputComponent::InputComponent()
    {
        m_initialContextName = "";
    }

    InputComponent::InputComponent(const InputDefinitionsPtr& defs)
        : m_definitions(defs)
    {}

    InputComponent::~InputComponent()
    {}

    void InputComponent::activate()
    {
        if (!m_active)
        {
            if (auto* inputSystem = system<WorldInputSystem>())
            {
                inputSystem->attachInput(this);
                m_active = true;
            }
        }
    }

    void InputComponent::deactivate()
    {
        if (m_active)
        {
            if (auto* inputSystem = system<WorldInputSystem>())
                inputSystem->detachInput(this);
    
            m_active = false;
        }
    }

    //---

    void InputComponent::handleAttach(World* scene)
    {
        TBaseClass::handleAttach(scene);

        if (const auto def = m_definitions.acquire())
        {
            if (const auto table = def->findTable(m_initialContextName))
            {
                m_context = base::CreateSharedPtr<InputContext>(table);
            }
        }
    }

    void InputComponent::handleDetach(World* scene)
    {
        TBaseClass::handleDetach(scene);

        if (m_active)
        {
            if (auto* inputSystem = scene->system<WorldInputSystem>())
                inputSystem->detachInput(this);
            m_active = false;
        }

        m_context.reset();
    }

    //--

    bool InputComponent::handleInput(const base::input::BaseEvent& evt) const
    {
        if (m_context)
        {
            InputEventPtr gameInputEvent;
            if (m_context->handleInputEvent(evt, gameInputEvent))
            {
                if (entity() && gameInputEvent)
                    entity()->handleGameInput(gameInputEvent);
                return true;
            }
        }

        return false;
    }

    //--

} // game