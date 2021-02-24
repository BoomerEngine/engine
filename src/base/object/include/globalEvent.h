/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: event #]
***/

#pragma once

#define EVENT_FUNC base::StringID name, base::IObject* source, const void* data, base::Type dataType

BEGIN_BOOMER_NAMESPACE(base)

///---

typedef std::function<bool(EVENT_FUNC)> TEventFunction;

//--

END_BOOMER_NAMESPACE(base)
