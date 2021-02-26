/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: event #]
***/

#pragma once

#define EVENT_FUNC StringID name, IObject* source, const void* data, Type dataType

BEGIN_BOOMER_NAMESPACE()

///---

typedef std::function<bool(EVENT_FUNC)> TEventFunction;

//--

END_BOOMER_NAMESPACE()
