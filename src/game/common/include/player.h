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

/// A player in the game
class GAME_COMMON_API IGamePlayer : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGamePlayer, IObject);

public:
    IGamePlayer(IGamePlayerIdentity* identity, IGamePlayerConnection* connection);
    virtual ~IGamePlayer();

    //-- 

    /// game we part of, valid only when attached
    INLINE IGame* game() const { return m_game; }

    /// player identity
    INLINE const GamePlayerIdentityPtr& identity() const { return m_identity; }

    /// player connection, only valid in multiplayer game
    INLINE const GamePlayerConnectionPtr& connection() const { return m_connection; }

    /// entity controlled by this player, valid only for players that are in game (not the ones just connected)
    INLINE const GamePlayerEntityPtr& entity() const { return m_entity; }

    //--

private:
    IGame* m_game = nullptr;

    GamePlayerIdentityPtr m_identity;
    GamePlayerConnectionPtr m_connection;
    GamePlayerEntityPtr m_entity;

    friend class IGame;
};

//--

/// Local player that has input and rendering on this machine
class GAME_COMMON_API IGamePlayerLocal : public IGamePlayer
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGamePlayerLocal, IObject);

public:
    IGamePlayerLocal(IGamePlayerIdentity* identity, IGamePlayerConnection* connection);
    virtual ~IGamePlayerLocal();

    /// process player input
    virtual bool handleInput(IGameScreen* screen, const InputEvent& evt);

    /// render game world
    virtual void handleRender(IGameScreen* screen, gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, float visibility);

private:
    rendering::CameraContextPtr m_cameraContext;
};

//--

END_BOOMER_NAMESPACE()

