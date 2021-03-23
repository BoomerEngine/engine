/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "platform.h"
#include "playerIdentity.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// Local player
class GAME_COMMON_API GamePlayerIdentityLocal : public IGamePlayerIdentity
{
    RTTI_DECLARE_VIRTUAL_CLASS(GamePlayerIdentityLocal, IGamePlayerIdentity);

public:
    GamePlayerIdentityLocal();
    virtual ~GamePlayerIdentityLocal();

    virtual const StringBuf& id() const override;
    virtual const StringBuf& name() const override;
    virtual bool local() const override;
    virtual bool signedIn() const override;
    virtual int controllerIndex() const override;

private:
    StringBuf m_id;
    StringBuf m_name;
};

//--

/// Local game platform, no Steam, etc
class GAME_COMMON_API GamePlatformLocal : public IGamePlatform
{
    RTTI_DECLARE_VIRTUAL_CLASS(GamePlatformLocal, IGamePlatform);

public:
    GamePlatformLocal();
    virtual ~GamePlatformLocal();

    //--

    virtual GamePlayerIdentityPtr queryLocalPlayer(int playerIndex = 0) override;

    virtual const StringBuf& name() const override;
    virtual void update() override;

    //--

private:
    RefPtr<GamePlayerIdentityLocal> m_localPlayer;
    StringBuf m_name;
};

//--

END_BOOMER_NAMESPACE()
