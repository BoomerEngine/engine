/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "gameHost.h"
#include "gameScreen.h"
#include "gameEvent.h"
#include "base/debug/include/debugPageContainer.h"

namespace game
{
    //--

    RTTI_BEGIN_TYPE_ENUM(HostType);
        RTTI_ENUM_OPTION(Standalone);
        RTTI_ENUM_OPTION(InEditor);
        RTTI_ENUM_OPTION(Server);
    RTTI_END_TYPE();

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(Host);
    RTTI_END_TYPE();

    Host::Host(HostType type, const ScreenPtr& initialScreen)
        : m_type(type)
    {
        if (initialScreen)
        {
            m_stack.pushBack(initialScreen);
            initialScreen->handleAttach();
        }

        createEventSuppliers();
    }

    Host::~Host()
    {
        for (int i = m_stack.lastValidIndex(); i >= 0; --i)
            m_stack[i]->handleDetach();
        m_stack.clear();
        m_eventSuppliers.clear();
        m_externalEvents.clear();
    }

    void Host::postExternalEvent(const EventPtr& evt)
    {
        if (evt)
        {
            auto lock = base::CreateLock(m_externalEventLock);
            m_externalEvents.pushBack(evt);
        }
    }

    bool Host::update(double dt)
    { 
        PC_SCOPE_LVL0(GameHostUpdate);

        // build new event stack
        auto newStack = m_stack;

        // process external events
        {
            // pull all events
            base::InplaceArray<EventPtr, 10> allEvents;
            for (const auto& supplier : m_eventSuppliers)
                while (auto evt = supplier->pull())
                    allEvents.pushBack(evt);

            // TODO: sort events
            // TODO: mask events

            // process events - allow each screen to handle it, starting from the most nested ones as they provide the most context
            for (const auto& evt : allEvents)
            {
                for (int level = newStack.lastValidIndex(); level > 0; --level)
                {
                    // process the
                    if (auto transition = m_stack[level]->handleEvent(evt))
                    {
                        TRACE_INFO("Event '{}' serviced by '{}'", *evt, m_stack[level]->cls()->name());
                        resolveTransition(newStack, level, transition);
                        break;
                    }
                }
            }
        }

        // update all screens - back to front, this allows parents to close the whole child stacks
        {
            uint32_t level = 0;
            while (level < newStack.size())
            {
                // update the state, it may result in transition request
                if (auto transition = newStack[level]->handleUpdate(dt))
                {
                    resolveTransition(newStack, level, transition);
                    break;
                }
                
                ++level;
            }
        }

        // apply changes to the state stack
        applyNewStack(newStack);

        // game is considered "active" if there is at least one state on the stack
        return !m_stack.empty();
    }

    void Host::render(rendering::command::CommandWriter& cmd, const HostViewport& viewport)
    {
        int firstStateToDraw = m_stack.lastValidIndex();
        while (firstStateToDraw > 0)
        {
            if (!m_stack[firstStateToDraw]->transparent())
                break;
            firstStateToDraw -= 1;
        }

        for (uint32_t i = firstStateToDraw; i < m_stack.size(); ++i)
        {
            const auto& screen = m_stack[i];
            screen->handleRender(cmd, viewport);
        }
    }

    bool Host::input(const base::input::BaseEvent& evt)
    {
        // process input, front to back
        for (int i = m_stack.lastValidIndex(); i >= 0; --i)
        {
            const auto& screen = m_stack[i];
            if (screen->handleInput(evt))
                return true;
        }

        return false;
    }

    //--

    static void RetireStates(base::Array<ScreenPtr>& stack, uint32_t levelToRetire)
    {
        if (levelToRetire < stack.size())
        {
            auto maxToRetire = stack.size() - levelToRetire;
            stack.erase(levelToRetire, maxToRetire);
        }
    }

    void Host::resolveTransition(base::Array<ScreenPtr>& stack, uint32_t level, const ScreenTransitionRequest& transition) const
    {
        if (transition)
        {
            // retire states
            if (transition.type == ScreenChangeType::Push)
                RetireStates(stack, level + 1); // remove all other overlay screens
            else if (transition.type == ScreenChangeType::Replace)
                RetireStates(stack, level); // remove this level and all other overlay
            else if (transition.type == ScreenChangeType::ReplaceAll || transition.type == ScreenChangeType::Exit)
                RetireStates(stack, 0); // remove everything

            // push new state
            if (transition.screen && transition.type != ScreenChangeType::Exit)
                stack.pushBack(transition.screen);
        }
    }

    void Host::applyNewStack(const base::Array<ScreenPtr>& newStack)
    {
        // detach any screen that are no longer in the new stack
        for (int i = m_stack.lastValidIndex(); i >= 0; --i)
        {
            const auto& screen = m_stack[i];
            if (!newStack.contains(screen))
                screen->handleDetach();
        }

        // attach new screens 
        for (const auto& screen : newStack)
        {
            if (!m_stack.contains(screen))
                screen->handleAttach();
        }

        // swap screen list
        m_stack = newStack;
    }

    //--

    void Host::createEventSuppliers()
    {
        base::InplaceArray<base::SpecificClassType<IEventSupplier>, 10> eventSupplierClasses;
        RTTI::GetInstance().enumClasses(eventSupplierClasses);

        for (const auto cls : eventSupplierClasses)
        {
            if (auto eventSupplier = cls.create())
            {
                if (eventSupplier->initialize(this))
                {
                    TRACE_INFO("Created event supplier '{}'", cls->name());
                    m_eventSuppliers.pushBack(eventSupplier);
                }
            }
        }
    }

    void Host::createDebugPages()
    {
        m_debugPages.create();
    }

    //--
    
} // game
