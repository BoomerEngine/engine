/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "game.h"
#include "player.h"
#include "playerEntity.h"
#include "playerIdentity.h"

#include "engine/rendering/include/service.h"
#include "engine/rendering/include/params.h"
#include "engine/world/include/world.h"
#include "engine/world/include/compiledWorldData.h"

#ifdef HAS_ENGINE_WORLD_COMPILER
#include "engine/world_compiler/include/worldCompiler.h"
#endif

#include "gpu/device/include/commandWriter.h"

BEGIN_BOOMER_NAMESPACE()

//----

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGame);
RTTI_END_TYPE();

IGame::IGame()
{
}

IGame::~IGame()
{
}

void IGame::attachPlayer(IGamePlayer* player)
{
    DEBUG_CHECK_RETURN(player);
    DEBUG_CHECK_RETURN(player->m_game == nullptr);

    m_players.pushBack(AddRef(player));
    player->m_game = this;

    handleAttachPlayer(player); // allow the game to create representation for the player

    if (auto ent = handleCreatePlayerRepresentation(player))
    {
        player->m_entity = ent;

        m_world->attachEntity(ent);
    }
}

void IGame::dettachPlayer(IGamePlayer* player)
{
    DEBUG_CHECK_RETURN(player);
    DEBUG_CHECK_RETURN(player->m_game == this);
    DEBUG_CHECK_RETURN(m_players.contains(player));

    if (player->m_entity)
    {
        handleRemovePlayerRepresentation(player, player->m_entity);

        m_world->detachEntity(player->m_entity);
        player->m_entity.reset();
    }

    handleDetachPlayer(player);

    player->m_game = nullptr;
    m_players.remove(player);
}

bool IGame::initialize(World* world)
{
    DEBUG_CHECK_RETURN_EX_V(world, "Invalid world", false);

    m_world = AddRef(world);

    if (!handleInitialize())
    {
        m_world.reset();
        return false;
    }

    return true;
}

bool IGame::update(double dt)
{
    m_world->update(dt);
    return true;
}

void IGame::renderImGuiDebugInterface()
{
    m_world->renderDebugGui();
    renderDebugPlayers();
}

//--

static ConfigProperty<bool> cvDebugPagePlayers("DebugPage.Game.Players", "IsVisible", false);

void IGame::renderDebugPlayers()
{
    if (cvDebugPagePlayers.get() && ImGui::Begin("Players"))
    {
        ImGui::Text("Players: %d", m_players.size());
        ImGui::Separator();

        for (const auto& player : m_players)
        {
            const auto name = player->identity()->name();
            if (ImGui::CollapsingHeader(name.c_str()))
            {
                ImGui::Text("Name: '%s'", player->identity()->name().c_str());
                ImGui::Text("ID: '%s'", player->identity()->id().c_str());

                if (player->entity())
                {
                    ImGui::Text("Entity class: '%s'", player->entity()->cls()->name().c_str());
                    ImGui::Text("Entity position: [%f,%f,%f]", player->entity()->cachedWorldTransform().T.x, player->entity()->cachedWorldTransform().T.y, player->entity()->cachedWorldTransform().T.z);
                }
            }
        }
        
        ImGui::End();
    }
}

//--

bool IGame::handleInitialize()
{
    return true;
}

void IGame::handleUpdate(float dt)
{

}

void IGame::handleAttachPlayer(IGamePlayer* player)
{

}

void IGame::handleDetachPlayer(IGamePlayer* player)
{

}

//--

END_BOOMER_NAMESPACE()
