/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#pragma once

#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE_EX(graph)

// a connection between sockets
// NOTE: there is only one connection object shared between two sockets (it's ok since it's not saved and parenting is not required)
// NOTE: the connection is editable object for the purpose of editor, not serialization as the connection is not saved directly
class CORE_GRAPH_API Connection : public IReferencable
{
public:
    Connection(Socket* first, Socket* second, bool enabled=true);
    virtual ~Connection();

    // is this connection enabled ?
    INLINE bool enabled() const { return m_enabled; }

    // get first connected socket
    INLINE Socket* first() const { return m_first.unsafe(); }

    // get the destination socket
    INLINE Socket* second() const { return m_second.unsafe(); }

    // enable/disable the connection
    void toggle(bool isEnabled);

    // get the other socket for a connection
    // NOTE: if the passed source socket is not part of the connection a NULL is returned
    Socket* otherSocket(const Socket* sourceSocket) const;

    // test if socket match the connection
    bool match(const Socket* a, const Socket* b) const;

private:
    // is the connection enabled ?
    bool m_enabled;

    // connected sockets
    SocketWeakPtr m_first;
    SocketWeakPtr m_second;
};

END_BOOMER_NAMESPACE_EX(graph)
