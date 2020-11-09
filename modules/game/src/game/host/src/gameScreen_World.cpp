/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: screens #]
***/

#include "build.h"
#include "gameHost.h"
#include "gameScreen_World.h"
#include "base/world/include/world.h"

namespace game
{
    //---

    RTTI_BEGIN_TYPE_CLASS(Screen_World);
    RTTI_END_TYPE();

    Screen_World::Screen_World()
    {
        m_world = base::CreateSharedPtr<base::world::World>();

        // TODO: inject interop system so the world knows about the game ?
        //m_world
    }

    Screen_World::~Screen_World()
    {}

    bool Screen_World::supportsNativeFadeInFadeOut() const
    {
        return true;
    }

    void Screen_World::handleUpdate(IGame* game, double dt)
    {
        return TBaseClass::handleUpdate(game, dt);
    }

    bool Screen_World::handleInput(IGame* game, const base::input::BaseEvent& evt)
    {
        // TODO: pass input to control entity

        return TBaseClass::handleInput(game, evt);
    }

    void Screen_World::handleRender(IGame* game, rendering::command::CommandWriter& cmd, const HostViewport& hostViewport)
    {
        // TODO: render world from the perspective of the view entity
    }
    
    //--

    void Screen_World::viewEntity(base::world::Entity* entity)
    {
        m_viewEntity = AddRef(entity);
    }

    void Screen_World::controlEntity(base::world::Entity* entity)
    {
        m_controlEntity = AddRef(entity);
    }

    //--

} // game


