/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
*/

#include "build.h"
#include "uiEventNotifier.h"

namespace ui
{
    ///--

    EventToken::EventToken(const TUnregisterFunction& func)
        : m_unregisterFunc(func)
    {}

    EventToken::~EventToken()
    {
        unregisterNow();
    }

    void EventToken::unbind()
    {
        m_unregisterFunc = TUnregisterFunction();
    }

    void EventToken::unregisterNow()
    {
        auto func = std::move(m_unregisterFunc);
        if (func)
            func();
    }

    ///--

} // ui
 