/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "game/common/include/game.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// a non-interactive game
class GAME_TEMPLATES_API DefaultGame : public IGame
{
    RTTI_DECLARE_VIRTUAL_CLASS(DefaultGame, IGame);

public:
    DefaultGame();

    virtual void handleAttachPlayer(IGamePlayer* player) override;
    virtual void handleDetachPlayer(IGamePlayer* player) override;

    virtual GamePlayerEntityPtr handleCreatePlayerRepresentation(IGamePlayer* player) override;
    virtual void handleRemovePlayerRepresentation(IGamePlayer* player, IGamePlayerEntity* entity) override;
};

//--

END_BOOMER_NAMESPACE()
