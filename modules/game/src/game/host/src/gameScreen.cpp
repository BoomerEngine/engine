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

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IScreen);
    RTTI_END_TYPE();

    IScreen::IScreen()
    {
    }

    IScreen::~IScreen()
    {}

    bool IScreen::supportsNativeFadeInFadeOut() const
    {
        return false;
    }

    bool IScreen::readyToHide(IGame* game)
    {
        return true; // no additional requirements
    }

    bool IScreen::readyToShow(IGame* game)
    {
        return true; // no additional requirements
    }

    void IScreen::handleUpdate(IGame* game, double dt)
    {
        // nothing in default implementation
    }

    void IScreen::handleEvent(IGame* game, const EventPtr& evt)
    {   
        // nothing in default implementation
    }

    void IScreen::handleRender(IGame* game, rendering::command::CommandWriter& cmd, const HostViewport& viewport)
    {
        // nothing in default implementation
    }

    bool IScreen::handleInput(IGame* game, const base::input::BaseEvent& evt)
    {
        return false;
    }

    void IScreen::handleStartHide(IGame* game)
    {
        
    }

    void IScreen::handleStartShow(IGame* game)
    {

    }

    void IScreen::handleDebug()
    {

    }

    //--

    ScreenTransitionEffectPtr IScreen::createDefaultShowTransition(IGame* game, IScreen* fromScreen) const
    {
        return nullptr; // no transition (instant swap)
    }

    ScreenTransitionEffectPtr IScreen::createDefaultHideTransition(IGame* game, IScreen* fromScreen) const
    {
        return nullptr; // no transition (instant swap)
    }

    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IScreenTransitionEffect);
    RTTI_END_TYPE();

    IScreenTransitionEffect::IScreenTransitionEffect()
    {}

    IScreenTransitionEffect::~IScreenTransitionEffect()
    {}

    bool IScreenTransitionEffect::finished() const
    {
        return true;
    }

    void IScreenTransitionEffect::handleUpdate(double dt)
    {}

    void IScreenTransitionEffect::handleRender(IGame* game, IScreen* screen, rendering::command::CommandWriter& cmd, const HostViewport& viewport)
    {
        if (screen)
            screen->handleRender(game, cmd, viewport);
    }

    //--

} // game


