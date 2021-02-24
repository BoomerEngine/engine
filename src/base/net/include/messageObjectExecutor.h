/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#pragma once

#include "base/object/include/object.h"
#include "base/system/include/thread.h"
#include "base/system/include/spinLock.h"
#include "base/containers/include/queue.h"

BEGIN_BOOMER_NAMESPACE(base::net)

//--

/// check if given object class supports given message type
extern BASE_NET_API bool CheckMessageSupport(ClassType contextObjectType, ClassType objectType, Type messageType);

/// dispatch message on object - will call matching function, NOTE: the type of the context object matters, ie, you may have more functions:
/// void handleMyMessage(const MyMessage& msg, const RefPtr<Player>& blah); <- called if context object is Player (or downclass)
/// void handleMyMessage(const MyMessage& msg, const RefPtr<Game>& blah); <- called if context object is Game (or downclass)
/// void handleMyMessage(const MyMessage& msg); <- this actually may be called as a fallback if more specific one is not found
/// NOTE: returns true if message was dispatched or FALSE if we didn't find a matching function
extern BASE_NET_API bool DispatchObjectMessage(IObject* object, Type messageType, const void* messagePayload, IObject* contextObject = nullptr);

//--

END_BOOMER_NAMESPACE(base::net)
