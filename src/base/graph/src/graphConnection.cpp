/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#include "build.h"
#include "graphSocket.h"
#include "graphConnection.h"
#include "graphBlock.h"

BEGIN_BOOMER_NAMESPACE(base::graph)

//--

Connection::Connection(Socket* source, Socket* target, bool enabled /*= true*/)
    : m_enabled(enabled)
    , m_first(source)
    , m_second(target)
{}

Connection::~Connection()
{}

void Connection::toggle(bool isEnabled)
{
    if (m_enabled != isEnabled)
    {
        m_enabled = isEnabled;

        auto first = m_first.lock();
        if (first)
            first->notifyConnectionsChanged();

        auto second = m_second.lock();
        if (second)
            second->notifyConnectionsChanged();
    }
}

Socket* Connection::otherSocket(const Socket* sourceSocket) const
{
    if (m_first == sourceSocket)
        return m_second.unsafe();
    else if (m_second == sourceSocket)
        return m_first.unsafe();
    return nullptr;
}

bool Connection::match(const Socket* a, const Socket* b) const
{
    return ((m_first == a) && (m_second == b)) || ((m_first == b) && (m_second == a));
}

//--

END_BOOMER_NAMESPACE(base::graph)
