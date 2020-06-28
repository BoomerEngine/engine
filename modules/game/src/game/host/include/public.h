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

    class IEvent;
    typedef base::RefPtr<IEvent> EventPtr;

    class IEventSupplier;
    typedef base::RefPtr<IEventSupplier> EventSupplierPtr;

    struct ScreenTransitionRequest;

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
