/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#include "build.h"
#include "graphBlock.h"
#include "graphSocket.h"
#include "graphConnection.h"
#include "graphContainer.h"

namespace base
{
    namespace graph
    {

        //--

        RTTI_BEGIN_TYPE_ENUM(BlockShape);
            RTTI_ENUM_OPTION(RectangleWithTitle);
            RTTI_ENUM_OPTION(SlandedWithTitle);
            RTTI_ENUM_OPTION(RoundedWithTitle);
            RTTI_ENUM_OPTION(Rectangle);
            RTTI_ENUM_OPTION(Slanded);
            RTTI_ENUM_OPTION(Rounded);
            RTTI_ENUM_OPTION(Circle);
            RTTI_ENUM_OPTION(Octagon);
            RTTI_ENUM_OPTION(TriangleRight);
            RTTI_ENUM_OPTION(TriangleLeft);
            RTTI_ENUM_OPTION(ArrowRight);
            RTTI_ENUM_OPTION(ArrowLeft);            
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(BlockSocketStyleInfo);
        RTTI_PROPERTY(m_direction);
        RTTI_PROPERTY(m_placement);
        RTTI_PROPERTY(m_socketColor);
        RTTI_PROPERTY(m_linkColor);
        RTTI_PROPERTY(m_tags);
        RTTI_PROPERTY(m_excludedTags);
        RTTI_PROPERTY(m_swizzle);
        RTTI_PROPERTY(m_multiconnection);
        RTTI_PROPERTY(m_visibleByDefault);
        RTTI_PROPERTY(m_hiddableByUser);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(BlockInfoMetadata);
        RTTI_END_TYPE();

        RTTI_BEGIN_TYPE_CLASS(BlockShapeMetadata);
        RTTI_END_TYPE();

        RTTI_BEGIN_TYPE_CLASS(BlockTitleColorMetadata);
        RTTI_END_TYPE();

        RTTI_BEGIN_TYPE_CLASS(BlockBorderColorMetadata);
        RTTI_END_TYPE();

        RTTI_BEGIN_TYPE_CLASS(BlockHelpMetadata);
        RTTI_END_TYPE();

        RTTI_BEGIN_TYPE_CLASS(BlockStyleNameMetadata);
        RTTI_END_TYPE();

        //--

        void BlockLayoutBuilder::socket(StringID name, const BlockSocketStyleInfo& info)
        {
            if (name)
            {
                for (auto& existingInfo : collectedSockets)
                {
                    if (existingInfo.name == name)
                    {
                        existingInfo.info = info;
                        return;
                    }
                }

                auto& newEntry = collectedSockets.emplaceBack();
                newEntry.name = name;
                newEntry.info = info;
            }
        }

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(Block);
            RTTI_METADATA(BlockInfoMetadata).group("Generic").title("Block");
            RTTI_PROPERTY(m_position);
            //RTTI_PROPERTY(m_sockets);
        RTTI_END_TYPE();

        Block::Block()
        {
        }

        Block::~Block()
        {
        }

        void Block::invalidateStyle()
        {
            if (auto container = graph())
                container->notifyBlockStyleChanged(this);
        }

        BlockShape Block::chooseBlockShape() const
        {
            if (const auto* info = cls()->findMetadata<BlockShapeMetadata>())
                return info->value;
            return BlockShape::RectangleWithTitle;
        }

        StringBuf Block::chooseTitle() const
        {
            if (const auto* info = cls()->findMetadata<BlockInfoMetadata>())
                return base::StringBuf(info->titleString);

            return StringBuf("Block");
        }

        Color Block::chooseTitleColor() const
        {
            if (const auto* info = cls()->findMetadata<BlockTitleColorMetadata>())
                return info->value;
            return Color(255, 0, 255, 255);
        }

        Color Block::chooseBorderColor() const
        {
            if (const auto* info = cls()->findMetadata<BlockBorderColorMetadata>())
                return info->value;
            return Color(0, 0, 0, 0);
        }

        void Block::buildLayout(BlockLayoutBuilder& builder) const
        {

        }

        void Block::rebuildLayout(bool postEvent /*= true*/)
        {
            // build new socket list
            BlockLayoutBuilder builder;
            buildLayout(builder);

            // extract existing sockets
            auto existingSockets = std::move(m_sockets);

            // add new sockets
            for (auto& it : builder.collectedSockets)
            {
                // find existing socket to update
                bool alreadyExists = false;
                for (uint32_t i = 0; i < existingSockets.size(); ++i)
                {
                    auto existingSocket = existingSockets[i];
                    auto existingSocketName = existingSocket->name();
                    if (existingSocketName == it.name)
                    {
                        // update the existing socket
                        existingSocket->updateSocketInfo(it.info);
                        m_sockets.pushBack(existingSocket);

                        // erase the entry
                        existingSockets.erase(i);
                        alreadyExists = true;
                        break;
                    }
                }

                // create new socket
                if (!alreadyExists)
                {
                    auto newSocket = base::RefNew<Socket>(it.name, it.info);
                    newSocket->parent(this);
                    m_sockets.pushBack(newSocket);
                }
            }

            // remove sockets that are no longer needed in the layout
            for (auto& it : existingSockets)
                it->removeAllConnections();

            // notify everybody
            if (postEvent)
            {
                if (auto container = graph())
                    container->notifyBlockLayoutChanged(this);
            }
        }

        Socket* Block::findSocket(StringID socketName) const
        {
            for (const auto& socket : m_sockets)
                if (socket->name() == socketName)
                    return socket;

            return nullptr;
        }

        void Block::handleConnectionsChanged()
        {
            if (auto container = graph())
                container->notifyBlockConnectionsChanged(this);

            markModified();
        }


        void Block::handleSocketLayoutChanged()
        {
            if (auto container = graph())
                container->notifyBlockLayoutChanged(this);
        }

        //--

        bool Block::hasConnections() const
        {
            for (const auto& socket : m_sockets)
                if (socket->hasConnections())
                    return true;
            return false;
        }

        bool Block::hasAnyConnectionTo(const Block* block) const
        {
            for (const auto& socket : m_sockets)
                if (socket->hasConnectionsToBlock(block))
                    return true;
            return false;
        }

        bool Block::hasConnectionOnSocket(StringID name) const
        {
            for (const auto& socket : m_sockets)
                if (socket->name() == name)
                    return socket->hasConnections();
            return false;
        }

        bool Block::hasAnyConnectionTo(const Socket* socket) const
        {
            for (const auto& socket : m_sockets)
                if (socket->hasConnectionsToSocket(socket))
                    return true;
            return false;
        }

        void Block::breakAllConnections()
        {
            for (const auto& socket : m_sockets)
                socket->removeAllConnections();
        }

        bool Block::enumConnections(const std::function<bool(Connection*)>& enumFunc) const
        {
            for (const auto& it : m_sockets)
            {
                for (const auto& con : it->connections())
                {
                    // do not report self-connections twice
                    if (con->first()->block() == con->second()->block())
                        if (con->first()->name() < con->second()->name())
                            continue;

                    // report connection
                    if (enumFunc(con))
                        return true;
                }
            }

            return false;
        }

        void Block::getConnections(Array<Connection*>& outConnections) const
        {
            outConnections.reserve(m_sockets.size() * 2);
            for (const auto& it : m_sockets)
            {
                for (const auto& con : it->connections())
                {
                    // do not report self-connections twice
                    if (con->first()->block() == con->second()->block())
                        if (con->first()->name() < con->second()->name())
                            continue;

                    // report connection
                    outConnections.emplaceBack(con);
                }
            }
        }

    } // graph
} // base