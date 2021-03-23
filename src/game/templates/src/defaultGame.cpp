/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "defaultGame.h"
#include "defaultGamePlayerEntity.h"
#include "defaultGamePlayer.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(DefaultGame);
RTTI_END_TYPE();

DefaultGame::DefaultGame()
{}

void DefaultGame::handleAttachPlayer(IGamePlayer* player)
{
    TBaseClass::handleAttachPlayer(player);
}

void DefaultGame::handleDetachPlayer(IGamePlayer* player)
{
    TBaseClass::handleDetachPlayer(player);
}

GamePlayerEntityPtr DefaultGame::handleCreatePlayerRepresentation(IGamePlayer* player)
{
    auto defaultPlayer = rtti_cast<DefaultPlayer>(player);
    DEBUG_CHECK_RETURN_EX_V(defaultPlayer, "Invalid player type", nullptr);

    auto playerEntity = RefNew<DefaultPlayerEntity>();
    playerEntity->requestTransformChange(defaultPlayer->m_spawnPosition, defaultPlayer->m_spawnRotation);

    return playerEntity;
}

void DefaultGame::handleRemovePlayerRepresentation(IGamePlayer* player, IGamePlayerEntity* entity)
{
    // default implementation is ok
}

//--

END_BOOMER_NAMESPACE()
