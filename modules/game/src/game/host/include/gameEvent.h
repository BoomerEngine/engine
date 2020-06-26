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

    /// generalized game "event" - ie. controller disconnected, invite requested, game disconnection etc
    class GAME_HOST_API IEvent : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IEvent, base::IObject);

    public:
        virtual ~IEvent();

        /// describe (for debug)
        virtual void print(base::IFormatStream& f) const;
    };

    //--

    /// background event "supplier" class, gathers events from "outside world" for the use in the game
    class GAME_HOST_API IEventSupplier : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IEventSupplier, base::IObject);

    public:
        virtual ~IEventSupplier();

        /// initialize for given game host, we may choose not to, especially standalone server and in-editor game does not need all game events
        virtual bool initialize(Host* host) = 0;

        /// pull game event, called untill it returns null
        virtual EventPtr pull() = 0;
    };

    //--

} // game
