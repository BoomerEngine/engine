/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#include "build.h"
#include "graphBlock.h"
#include "graphConnection.h"
#include "graphSocket.h"

BEGIN_BOOMER_NAMESPACE_EX(graph)

//--

RTTI_BEGIN_TYPE_ENUM(SocketDirection);
    RTTI_ENUM_OPTION(Input);
    RTTI_ENUM_OPTION(Output);
    RTTI_ENUM_OPTION(Bidirectional);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_ENUM(SocketPlacement);
    RTTI_ENUM_OPTION(Top);
    RTTI_ENUM_OPTION(Bottom);
    RTTI_ENUM_OPTION(Left);
    RTTI_ENUM_OPTION(Right);
    RTTI_ENUM_OPTION(Center);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_CLASS(Socket);
    //RTTI_PROPERTY(m_connections); // Connections are not saved here, they are saved in the graph
    RTTI_PROPERTY(m_name);
    RTTI_PROPERTY(m_info);
    RTTI_PROPERTY(m_visible);
RTTI_END_TYPE();

//--

Socket::Socket()
{}

Socket::Socket(StringID name, const BlockSocketStyleInfo& info)
    : m_name(name)
    , m_info(info)
{
    m_visible = m_info.m_visibleByDefault;
}

Socket::~Socket()
{}

Block* Socket::block() const
{
    return rtti_cast<Block>(parent());
}

void Socket::notifyLayoutChanged()
{
    if (auto block = this->block())
        block->handleSocketLayoutChanged();
}

void Socket::notifyConnectionsChanged()
{
    if (auto block = this->block())
        block->handleConnectionsChanged();
}

namespace helper
{
    static Socket* GetConnectedSocket(const Connection* connection, const Socket* socket)
    {
        if (connection->first() == socket)
            return connection->second();
        else if (connection->second() == socket)
            return connection->first();
        else
            return nullptr;
    }
} // helper

void Socket::updateSocketInfo(const BlockSocketStyleInfo& info)
{
    m_info = info;
}

void Socket::removeAllConnections()
{
    auto connectionsToRemove = std::move(m_connections);

    for (auto& it : connectionsToRemove)
        if (auto target = helper::GetConnectedSocket(it, this))
            target->removeAllConnectionsToSocket(this);

    notifyConnectionsChanged();
}

void Socket::updateVisibility(bool visible)
{
    if (!visible && !m_connections.empty())
        return;

    if (m_visible != visible)
    {
        m_visible = visible;
        notifyLayoutChanged();
    }
}

void Socket::removeAllConnectionsToSocket(const Socket* to)
{
    bool somethingRemoved = false;
    bool somethingRemovedAtLeastOnce = false;
    do
    {
        somethingRemoved = false;
        for (uint32_t i = 0; i < m_connections.size(); ++i)
        {
            auto target = helper::GetConnectedSocket(m_connections[i], this);
            if (target == to)
            {
                // remove from this table first to prevent recursion
                m_connections.erase(i);
                somethingRemoved = true;
                somethingRemovedAtLeastOnce = true;

                // remove from the other side (will recurse but nothing will be deleted)
                target->removeAllConnectionsToSocket(this);
                break; // the connections may have been totally changed, restart checking
            }
        }

    }
    while (somethingRemoved);

    if (somethingRemovedAtLeastOnce)
        notifyConnectionsChanged();
}

void Socket::removeAllConnectionsToBlock(const Block* to)
{
    bool somethingRemoved = false;
    bool somethingRemovedAtLeastOnce = false;
    do
    {
        for (uint32_t i = 0; i < m_connections.size(); ++i)
        {
            auto target = helper::GetConnectedSocket(m_connections[i], this);
            if (target->block() == to)
            {
                target->removeAllConnectionsToSocket(this);
                m_connections.erase(i);
                somethingRemoved = true;
                somethingRemovedAtLeastOnce = true;
                break; // the connections may have been totally changed, restart checking
            }
        }

    }
    while (somethingRemoved);

    if (somethingRemovedAtLeastOnce)
        notifyConnectionsChanged();
}

bool Socket::hasConnectionsToSocket(const Socket* to) const
{
    for (auto& it : m_connections)
    {
        auto target = helper::GetConnectedSocket(it, this);
        if (target == to)
        {
            return true;
        }
    }

    return false;
}

bool Socket::hasConnectionsToBlock(const Block* to) const
{
    for (auto& it : m_connections)
    {
        auto target = helper::GetConnectedSocket(it, this);
        if (target && target->block() == to)
            return true;
    }

    return false;
}

bool Socket::connectTo(Socket* to)
{
    // we cannot connect to empty socket
    if (!to)
        return false;

    // validate connection rules
    if (!CanConnect(*this, *to))
        return false;

    // do not connect twice
    if (hasConnectionsToSocket(to))
        return false;

    // create the connection
    // NOTE: object is NOT parented to anything
    auto connection = RefNew<Connection>(this, to);

    // add connection to both lists
    m_connections.pushBack(connection);
    to->m_connections.pushBack(connection);

    // show all sockets
    updateVisibility(true);
    to->updateVisibility(true);

    // notify
    notifyConnectionsChanged();
    to->notifyConnectionsChanged();
    return true;
}

bool Socket::CheckDirections(const Socket& a, const Socket& b)
{
    if (a.info().m_direction == SocketDirection::Bidirectional)
        return true;

    if (b.info().m_direction == SocketDirection::Bidirectional)
        return true;

    if (a.info().m_direction == SocketDirection::Input && b.info().m_direction == SocketDirection::Output)
        return true;

    if (a.info().m_direction == SocketDirection::Output && b.info().m_direction == SocketDirection::Input)
        return true;

    return false;
}

bool Socket::CheckTags(const Socket& a, const Socket& b)
{
    // empty tag list matches with everything
    if (!a.info().m_tags.empty())
    {
        // if the tag list is not empty we need to find the tag in the target list
        bool hasTargetTag = false;
        for (auto& tag : a.info().m_tags)
        {
            if (b.info().m_tags.contains(tag))
            {
                hasTargetTag = true;
                break;
            }
        }

        if (!hasTargetTag)
            return false;
    }

    // check the exclusion tags
    for (auto& tag : a.info().m_excludedTags)
        if (b.info().m_tags.contains(tag))
            return false;

    // we can connect
    return true;
}

static bool SameConnection(const Connection* a, const Connection* b)
{
    if (a == b)
        return true;

    if (!a || !b)
        return false;

    return ((a->first() == b->first()) && (a->second() == b->second())) ||
        ((a->first() == b->second()) && (a->second() == b->first())); // NOTE: this should NOT happen
}

static bool SameConnection(const Connection* a, const Socket* source, const Socket* target)
{
    return ((a->first() == source) && (a->second() == target)) ||
        ((a->first() == target) && (a->second() == source));
}

static bool ConnectionInList(const Array<Connection*>* connectionList, const Socket* source, const Socket* target)
{
    if (!connectionList)
        return false;

    for (const auto& con : *connectionList)
        if (SameConnection(con, source, target))
            return true;

    return false;
}

bool Socket::CheckDuplicates(const Socket& a, const Socket& b, const Array<Connection*>* removedConnections)
{
    for (const auto& con : a.connections())
    {
        if (SameConnection(con, &a, &b))
        {
            if (!ConnectionInList(removedConnections, &a, &b))
                return false;
        }
    }

    return true;
}

bool Socket::CheckMulticonnections(const Socket& a, const Array<Connection*>* removedConnections/*=nullptr*/)
{
    // we cannot connect to a socket that is full...
    if (!a.info().m_multiconnection)
    {
        // ... unless the connection is in a list of connections to be removed
        for (auto &con : a.connections())
            if (!ConnectionInList(removedConnections, con->first(), con->second()))
                return false; // existing connection will not be removed, cannot create new ones
    }

    // there's space in the socket for the connection
    return true;
}

bool Socket::CanConnect(const Socket& from, const Socket& to, const Array<Connection*>* removedConnections/*=nullptr*/)
{
    // we cannot self connect
    if (&from == &to)
        return false;

    // we can connect only if direction is fine
    if (!CheckDirections(from, to))
        return false;

    // validate tags (both ways)
    if (!CheckTags(from, to) || !CheckTags(to, from))
        return false;

    // check that we do not duplicate any existing connections
    if (!CheckDuplicates(from, to, removedConnections) || !CheckDuplicates(to, from, removedConnections))
        return false;

    // make sure there's space in the sockets
    if (!CheckMulticonnections(from, removedConnections) || !CheckMulticonnections(to, removedConnections))
        return false;

    // we can connect
    return true;
}

//--

END_BOOMER_NAMESPACE_EX(graph)
