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

    /// A screen in the game, usually only one screen is active
    class GAME_HOST_API IScreen : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IScreen, base::IObject);

    public:
        IScreen();
        virtual ~IScreen();

        //--

        // does this screen support native fade in/fade out (so we don't have to render to a texture)
        virtual bool supportsNativeFadeInFadeOut() const;

        //--

        // check if screen is ready to be replaced (ie. fadeout ended, stuff was saved on server, etc)
        // NOTE: called only on the "leaving" side of screen transition
        virtual bool readyToHide(IGame* game);

        // check if screen is ready to be shown (ie. loading has finished, data was fetched from server, etc)
        // NOTE: called only on the "incoming" side of screen transition
        virtual bool readyToShow(IGame* game);

        //--

        // update with given engine time delta
        virtual void handleUpdate(IGame* game, double dt);

        // allow this state to process an external game event, this can trigger instant state transition
        virtual void handleEvent(IGame* game, const EventPtr& evt);

        // render to given rendering output
        virtual void handleRender(IGame* game, rendering::command::CommandWriter& cmd, const HostViewport& viewport);

        // service input message, TEMPSHIT, will be changed to game actions
        virtual bool handleInput(IGame* game, const base::input::BaseEvent& evt);

        // called when screen is requested to go away, screen should perform stuff necessary for it's demise
        virtual void handleStartHide(IGame* game);

        // called when screen screen is added to the list of incoming screens, it should start loading stuff 
        virtual void handleStartShow(IGame* game);

        // render debug ImGui
        virtual void handleDebug();

        //--

        // create a default transition for showing this screen, can return NULL for no transition
        virtual ScreenTransitionEffectPtr createDefaultShowTransition(IGame* game, IScreen* fromScreen) const;

        // create a default transition for hiding this screen, can return NULL for no transition
        virtual ScreenTransitionEffectPtr createDefaultHideTransition(IGame* game, IScreen* fromScreen) const;

        //--
    };

    //--

    /// screen transition effect
    class GAME_HOST_API IScreenTransitionEffect : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IScreenTransitionEffect, base::IObject);

    public:
        IScreenTransitionEffect();
        virtual ~IScreenTransitionEffect();

        //--

        /// check if transition has finished (if so it can be deleted and the screen can be rendered without it)
        virtual bool finished() const;

        //--

        /// advance transition
        virtual void handleUpdate(double dt);

        /// render the game screen using this transition (may render to temporary RT and than do a post process, may mask out stuff, etc)
        virtual void handleRender(IGame* game, IScreen* screen, rendering::command::CommandWriter& cmd, const HostViewport& viewport);
    };

    //--

} // game
