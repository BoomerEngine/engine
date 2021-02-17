/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "base/world/include/worldEntity.h"
#include "base/object/include/object.h"

namespace game
{
    //--

    enum class GameWorldStackOperation : uint8_t
    {
        Nop, // do nothing, allows to load new content without switching to it
        Activate, // push world at the top of the stack and make it active, keep other worlds as it is
        Replace, // remove current world and replace it with new one (also activates it)
        ReplaceAll, // replace all worlds with new one
    };

    /// initialization params for the game stack
    struct GAME_HOST_API GameStartInfo
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(GameStartInfo);

        bool showLoadingScreen = true;

        GameWorldStackOperation op = GameWorldStackOperation::Activate;

        base::StringBuf scenePath;

        base::Vector3 spawnPosition;
        base::Angles spawnRotation;
        bool spawnOverrideEnabled = false;

        GameStartInfo();
    };

    //--

    /// setup for transitioning to another world
    struct GAME_HOST_API GameTransitionInfo
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(GameTransitionInfo);

        bool showLoadingScreen = false;

        GameWorldStackOperation op = GameWorldStackOperation::Activate;

        WorldPtr world;

        GameTransitionInfo();
    };

    //--

    /// rendering viewport
    struct GAME_HOST_API GameViewport
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(GameViewport);

        WorldPtr world;
        base::world::EntityCameraPlacement cameraPlacement;
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
    class GAME_HOST_API Game : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Game, base::IObject);

    public:
        Game();
        virtual ~Game();

        //--

        // get current world
        INLINE const WorldPtr& world() const { return m_currentWorld; }

        // get all attached (ticked) scenes
        INLINE const base::Array<WorldPtr>& worlds() const { return m_worlds; }

        //--

        // update with given engine time delta
        bool processUpdate(double dt, GameLoadingScreenState loadingScreenState);

        // render all required content, all command should be recorded via provided command buffer writer
        bool processRender(GameViewport& outRendering);

        // render to on screen canvas
        void processRenderCanvas(base::canvas::Canvas& canvas);

        // service input message
        bool processInput(const base::input::BaseEvent& evt);

        //--

        // request closing of the game object, usually called from some kind of "Quit to Desktop" option
        void requestClose();

        //--

        /// does current game state require loading screen
        bool requiresLoadingScreen() const;

        /// cancel any pending world loading (also one in progress)
        void cancelPendingWorldTransitions();

        /// switch to already attached world
        /// NOTE: current content may be unloaded, new content may be loaded
        void scheduleWorldTransition(const GameTransitionInfo& info);

        /// switch to new world
        void scheduleNewWorldTransition(const GameStartInfo& info);

        //--

    protected:
        //--

        WorldPtr m_currentWorld;
        base::Array<WorldPtr> m_worlds;

        //--

        base::NativeTimePoint m_currentWorldTransitionStart;
        base::RefPtr<GameWorldTransition> m_currentWorldTransition;
        base::RefPtr<GameWorldTransition> m_pendingWorldTransition;

        void processWorldTransition(GameLoadingScreenState loadingScreenState);
        void applyWorldTransition(const GameWorldTransition& setup);

        //--

        struct LocalStreamingTask : public IReferencable
        {
            base::RefPtr<base::world::StreamingTask> task;
            base::RefWeakPtr<World> world;

            std::atomic<bool> flagFinished = false;
        };

        base::RefPtr<LocalStreamingTask> m_currentWorldStreaming;

        void processWorldStreaming(GameLoadingScreenState loadingScreenState);

        //--

        bool m_requestedClose = false;
    };

    //--

} // game
