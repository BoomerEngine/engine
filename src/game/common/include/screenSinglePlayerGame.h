/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "screen.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// single player initialization data
struct GameScreenSinglePlayerSetup
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(GameScreenSinglePlayerSetup);

public:
    StringBuf worldPath;
    CompiledWorldDataPtr worldData = nullptr;

    GamePlayerIdentityPtr localPlayerIdentity;

    bool overrideSpawnPosition = false;
    Vector3 spawnPosition;
    Angles spawnRotation;
};

//--

struct GameScreenSinglePlayerSetupLoadingState;

/// basic screen for local single player game
class GAME_COMMON_API IGameScreenSinglePlayer : public IGameScreen
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGameScreenSinglePlayer, IGameScreen);

public:
    IGameScreenSinglePlayer();
    virtual ~IGameScreenSinglePlayer();

    ///---

    INLINE const GamePtr& game() const { return m_game; }

    INLINE const GamePlayerLocalPtr& player() const { return m_player; }

    ///---

    // initialize the screen
    bool initialize(const GameScreenSinglePlayerSetup& gameSetup);

    ///---

protected:
    //--

    virtual bool queryOpaqueState() const override;
    virtual bool queryFilterAllInput() const override;
    virtual bool queryInputCaptureState() const override;

    virtual void handleDetached() override;
    virtual void handleUpdate(double dt) override;
    virtual bool handleInput(const input::BaseEvent& evt) override;
    virtual void handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, float visibility) override;
    virtual void handleRenderImGuiDebugOverlay() override;

    //--

    virtual GamePtr createGame(const GameScreenSinglePlayerSetup& gameSetup) = 0;

    virtual GamePlayerLocalPtr createLocalPlayer(const GameScreenSinglePlayerSetup& gameSetup) = 0;

    //--

private:
    //--

    GamePtr m_game; // game being played
    GamePlayerLocalPtr m_player; // the local, only player

    //--

    RefPtr<GameScreenSinglePlayerSetupLoadingState> m_loadingState;

    void updateLoading();

    //--
};

//--

END_BOOMER_NAMESPACE()
