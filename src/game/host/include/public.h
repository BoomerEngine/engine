/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "game_host_glue.inl"

namespace game
{
    //--

    class Host;
    
    class IViewportRenderer;

    struct HostViewport;

    class Game;
    typedef base::RefPtr<Game> GamePtr;

    class World;
    typedef base::RefPtr<World> WorldPtr;

    //--

    struct InputAction;

    class InputActionTable;
    typedef base::RefPtr<InputActionTable> InputActionTablePtr;

    class InputContext;
    typedef base::RefPtr<InputContext> InputContextPtr;

    class InputEvent;
    typedef base::RefPtr<InputEvent> InputEventPtr;

    class InputDefinitions;
    typedef base::RefPtr<InputDefinitions> InputDefinitionsPtr;
    typedef base::res::Ref<InputDefinitions> InputDefinitionsRef;

    //--

} // game
