/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
*
***/

#include "build.h"
#include "world.h"
#include "worldInputSystem.h"
#include "gameInputComponent.h"

namespace game
{

    //---

    RTTI_BEGIN_TYPE_CLASS(WorldInputSystem);
    RTTI_END_TYPE();

    WorldInputSystem::WorldInputSystem()
    {}

    WorldInputSystem::~WorldInputSystem()
    {}

    //--

    bool WorldInputSystem::handleInitialize(World& scene)
    {
        return true;
    }

    void WorldInputSystem::handleShutdown(World& scene)
    {
    }

    void WorldInputSystem::attachInput(InputComponent* component)
    {
        if (component)
            m_inputComponents.pushBackUnique(component);
    }

    void WorldInputSystem::detachInput(InputComponent* component)
    {
        m_inputComponents.remove(component);
    }

    bool WorldInputSystem::handleInput(const base::input::BaseEvent& evt)
    {
        for (const auto& ctx : m_inputComponents)
        {
            if (auto input = ctx.lock())
                if (input->handleInput(evt))
                    break;
        }

        return false;
    }

    //---

} // game

