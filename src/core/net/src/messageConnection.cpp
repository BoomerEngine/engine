/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: messages #]
***/

#include "build.h"
#include "messageConnection.h"

BEGIN_BOOMER_NAMESPACE_EX(net)

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MessageConnection);
    RTTI_FUNCTION("getConnectionId", connectionId);
    RTTI_FUNCTION("getLocalAddress", localAddress);
    RTTI_FUNCTION("getRemoteAddress", remoteAddress);
    RTTI_FUNCTION("isConnected", isConnected);
    //RTTI_FUNCTION("send", sendScripted);
RTTI_END_TYPE();

MessageConnection::MessageConnection()
{}

MessageConnection::~MessageConnection()
{}

//--

END_BOOMER_NAMESPACE_EX(net)
