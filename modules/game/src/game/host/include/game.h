/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once
#include "base/script/include/scriptObject.h"

namespace game
{

    //--

    /// initialization params for the game stack
    struct GAME_HOST_API GameInitData
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(GameInitData);

        base::Vector3 spawnPositionOverride;
        base::Angles spawnRotationOverride;
        bool spawnPositionOverrideEnabled = false;
        bool spawnRotationOverrideEnabled = false;

        base::StringBuf worldPathOverride;
    };

    //--

    /// This class spawns the game from setup data
    /// NOTE: technically there should be only one game factory per project but it can be override with project settings and with commandline
    class GAME_HOST_API IGameLauncher : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IGameLauncher, base::script::ScriptedObject);

    public:
        IGameLauncher();
        virtual ~IGameLauncher();

        //--

        /// create game (with initial screen setup hopefully) for given initialization data
        CAN_YIELD virtual GamePtr createGame(const GameInitData& initData) = 0;

        //--
    };

    //--

    /// The game instance class, usually subclassed by the game type
    class GAME_HOST_API IGame : public base::script::ScriptedObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IGame, base::script::ScriptedObject);

    public:
        IGame();
        virtual ~IGame();

        //--

        // update with given engine time delta
        bool processUpdate(double dt);

        // render all required content, all command should be recorded via provided command buffer writer
        void processRender(rendering::command::CommandWriter& cmd, const HostViewport& viewport);

        // service input message
        bool processInput(const base::input::BaseEvent& evt);

        //--

        // post external even to the game, can happen from anywhere (threadsafe)
        void postExternalEvent(const EventPtr& evt);

        //--

        // request closing of the game object, usually called from some kind of "Quit to Desktop" option
        void requestClose();

        //--

        // early game update - called before any system
        virtual void handleEarlyUpdate(double dt);

        // main game update - called after all systems were ticked but before screens were ticked
        virtual void handleMainUpdate(double dt);

        // last game update - called alter screens were ticked but before anything was rendered for this frame
        virtual void handleLateUpdate(double dt);

        // handle any outstanding input - not processed by game screens, mostly used for debug stuff
        virtual bool handleOutstandingInput(const base::input::BaseEvent& evt);

        // render debug ImGui windows
        virtual void handleDebug();

        //--

        // Request a screen switch
        // Screen will be switched once the old screen allows it and the new screen is ready (both must happen)
        // Screen switch will be done with a specified transition (than can be composite of two)
        void requestScreenSwitch(GameScreenType screenType, IScreen* newScreen, IScreenTransitionEffect* outTransition = nullptr, IScreenTransitionEffect* inTransition = nullptr);

        //--

         /// get scene system
        /// NOTE: requesting system that does not exist if a fatal error, please check scene flags first
        template< typename T >
        INLINE T* system() const
        {
            static auto userIndex = base::reflection::ClassID<T>()->userIndex();
            ASSERT_EX(userIndex != -1, "Trying to access unregistered scene system");
            auto system = (T*)m_systemMap[userIndex];
            ASSERT_EX(!system || system->cls()->is(T::GetStaticClass()), "Invalid system registered");
            return system;
        }

        /// get system pointer
        INLINE IGameSystem* systemPtr(short index) const
        {
            return m_systemMap[index];
        }

        //--

    protected:
        //--

        struct ScreenIncomingState
        {
            ScreenPtr screen;
            ScreenTransitionEffectPtr outTransition;
            ScreenTransitionEffectPtr inTransition;

            INLINE operator bool() const { return screen != nullptr; }

            INLINE void reset()
            {
                screen.reset();
                outTransition.reset();
                inTransition.reset();
            }
        };

        enum class ScreenState : uint8_t
        {
            Showing,
            Normal,
            Hiding,
        };

        struct ScreenStateData
        {
            ScreenPtr current;
            ScreenTransitionEffectPtr currentTransition;
            ScreenState state = ScreenState::Normal;

            //--

            ScreenIncomingState incoming; // currently being processed
            ScreenIncomingState pending; // next in line
        };

        static const uint32_t MAX_SCREENS = (uint32_t)GameScreenType::MAX;
        ScreenStateData m_screens[MAX_SCREENS];

        void updateScreenState(ScreenStateData& data, double dt);

        //--

        base::SpinLock m_externalEventLock;
        base::Array<EventPtr> m_externalEvents;

        //--

        base::Array<GameSystemPtr> m_systems;
        base::Array<IGameSystem*> m_systemMap;

        void createSystems();

        //--

        bool m_requestedClose = false;
    };

    //--

} // game
