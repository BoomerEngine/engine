/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "engine/world/include/worldEntity.h"
#include "engine/rendering/include/stats.h"
#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// The game instance class, subclassed by different game types
class GAME_COMMON_API IGame : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGame, IObject);

public:
    IGame();
    virtual ~IGame();

    //--

    /// the world game will use for it's simulation
    INLINE const WorldPtr& world() const { return m_world; }

    /// all players in the game
    INLINE const Array<GamePlayerPtr>& players() const { return m_players; }

    //--

    // initialize game with given world (NOTE: called usually on the loading thread)
    bool initialize(World* world);

    // update game
    bool update(double dt);

    // render debug GUI
    void renderImGuiDebugInterface();

    //--

    /// attach player to the game
    void attachPlayer(IGamePlayer* player);

    /// detach player from the game
    void dettachPlayer(IGamePlayer* player);

    //-

protected:
    // handle game initialization
    virtual bool handleInitialize();

    // handle internal update
    virtual void handleUpdate(float dt);

    // handle game specific code to create player representation
    virtual void handleAttachPlayer(IGamePlayer* player);

    // handle game specific code to destroy player representation
    virtual void handleDetachPlayer(IGamePlayer* player);

    // create entity representation for a player
    virtual GamePlayerEntityPtr handleCreatePlayerRepresentation(IGamePlayer* player) = 0;

    // remove player representation
    virtual void handleRemovePlayerRepresentation(IGamePlayer* player, IGamePlayerEntity* entity) = 0;

private:
    WorldPtr m_world;
    Array<GamePlayerPtr> m_players;

    void renderDebugPlayers();
};

//--

END_BOOMER_NAMESPACE()
