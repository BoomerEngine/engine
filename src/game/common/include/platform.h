/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "core/object/include/object.h"
#include "core/containers/include/queue.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// Game platform (steam, etc)
class GAME_COMMON_API IGamePlatform : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGamePlatform, IObject);

public:
    IGamePlatform();
    virtual ~IGamePlatform();

    //--

    /// get local platform name ("LocalPC", "Steam", "GOG", etc)
    virtual const StringBuf& name() const = 0;

    /// update platform state
    virtual void update() = 0;

    //---

    /// push platform event to the event queue
    void pushEvent(IGamePlatformEvent* evt);

    /// pop event from the event queue
    GamePlatformEventPtr popEvent();

    //---

    /// get the local player identity
    /// NOTE: this might change when player signs in/out
    /// NOTE: this is never NULL
    virtual GamePlayerIdentityPtr queryLocalPlayer(int playerIndex = 0) = 0;

    //--

private:
    SpinLock m_eventLock;
    Queue<GamePlatformEventPtr> m_events;
};

//--

END_BOOMER_NAMESPACE()
