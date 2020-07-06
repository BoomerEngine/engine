/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "base/script/include/scriptObject.h"

namespace game
{

    //--

    /// The game-global "system" in the game - this usually is something that can span multiple worlds and it truly global
    /// Typical examples: game save state, connection to global game servers, account management, achievements, etc
    class GAME_HOST_API IGameSystem : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IGameSystem, base::script::ScriptedObject);

    public:
        IGameSystem();
        virtual ~IGameSystem();

        //--

        // initialize system
        virtual void handleInitialize(IGame* game);

        // updated before any screen in the game is updated, can be used to push screens/popups
        virtual void handlePreUpdate(IGame* game, double dt);

        // updated after all screens in the game were updated but before anything was rendered
        virtual void handlePostUpdate(IGame* game, double dt);

        // render all DEBUG/temporary content (ie. like connection status, etc), not called in final game
        virtual void handleRenderDebug(IGame* game, rendering::command::CommandWriter& cmd, const HostViewport& viewport);

        // render game global configuration windows (with ImGui) not called in final game
        virtual void handleDebug(IGame* game);

        //--
    };

    //--

} // game
