/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\systems #]
***/

#pragma once

#include "worldSystem.h"

namespace game
{
    //---

    /// input system
    class GAME_WORLD_API WorldInputSystem : public IWorldSystem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(WorldInputSystem, IWorldSystem);

    public:
        WorldInputSystem();
        virtual ~WorldInputSystem();

        //--

        // attach input component to the input queue
        void attachInput(InputComponent* component);

        // detach input componet from the input queue
        void detachInput(InputComponent* component);

        //--

        // process input event
        bool handleInput(const base::input::BaseEvent& evt);

        //--

    protected:
        // IWorldSystem
        virtual bool handleInitialize(World& scene) override;
        virtual void handleShutdown(World& scene) override;

        base::Array<base::RefWeakPtr<InputComponent>> m_inputComponents;
    };

    //---

} // game