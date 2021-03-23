/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "platformLocal.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(GamePlayerIdentityLocal);
RTTI_END_TYPE();

GamePlayerIdentityLocal::GamePlayerIdentityLocal()
{
    m_name = GetUserName();
    if (!m_name)
        m_name = "player";

    StringView host = GetHostName();
    if (!host)
        host = "local";

    m_id = TempString("{}-{}", host, m_name);
}

GamePlayerIdentityLocal::~GamePlayerIdentityLocal()
{
}

const StringBuf& GamePlayerIdentityLocal::id() const
{
    return m_id;
}

const StringBuf& GamePlayerIdentityLocal::name() const
{
    return m_name;
}

bool GamePlayerIdentityLocal::local() const
{
    return true;
}

bool GamePlayerIdentityLocal::signedIn() const
{
    return true;
}

int GamePlayerIdentityLocal::controllerIndex() const
{
    return 0;
}

//--

RTTI_BEGIN_TYPE_CLASS(GamePlatformLocal);
RTTI_END_TYPE();

GamePlatformLocal::GamePlatformLocal()
    : m_name("local")
{
    if (!IsDefaultObjectCreation())
        m_localPlayer = RefNew<GamePlayerIdentityLocal>();
}

GamePlatformLocal::~GamePlatformLocal()
{}

const StringBuf& GamePlatformLocal::name() const
{
    return m_name;
}

GamePlayerIdentityPtr GamePlatformLocal::queryLocalPlayer(int playerIndex)
{
    if (playerIndex == 0)
        return m_localPlayer;
    return nullptr;
}

void GamePlatformLocal::update()
{
    // nothing
}

//--
    
END_BOOMER_NAMESPACE()