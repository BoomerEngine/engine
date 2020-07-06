/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "game.h"
#include "gameScreen.h"
#include "gameSystem.h"

namespace game
{
    //----

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGame);
    RTTI_END_TYPE();

    IGame::IGame()
    {
        createSystems();
    }

    IGame::~IGame()
    {}

    bool IGame::processUpdate(double dt)
    {
        // pre-screen update
        handleEarlyUpdate(dt);

        // update screens - fade in/fade out
        for (auto& screen : m_screens)
            updateScreenState(screen, dt);

        // tick active screen
        for (auto& screen : m_screens)
            if (screen.current)
                screen.current->handleUpdate(this, dt);

        // post-screen update
        handleMainUpdate(dt);

        // we can only run if we have at least one active screen
        bool hasScreens = false;
        for (auto& screen : m_screens)
            hasScreens |= (screen.current != nullptr);
        return hasScreens;
    }

    void IGame::processRender(rendering::command::CommandWriter& cmd, const HostViewport& viewport)
    {
        // render screens
        for (auto& screen : m_screens)
        {
            if (screen.current)
            {
                // TODO: render using transitions when active
                screen.current->handleRender(this, cmd, viewport);
            }
        }
    }

    bool IGame::processInput(const base::input::BaseEvent& evt)
    {
        // pass to screens in reverse order
        for (int i = MAX_SCREENS - 1; i >= 0; --i)
        {
            auto& screen = m_screens[i];

            if (screen.current)
            {
                if (screen.current->handleInput(this, evt))
                    return true;
            }
        }

        // handle unprocessed input locally
        if (handleOutstandingInput(evt))
            return true;

        // input was not processed
        return false;
    }

    void IGame::requestClose()
    {
        if (!m_requestedClose)
        {
            TRACE_INFO("Game requested close");

            // TODO: start a fade on each screen

            for (auto& screen : m_screens)
            {
                screen.incoming = ScreenIncomingState();
                screen.current = nullptr;
            }

            m_requestedClose = true;
        }
    }

    //---

    void IGame::handleEarlyUpdate(double dt)
    {
        // nothing here
    }

    void IGame::handleMainUpdate(double dt)
    {
        // nothing here
    }

    void IGame::handleLateUpdate(double dt)
    {
        // nothing here
    }

    bool IGame::handleOutstandingInput(const base::input::BaseEvent& evt)
    {
        return false;
    }

    void IGame::handleDebug()
    {
        for (const auto& system : m_systems)
            system->handleDebug(this);

        for (const auto& screen : m_screens)
            if (screen.current)
                screen.current->handleDebug();
    }

    //---

    void IGame::postExternalEvent(const EventPtr& evt)
    {
        if (evt)
        {
            auto lock = base::CreateLock(m_externalEventLock);
            m_externalEvents.pushBack(evt);
        }
    }

    //---

    void IGame::requestScreenSwitch(GameScreenType type, IScreen* newScreen, IScreenTransitionEffect* outTransition /*= nullptr*/, IScreenTransitionEffect* inTransition /*= nullptr*/)
    {
        auto& entry = m_screens[(int)type];

        // can't switch screens while existing
        if (m_requestedClose)
            return;

        // setup the incoming screen (will replace any existing one that has not yet started)
        entry.pending.screen = AddRef(newScreen);

        // select the out transition
        if (inTransition)
            entry.pending.inTransition = AddRef(inTransition);
        else if (newScreen)
            entry.pending.inTransition = newScreen->createDefaultShowTransition(this, entry.current);
        else
            entry.pending.inTransition = nullptr; // just pop

        // select the "out" transition
        if (outTransition)
            entry.pending.outTransition = AddRef(outTransition);
        else if (entry.current)
            entry.pending.outTransition = entry.current->createDefaultHideTransition(this, newScreen);
        else
            entry.pending.outTransition = nullptr; // just pop

        // perform state update right away (this will handle cases when we have a "pop" transition)
        updateScreenState(entry, 0.0);
    }

    //---

    void IGame::updateScreenState(ScreenStateData& data, double dt)
    {
        // if there's nothing already blending get it from the pending
        if (!data.incoming)
        {
            data.incoming = data.pending;
            data.pending.reset();
        }

        // process the internal state machine
        if (data.state == ScreenState::Normal)
        {
            // if we have incoming transition then we need to switch
            if (data.incoming)
            {
                // start transition to next screen
                DEBUG_CHECK(!data.currentTransition);
                data.currentTransition = data.incoming.outTransition;
                data.state = ScreenState::Hiding;
            }
        }

        // handle hiding of previous screen
        if (data.state == ScreenState::Hiding)
        {
            if (data.currentTransition)
                data.currentTransition->handleUpdate(dt);

            if (!data.currentTransition || data.currentTransition->finished())
            {
                data.currentTransition.reset();

                data.state = ScreenState::Showing;
                data.current = data.incoming.screen;
                data.currentTransition = data.incoming.inTransition;

                data.incoming = ScreenIncomingState();
            }
        }

        // handle showing of the screen
        if (data.state == ScreenState::Showing)
        {
            if (data.currentTransition)
            {
                data.currentTransition->handleUpdate(dt);

                if (data.currentTransition->finished())
                    data.currentTransition.reset();
            }

            if (!data.currentTransition)
                data.state = ScreenState::Normal;
        }
    }

    //---

    void IGame::createSystems()
    {
        base::InplaceArray<base::SpecificClassType<IGameSystem>, 20> systemClasses;
        RTTI::GetInstance().enumClasses(systemClasses);
        TRACE_INFO("Found {} game sytem classes", systemClasses.size());

        // initialize the services
        for (auto cls : systemClasses)
        {
            TRACE_SPAM("Initializing game system '{}'", cls->name());

            // create service
            auto systemPtr = cls.create();
            systemPtr->handleInitialize(this);
            m_systems.pushBack(systemPtr);

            // map
            cls->assignUserIndex(m_systemMap.size());
            m_systemMap.pushBack(systemPtr);
        }
    }

    //---

} // ui
