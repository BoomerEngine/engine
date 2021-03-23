/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "defaultGame.h"
#include "defaultGamePlayer.h"
#include "defaultGameSinglePlayerScreen.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(DefaultSinglePlayerGameScreen);
RTTI_END_TYPE()

DefaultSinglePlayerGameScreen::DefaultSinglePlayerGameScreen()
{}

GamePtr DefaultSinglePlayerGameScreen::createGame(const GameScreenSinglePlayerSetup& gameSetup)
{
    return RefNew<DefaultGame>();
}

GamePlayerLocalPtr DefaultSinglePlayerGameScreen::createLocalPlayer(const GameScreenSinglePlayerSetup& gameSetup)
{
    return RefNew<DefaultPlayer>(gameSetup.localPlayerIdentity, gameSetup.spawnPosition, gameSetup.spawnRotation);
}

//--

END_BOOMER_NAMESPACE()
