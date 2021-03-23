/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "game/common/include/screenSinglePlayerGame.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// default single player "game" when player is a non-interactive flying camera
class GAME_TEMPLATES_API DefaultSinglePlayerGameScreen : public IGameScreenSinglePlayer
{
    RTTI_DECLARE_VIRTUAL_CLASS(DefaultSinglePlayerGameScreen, IGameScreenSinglePlayer);

public:
    DefaultSinglePlayerGameScreen();

    ///---

    virtual GamePtr createGame(const GameScreenSinglePlayerSetup& gameSetup) override;
    virtual GamePlayerLocalPtr createLocalPlayer(const GameScreenSinglePlayerSetup& gameSetup) override;

    //--
};

//--

END_BOOMER_NAMESPACE()
