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

namespace game
{
    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IEvent);
    RTTI_END_TYPE();

    IEvent::~IEvent()
    {}

    void IEvent::print(base::IFormatStream& f) const
    {}

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IEventSupplier);
    RTTI_END_TYPE();

    IEventSupplier::~IEventSupplier()
    {}

    //--

} // game


