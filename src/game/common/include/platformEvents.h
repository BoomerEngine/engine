/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// Game platform event
class GAME_COMMON_API IGamePlatformEvent : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGamePlatformEvent, IObject);

public:
    IGamePlatformEvent(StringID name);
    virtual ~IGamePlatformEvent();

    INLINE StringID name() const { return m_name; }
    INLINE NativeTimePoint timestamp() const { return m_timestamp; }
    
    virtual GamePlayerIdentityPtr player() const = 0;

private:
    StringID m_name;
    NativeTimePoint m_timestamp;
};

//--

/// Game platform event that affects a player
class GAME_COMMON_API IGamePlatformEventWithPlayer : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGamePlatformEventWithPlayer, IObject);

public:
    IGamePlatformEventWithPlayer(IGamePlayerIdentity* id);

    virtual GamePlayerIdentityPtr player() const override final { return m_player; }

private:
    GamePlayerIdentityPtr m_player;
};


//--

/// Player got signed in
class GAME_COMMON_API GamePlatformEventPlayerSignedIn : public IGamePlatformEventWithPlayer
{
    RTTI_DECLARE_VIRTUAL_CLASS(GamePlatformEventPlayerSignedIn, IGamePlatformEventWithPlayer);

public:
};

//--

/// Player got signed out
class GAME_COMMON_API GamePlatformEventPlayerSignedOut : public IGamePlatformEventWithPlayer
{
    RTTI_DECLARE_VIRTUAL_CLASS(GamePlatformEventPlayerSignedOut, IGamePlatformEventWithPlayer);

public:
};

//--

END_BOOMER_NAMESPACE()
