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

} // game
