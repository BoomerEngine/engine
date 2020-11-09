/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: screens #]
***/

#pragma once

#include "gameScreen.h"

namespace game
{
    //--

    // screen that has a world that can run scene simulation
    class Screen_World : public IScreen
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Screen_World, IScreen);

    public:
        Screen_World();
        virtual ~Screen_World();

        // entity we use to view the world
        INLINE const base::world::EntityPtr& viewEntity() const { return m_viewEntity; }

        // entity we use to control the gameplay (player)
        INLINE const base::world::EntityPtr& controlEntity() const { return m_controlEntity; }

        //--

        // bind new view entity, it will be used to render stuff (if it has a camera)
        void viewEntity(base::world::Entity* entity);

        // bind nerw control entity
        void controlEntity(base::world::Entity* entity);

        //--

    protected:
        virtual bool supportsNativeFadeInFadeOut() const override;
        virtual void handleUpdate(IGame* game, double dt) override;
        virtual void handleRender(IGame* game, rendering::command::CommandWriter& cmd, const HostViewport& viewport) override;
        virtual bool handleInput(IGame* game, const base::input::BaseEvent& evt) override;

    private:
        base::world::WorldPtr m_world;
        base::world::EntityPtr m_viewEntity;
        base::world::EntityPtr m_controlEntity;
    };
    
    //--

} // game
