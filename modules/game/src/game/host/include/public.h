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

} // game
