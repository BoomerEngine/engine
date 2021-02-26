/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "engine/world/include/worldEntity.h"
#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

//--

enum class GameGameWorldStackOperation : uint8_t
{
    Nop, // do nothing, allows to load new content without switching to it
    Activate, // push world at the top of the stack and make it active, keep other worlds as it is
    Replace, // remove current world and replace it with new one (also activates it)
    ReplaceAll, // replace all worlds with new one
};

/// initialization params for the game stack
struct ENGINE_GAME_API GameStartInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(GameStartInfo);

    bool showLoadingScreen = true;

    GameGameWorldStackOperation op = GameGameWorldStackOperation::Activate;

    StringBuf scenePath;

    Vector3 spawnPosition;
    Angles spawnRotation;
    bool spawnOverrideEnabled = false;

    GameStartInfo();
};

//--

/// setup for transitioning to another world
struct ENGINE_GAME_API GameTransitionInfo
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(GameTransitionInfo);

    bool showLoadingScreen = false;

    GameGameWorldStackOperation op = GameGameWorldStackOperation::Activate;

    GameWorldPtr world;

    GameTransitionInfo();
};

//--

/// rendering viewport
struct ENGINE_GAME_API GameViewport
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(GameViewport);

    GameWorldPtr world;
    EntityCameraPlacement cameraPlacement;
};

//--

/// loading screen state
enum class GameLoadingScreenState : uint8_t
{
    Hidden,
    Transition,
    Visible,
};

//--

/// The game instance class
class ENGINE_GAME_API Game : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(Game, IObject);

public:
    Game();
    virtual ~Game();

    //--

    // get current world
    INLINE const GameWorldPtr& world() const { return m_currentGameWorld; }

    // get all attached (ticked) scenes
    INLINE const Array<GameWorldPtr>& worlds() const { return m_worlds; }

    //--

    // update with given engine time delta
    bool processUpdate(double dt, GameLoadingScreenState loadingScreenState);

    // render all required content, all command should be recorded via provided command buffer writer
    bool processRender(GameViewport& outRendering);

    // render to on screen canvas
    void processRenderCanvas(canvas::Canvas& canvas);

    // service input message
    bool processInput(const input::BaseEvent& evt);

    //--

    // request closing of the game object, usually called from some kind of "Quit to Desktop" option
    void requestClose();

    //--

    /// does current game state require loading screen
    bool requiresLoadingScreen() const;

    /// cancel any pending world loading (also one in progress)
    void cancelPendingGameWorldTransitions();

    /// switch to already attached world
    /// NOTE: current content may be unloaded, new content may be loaded
    void scheduleGameWorldTransition(const GameTransitionInfo& info);

    /// switch to new world
    void scheduleNewGameWorldTransition(const GameStartInfo& info);

    //--

protected:
    //--

    GameWorldPtr m_currentGameWorld;
    Array<GameWorldPtr> m_worlds;

    //--

    NativeTimePoint m_currentGameWorldTransitionStart;
    RefPtr<GameGameWorldTransition> m_currentGameWorldTransition;
    RefPtr<GameGameWorldTransition> m_pendingGameWorldTransition;

    void processGameWorldTransition(GameLoadingScreenState loadingScreenState);
    void applyGameWorldTransition(const GameGameWorldTransition& setup);

    //--

    struct LocalStreamingTask : public IReferencable
    {
        RefPtr<StreamingTask> task;
        RefWeakPtr<GameWorld> world;

        std::atomic<bool> flagFinished = false;
    };

    RefPtr<LocalStreamingTask> m_currentGameWorldStreaming;

    void processGameWorldStreaming(GameLoadingScreenState loadingScreenState);

    //--

    bool m_requestedClose = false;
};

//--

END_BOOMER_NAMESPACE()
