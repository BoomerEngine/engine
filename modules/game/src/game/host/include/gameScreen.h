/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

namespace game
{
    //--

    /// How should the stack change
    enum class ScreenChangeType : uint8_t
    {
        // keep existing screen as it
        Keep,

        // push a new screen on top of current stack item - popup menu within current screen
        Push,

        // replace current element with new element (ie, next menu)
        Replace,

        // replace the whole stack with new state
        ReplaceAll,

        // exit the game
        Exit,
    };

    /// A request to transition to different screen
    struct GAME_HOST_API ScreenTransitionRequest
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(ScreenTransitionRequest);

        ScreenChangeType type; // type of change
        ScreenPtr screen; // screen

        ScreenTransitionRequest(); // keep
        ScreenTransitionRequest(const ScreenTransitionRequest& other) = default;
        ScreenTransitionRequest& operator=(const ScreenTransitionRequest& other) = default;
        ScreenTransitionRequest(ScreenChangeType type, const ScreenPtr& screen);

        static ScreenTransitionRequest ReplaceAll(const ScreenPtr& screen);
        static ScreenTransitionRequest Replace(const ScreenPtr& screen);
        static ScreenTransitionRequest Push(const ScreenPtr& screen);

        inline operator bool() const { return screen && type != ScreenChangeType::Keep; }
    };

    //--

    /// fade in/out helper
    struct GAME_HOST_API ScreenFadeHelper
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(ScreenFadeHelper);

        float m_fraction = 1.0f;
        float m_speed = 0.0f;

        // start a fade in toward 1.0 that should take given amount of time (0=instant on next update)
        void startFadeIn(float time);

        // start a fade out toward 0.0 that should take given amount of time (0=instant on next update)
        void startFadeOut(float time);

        // true if we are currently fading (non zero speed)
        bool fading();

        // update the fade, returns true when the fade has finished (we reched 0 or 1, but only if it was in progress)
        bool update(float dt);
    };

    //--

    /// A screen in the game stack machine
    class GAME_HOST_API IScreen : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IScreen, base::IObject);

    public:
        IScreen();
        virtual ~IScreen();

        //-- 

        // does this screen has exclusive keyboard input ? if so all screens below this one got automatically deprived of it
        INLINE bool exclusiveKeyboardInput() const { return m_exclusiveKeyboardInput; }

        // does this screen has exclusive mouse input ? if so all screens below this one got automatically deprived of it
        INLINE bool exclusiveMouseInput() const { return m_exclusiveMouseInput; }

        // does this screen has exclusive gamepad input ? if so all screens below this one got automatically deprived of it
        INLINE bool exclusivePadInput() const { return m_exclusivePadInput; }

        // is this a transparent screen ?
        INLINE bool transparent() const { return m_transparent; }

        //--

        // check if screen is ready for switching to, usually used in screens that require setup
        virtual bool handleReadyCheck();

        // update with given engine time delta
        virtual ScreenTransitionRequest handleUpdate(double dt);

        // allow this state to process an external game event, this can trigger instant state transition
        virtual ScreenTransitionRequest handleEvent(const EventPtr& evt);

        // render to given rendering output
        virtual void handleRender(rendering::command::CommandWriter& cmd, const HostViewport& viewport);

        // service input message, TEMPSHIT, will be changed to game actions
        virtual bool handleInput(const base::input::BaseEvent& evt);

        // called when screen if added to the stack
        virtual void handleAttach();

        // called when screen is removed from the stack
        virtual void handleDetach();

        //--

        // cancel any pending transition and fade the screen back in
        void cancelTransition();

        // start fading out this screen and when you are done do a transition
        // NOTE: as a bonus we will start the transition only once the screen in question becomes ready
        void startTransition(const ScreenTransitionRequest& request);

        //--

    protected:
        bool m_exclusiveKeyboardInput = false;
        bool m_exclusiveMouseInput = false;
        bool m_exclusivePadInput = false; // this includes other game controllers as well
        bool m_transparent = true; // panel does not prevent the bottom one from rendering
        
        ScreenFadeHelper m_fade; // fade in/out helper
        float m_fadeInTime = 0.3f;
        float m_fadeOutTime = 0.3f;

        ScreenTransitionRequest m_pendingTransition;
        ScreenTransitionRequest m_currentTransition;
    };

    //--

} // game
