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
    struct HostViewport;

    class IScreen;
    typedef base::RefPtr<IScreen> ScreenPtr;

    class IScreenTransitionEffect;
    typedef base::RefPtr<IScreenTransitionEffect> ScreenTransitionEffectPtr;

    class IEvent;
    typedef base::RefPtr<IEvent> EventPtr;

    class IEventSupplier;
    typedef base::RefPtr<IEventSupplier> EventSupplierPtr;

    class IGame;
    typedef base::RefPtr<IGame> GamePtr;

    class IGameSystem;
    typedef base::RefPtr<IGameSystem> GameSystemPtr;

    enum class GameScreenType : uint8_t
    {
        Background, // almost 99% this is the game world + loading screen
        Foreground, // almost 99^ those are UI based screens

        MAX,
    };

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
