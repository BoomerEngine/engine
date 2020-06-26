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

namespace game
{
    //---

    base::ConfigProperty<float> cvGameScreenDefaultFadeInTime("Game.Screen", "FadeInSpeed", 0.3f);
    base::ConfigProperty<float> cvGameScreenDefaultFadeOutTime("Game.Screen", "FadeOutSpeed", 0.3f);

    //---

    RTTI_BEGIN_TYPE_ENUM(ScreenChangeType);
        RTTI_ENUM_OPTION(Keep);
        RTTI_ENUM_OPTION(Push);
        RTTI_ENUM_OPTION(Replace);
        RTTI_ENUM_OPTION(ReplaceAll);
        RTTI_ENUM_OPTION(Exit);
    RTTI_END_TYPE();

    //---

    RTTI_BEGIN_TYPE_CLASS(ScreenTransitionRequest);
        RTTI_PROPERTY(type);
        RTTI_PROPERTY(screen);
    RTTI_END_TYPE();

    ScreenTransitionRequest::ScreenTransitionRequest()
        : type(ScreenChangeType::Keep)
    {}

    ScreenTransitionRequest::ScreenTransitionRequest(ScreenChangeType type_, const ScreenPtr & screen_)
        : type(type_)
        , screen(screen_)
    {}

    ScreenTransitionRequest ScreenTransitionRequest::ReplaceAll(const ScreenPtr& screen)
    {
        return ScreenTransitionRequest(ScreenChangeType::ReplaceAll, screen);
    }

    ScreenTransitionRequest ScreenTransitionRequest::Replace(const ScreenPtr& screen)
    {
        return ScreenTransitionRequest(ScreenChangeType::Replace, screen);
    }

    ScreenTransitionRequest ScreenTransitionRequest::Push(const ScreenPtr& screen)
    {
        return ScreenTransitionRequest(ScreenChangeType::Push, screen);
    }

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IScreen);
    RTTI_END_TYPE();

    IScreen::IScreen()
    {
        m_fadeInTime = cvGameScreenDefaultFadeInTime.get();
        m_fadeOutTime = cvGameScreenDefaultFadeOutTime.get();
    }

    IScreen::~IScreen()
    {}


    bool IScreen::handleReadyCheck()
    {
        return true;
    }

    ScreenTransitionRequest IScreen::handleUpdate(double dt)
    {
        if (m_pendingTransition && m_pendingTransition.screen->handleReadyCheck())
        {
            m_currentTransition = std::move(m_pendingTransition);
            m_pendingTransition = ScreenTransitionRequest();
            m_fade.startFadeOut(m_fadeOutTime);
        }

        bool fadeOut = m_fade.m_speed < 0.0f;
        if (m_fade.update(dt))
        {
            if (fadeOut && m_currentTransition)
            {
                auto transition = std::move(m_currentTransition);
                m_currentTransition = ScreenTransitionRequest();
                return transition;
            }
        }

        return ScreenTransitionRequest(); // keep existing screen
    }

    ScreenTransitionRequest IScreen::handleEvent(const EventPtr& evt)
    {
        return ScreenTransitionRequest(); // keep existing screen
    }

    void IScreen::handleRender(rendering::command::CommandWriter& cmd, const HostViewport& viewport)
    {}

    bool IScreen::handleInput(const base::input::BaseEvent& evt)
    {
        return false;
    }

    void IScreen::handleAttach()
    {
        m_fade.m_fraction = 0.0f;
        m_fade.startFadeIn(m_fadeInTime);
    }

    void IScreen::handleDetach()
    {}

    //---

    void IScreen::cancelTransition()
    {
        if (m_pendingTransition)
        {
            TRACE_INFO("Pending transition canceled");
            m_currentTransition = ScreenTransitionRequest();
            m_pendingTransition = ScreenTransitionRequest();
            m_fade.startFadeIn(m_fadeInTime);
        }
    }

    void IScreen::startTransition(const ScreenTransitionRequest& request)
    {
        DEBUG_CHECK_EX(request, "No valid request specified");
        DEBUG_CHECK_EX(!m_pendingTransition, "Already have pending transition, that's bad for flow");

        if (request)
        {
            if (request.screen->handleReadyCheck())
            {
                m_currentTransition = request;
                m_fade.startFadeOut(m_fadeInTime);
            }
            else
            {
                m_pendingTransition = request;
            }
        }
    }
    
    //--

    RTTI_BEGIN_TYPE_CLASS(ScreenFadeHelper);
    RTTI_PROPERTY(m_fraction);
    RTTI_PROPERTY(m_speed);
    RTTI_END_TYPE();

    static float CalcSpeed(float time)
    {
        return (time > 0.0001) ? (1.0 / time) : 10000.0f; // limit
    }

    void ScreenFadeHelper::startFadeIn(float time)
    {
        if (m_fraction < 1.0f)
            m_speed = CalcSpeed(time);
    }

    void ScreenFadeHelper::startFadeOut(float time)
    {
        if (m_fraction > 0.0f)
            m_speed = -CalcSpeed(time);
    }

    bool ScreenFadeHelper::fading()
    {
        return m_speed != 0.0f;
    }

    bool ScreenFadeHelper::update(float dt)
    {
        if (m_speed > 0.0f)
        {
            m_fraction += m_speed * dt;
            if (m_fraction >= 1.0f)
            {
                m_fraction = 1.0f;
                m_speed = 0.0f;
                return true;
            }
        }
        else if (m_speed < 0.0f)
        {
            m_fraction += m_speed * dt;
            if (m_fraction <= 0.0f)
            {
                m_fraction = 0.0f;
                m_speed = 0.0f;
                return true;
            }
        }

        return false;
    }

    //--

} // game


