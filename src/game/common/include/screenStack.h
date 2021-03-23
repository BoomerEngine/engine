/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "engine/rendering/include/stats.h"
#include "core/containers/include/queue.h"

BEGIN_BOOMER_NAMESPACE()

//--

enum class GameTransitionMode : uint8_t
{
    // push new screen on top of the stack, plays "fade-in" animation
    PushOnTop,

    // replace current top screen with a new one, fades out the first and fades in the second
    ReplaceTop,

    // replace whole stack with new screen
    ReplaceAll,
};

//--

/// Container class for master game project, worlds, scenes and everything else
/// Can be instanced as a standalone app/project or in editor
/// This class also provides the highest (global) level of scripting in the game - allows to write stuff like loading screens, settings panel, world selection etc
/// This class is NOT intended to be derived from, internal flow is managed with "game screens"
class GAME_COMMON_API GameScreenStack : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(GameScreenStack, IObject);

public:
    GameScreenStack(IGamePlatform* platform);
    virtual ~GameScreenStack();

    //--

    // get the game platform that allows us to communicate with outside world
    INLINE IGamePlatform* platform() const { return m_platform; }

    // do we have pending transitions ?
    INLINE bool hasPendingTransition() const { return !m_pendingFadeInScreens.empty() || !m_pendingFadeOutScreens.empty() || !m_pendingTransition.empty(); }

    // transition to a new screen
    // NOTE: it's legal to transition to null screen, especially in the ReplaceAll and ReplaceTop modes
    // NOTE: being left with no screens on the stack exits the host (and usually the application)
    void pushTransition(IGameScreen* screen, GameTransitionMode mode, bool instant = false);

    //--

    // update with given engine time delta, returns false if there are no screens to update (game finished)
    bool update(double dt);

    // render all required content, all command should be recorded via provided command buffer writer
    void render(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output);

    // service input message
    bool input(const input::BaseEvent& evt);

    // should the host have exclusive access to user input?
    bool shouldCaptureInput() const;

    //--

    // request the whole game to close (to desktop usually)
    void requestExit();

    //--

private:
    IGamePlatform* m_platform = nullptr;

    //--

    ImGui::ImGUICanvasHelper* m_imgui = nullptr;
    bool m_imguiOverlayInputPassThrough = false;

    //--

    bool m_exitRequested = false;

    //--

    struct Screen
    {
        GameScreenPtr screen;
        float fadeVisibility = 0.0f;
        float fadeSpeed = 0.0f;
    };

    Array<Screen> m_screens;

    void attachScreen(IGameScreen* controller, float fadeInTime);
    void detachScreen(IGameScreen* controller);

    //--

    struct PendingScreen
    {
        GameScreenPtr screen;
        bool instant = false;
    };

    Queue<PendingScreen> m_pendingFadeOutScreens;
    Queue<PendingScreen> m_pendingFadeInScreens;

    struct PendingTransition
    {
        GameScreenPtr screen;
        GameTransitionMode mode;
        bool instant = false;
    };

    Queue<PendingTransition> m_pendingTransition;

    void updateTransitions(float dt);

    bool hasTransitionInProgress() const;
    bool updateCurrentScreenFades(float dt);
    bool updatePendingFadeOut();
    bool updatePendingFadeIn();
    bool updatePendingTransition();

    //--

    void renderImGuiDebugOverlay(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output);
    bool processDebugInput(const input::BaseEvent& evt);
};

//--

END_BOOMER_NAMESPACE()
