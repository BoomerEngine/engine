/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
***/

#include "build.h"
#include "graphContainer.h"
#include "graphBlock.h"
#include "graphSocket.h"
#include "graphConnection.h"
#include "graphObserver.h"
#include "core/object/include/streamOpcodeReader.h"
#include "core/object/include/streamOpcodeWriter.h"
#include "core/reflection/include/reflectionTypeName.h"
//#include "graphViewNative.h"

BEGIN_BOOMER_NAMESPACE_EX(graph)

//--

RTTI_BEGIN_TYPE_STRUCT(PersistentConnection);
    RTTI_PROPERTY(firstBlockIndex);
    RTTI_PROPERTY(firstSocketName);
    RTTI_PROPERTY(secondBlockIndex);
    RTTI_PROPERTY(secondSocketName);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_STRUCT(PersistentBlockState);
    RTTI_PROPERTY(placement);
RTTI_END_TYPE();

PersistentBlockState::PersistentBlockState()
    : placement(0, 0)
{}

//--

IGraphObserver::~IGraphObserver()
{}

//--

RTTI_BEGIN_TYPE_CLASS(SupprotedBlockConnectionInfo);
    RTTI_PROPERTY(blockClass);
    RTTI_PROPERTY(socketNames);
RTTI_END_TYPE();

SupprotedBlockConnectionInfo::SupprotedBlockConnectionInfo()
{}

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(Container);
    RTTI_PROPERTY(m_blocks);
    RTTI_PROPERTY(m_persistentConnections);
    ///RTTI_PROPERTY(m_persistentBlockState);
RTTI_END_TYPE();

Container::Container()
{
}

Container::~Container()
{
}

bool Container::canAddBlockOfClass(ClassType blockClass) const
{
    return !blockClass->isAbstract() && blockClass->is(Block::GetStaticClass());
}

bool Container::canDeleteBlock(const BlockPtr& block) const
{
    return true;
}

void Container::supportedBlockConnections(const Socket* sourceSocket, Array<SupprotedBlockConnectionInfo>& outSupportedBlocks) const
{
    InplaceArray<SpecificClassType<Block>, 100> allBlockClasses;
    supportedBlockClasses(allBlockClasses);

    for (const auto blockClass : allBlockClasses)
    {
        if (!canAddBlockOfClass(blockClass))
            continue;

        if (auto tempBlock = blockClass.create()) // TODO: cache ?
        {
            tempBlock->rebuildLayout();

            SupprotedBlockConnectionInfo* entry = nullptr;

            for (const auto& destSocket : tempBlock->sockets())
            {
                if (Socket::CanConnect(*sourceSocket, *destSocket))
                {
                    if (!entry)
                    {
                        entry = &outSupportedBlocks.emplaceBack();
                        entry->blockClass = blockClass;
                    }

                    entry->socketNames.pushBack(destSocket->name());
                }
            }
        }
    }
}

void Container::notifyStructureChanged()
{
    TBaseClass::markModified();
}

void Container::removeBlock(const BlockPtr& block)
{
    ASSERT_EX(block, "Block to remove must be specified");
    ASSERT_EX(m_blocks.contains(block), "Block is not register in this container");

    m_blocks.removeUnordered(block);
    block->parent(nullptr);

    notifyBlockRemoved(block);
    notifyStructureChanged();
}

void Container::addBlock(const BlockPtr& block)
{
    ASSERT_EX(block, "Block to remove must be specified");
    ASSERT_EX(!m_blocks.contains(block), "Block is alredy register in this container");

    m_blocks.pushBack(block);
    block->parent(this);
    block->rebuildLayout();

    notifyBlockAdded(block);
    notifyStructureChanged();
}

//--

void Container::attachObserver(IGraphObserver* observer)
{
    if (observer)
    {
        DEBUG_CHECK(!m_observers.contains(observer));
        m_observers.pushBack(observer);
    }
}

void Container::dettachObserver(IGraphObserver* observer)
{
    DEBUG_CHECK(m_observers.contains(observer));

    if (auto index = m_observers.find(observer))
        m_observers[index] = nullptr;
}

void Container::notifyBlockAdded(Block* block)
{
    for (auto* observer : m_observers)
        if (observer)
            observer->handleBlockAdded(block);

    m_observers.removeUnorderedAll(nullptr);
}

void Container::notifyBlockRemoved(Block* block)
{
    for (auto* observer : m_observers)
        if (observer)
            observer->handleBlockRemoved(block);

    m_observers.removeUnorderedAll(nullptr);
}

void Container::notifyBlockStyleChanged(Block* block)
{
    for (auto* observer : m_observers)
        if (observer)
            observer->handleBlockStyleChanged(block);

    m_observers.removeUnorderedAll(nullptr);
}

void Container::notifyBlockLayoutChanged(Block* block)
{
    for (auto* observer : m_observers)
        if (observer)
            observer->handleBlockLayoutChanged(block);

    m_observers.removeUnorderedAll(nullptr);
}

void Container::notifyBlockConnectionsChanged(Block* block)
{
    for (auto* observer : m_observers)
        if (observer)
            observer->handleBlockConnectionsChanged(block);

    m_observers.removeUnorderedAll(nullptr);
}

//--

namespace helper
{
    // store only connections going one way
    static bool ShouldStore(const PersistentConnection& con)
    {
        if (con.firstBlockIndex == con.secondBlockIndex)
            return con.firstSocketName < con.secondSocketName;
        return con.firstBlockIndex < con.secondBlockIndex;
    }

    // extract all connections in given block
    static void ExtractConnections(const BlockPtr& block, const HashMap<const Block*, int >& blockMap, Array<PersistentConnection>& outConnections)
    {
        if (block)
        {
            int firstBlockIndex = INDEX_NONE;
            blockMap.find(block.get(), firstBlockIndex);
            ASSERT_EX(firstBlockIndex != INDEX_NONE, "Block not found in the block map");

            for (auto& socket : block->sockets())
            {
                auto firstSocketName = socket->name();

                // process the connections
                for (auto& con : socket->connections())
                {
                    // get second socket
                    auto secondSocket = con->otherSocket(socket);

                    // get second block
                    auto secondBlock = secondSocket->block();
                    int secondBlockIndex = INDEX_NONE;
                    blockMap.find(secondBlock, secondBlockIndex);
                    ASSERT_EX(secondBlockIndex != INDEX_NONE, "Block not found in the block map");

                    // setup persistent connection data
                    PersistentConnection info;
                    info.firstBlockIndex = range_cast<uint32_t>(firstBlockIndex);
                    info.firstSocketName = firstSocketName;
                    info.secondBlockIndex = range_cast<uint32_t>(secondBlockIndex);
                    info.secondSocketName = secondSocket->name();

                    // store connection only for half of the shit
                    if (ShouldStore(info))
                        outConnections.pushBack(info);
                }
            }
        }
    }
} //helper

void Container::onReadBinary(stream::OpcodeReader& reader)
{
    TBaseClass::onReadBinary(reader);

    if (reader.version() >= VER_THREAD_SAFE_GRAPHS)
        readPersistentConnections(reader);
}

void Container::onWriteBinary(stream::OpcodeWriter& writer) const
{
    TBaseClass::onWriteBinary(writer);
    writePersistentConnections(writer);
}

void Container::readPersistentConnections(stream::OpcodeReader& reader)
{
    m_persistentConnections.reset();

    TypeSerializationContext context;
    const auto dataType = GetTypeObject<Array<PersistentConnection>>();
    dataType->readBinary(context, reader, &m_persistentConnections);
}

void Container::writePersistentConnections(stream::OpcodeWriter& writer) const
{
    // prepare block mapping
    // NOTE: this must match the way the blocks are stored
    HashMap<const Block*, int> blockMapping;
    blockMapping.reserve(m_blocks.size());
    for (uint32_t i = 0; i < m_blocks.size(); ++i)
        blockMapping.set(m_blocks[i].get(), i);

    // reserve memory
    Array<PersistentConnection> persistentConnections;
    persistentConnections.reset();
    persistentConnections.reserve(m_blocks.size() * 2);
                
    // gather connections
    for (auto& block : m_blocks)
        helper::ExtractConnections(block, blockMapping, persistentConnections);

    // status
    TRACE_INFO("Captured {} connections for saving from {} blocks", persistentConnections.size(), m_blocks.size());

    // save data
    {
        TypeSerializationContext context;
        const auto dataType = GetTypeObject<Array<PersistentConnection>>();
        dataType->writeBinary(context, writer, &persistentConnections, nullptr);
    }
}

void Container::onPostLoad()
{
    // process base object
    TBaseClass::onPostLoad();

    // remove empty blocks
    m_blocks.removeAll(nullptr);

    // rebuild the layout data for each block
    // this will create sockets
    for (auto& block : m_blocks)
        block->rebuildLayout();

    // apply the stored connections to blocks
    if (!m_persistentConnections.empty())
    {
        applyConnections(m_persistentConnections);
        m_persistentConnections.clear();
    }
}

void Container::applyConnections(const Array<PersistentConnection>& persistentConnections)
{
    // apply the persistent connections to the sockets
    uint32_t numFailedConnections = 0;
    for (uint32_t i = 0; i < persistentConnections.size(); ++i)
    {
        auto& con = persistentConnections[i];

        // validate first block index
        if (con.firstBlockIndex >= m_blocks.size())
        {
            TRACE_ERROR("Persistent connection {} uses invalid first block index {} (there are only {} blocks)", i, con.firstBlockIndex, m_blocks.size());
            numFailedConnections += 1;
            continue;
        }

        // validate the second blocks
        if (con.secondBlockIndex >= m_blocks.size())
        {
            TRACE_ERROR("Persistent connection {} uses invalid second block index {} (there are only {} blocks)", i, con.secondBlockIndex, m_blocks.size());
            numFailedConnections += 1;
            continue;
        }

        // get the first block
        auto firstBlock = m_blocks[con.firstBlockIndex];
        if (!firstBlock)
        {
            TRACE_ERROR("Persistent connection {} uses first block index {} that is now null", i, con.firstBlockIndex);
            numFailedConnections += 1;
            continue;
        }
        else if (!firstBlock)
        {
            TRACE_ERROR("Persistent connection {} uses first block index {} (class '{}') that has no layout", i,
                con.firstBlockIndex, firstBlock->cls()->name().c_str());
            numFailedConnections += 1;
            continue;
        }

        // get the second block
        auto secondBlock = m_blocks[con.secondBlockIndex];
        if (!secondBlock)
        {
            TRACE_ERROR("Persistent connection {} uses second block index {} that is now null", i, con.secondBlockIndex);
            numFailedConnections += 1;
            continue;
        }
        else if (!secondBlock)
        {
            TRACE_ERROR("Persistent connection {} uses second block index {} (class '{}') that has no layout", i,
                con.secondBlockIndex, secondBlock->cls()->name().c_str());
            numFailedConnections += 1;
            continue;

        }

        // get the first socket
        auto firstSocket = firstBlock->findSocket(con.firstSocketName);
        if (!firstSocket)
        {
            TRACE_ERROR("Persistent connection {} uses socket '{}' in first block index {} (class '{}') that no longer exists", i,
                con.firstSocketName.c_str(), con.firstBlockIndex, firstBlock->cls()->name().c_str());
            numFailedConnections += 1;
            continue;
        }

        // get the second socket
        auto secondSocket = secondBlock->findSocket(con.secondSocketName);
        if (!secondSocket)
        {
            TRACE_ERROR("Persistent connection {} uses socket '{}' in second block index {} (class '{}') that no longer exists", i,
                con.secondSocketName.c_str(), con.secondBlockIndex, secondBlock->cls()->name().c_str());
            numFailedConnections += 1;
            continue;
        }

        // finally, check if the connection is still possible
        if (!Socket::CanConnect(*firstSocket, *secondSocket))
        {
            TRACE_ERROR("Persistent connection {} between socket '{}' in block index {} (class '{}') and socket '{}' in block index {} (class '{}') is no longer possible", i,
                con.firstSocketName.c_str(), con.firstBlockIndex, firstBlock->cls()->name().c_str(),
                con.secondSocketName.c_str(), con.secondBlockIndex, secondBlock->cls()->name().c_str());
            numFailedConnections += 1;
            continue;
        }

        // connect the blocks
        firstSocket->connectTo(secondSocket);
    }

    // stats
    TRACE_INFO("Restored {} persistent connections ({} failed)", persistentConnections.size(), numFailedConnections);
}

//--

ContainerPtr Container::extractSubGraph(const Array<BlockPtr>& fromBlocks) const
{
    // map blocks to local indices
    uint32_t nextBlockID = 0;
    HashMap<const Block*, uint32_t> blockIdMap;
    for (const auto& block : fromBlocks)
    {
        if (!block || blockIdMap.contains(block))
            continue;

        DEBUG_CHECK_EX(block->parent() == this, "Block not from this graph?");
        if (block->parent() != this)
            continue;

        bool inBlockList = m_blocks.contains(block);
        DEBUG_CHECK_EX(inBlockList, "Block no in local block list");
        if (!inBlockList)
            continue;

        blockIdMap[block] = nextBlockID++;
    }

    // create block copies
    auto newContainer = cls()->create<Container>();

    Array<Block*> newBlocks;
    newBlocks.reserve(nextBlockID);
    for (const auto* block : blockIdMap.keys())
    {
        auto newBlock = rtti_cast<Block>(block->clone());
        if (!newBlock)
        {
            TRACE_ERROR("Unable to clone block '{}' when making graph copy", block->chooseTitle());
            return nullptr;
        }

        newBlock->rebuildLayout();

        newContainer->addBlock(newBlock);
        newBlocks.pushBack(newBlock);
    }

    // restore connections in internal blocks
    uint32_t numDroppedConnections = 0;
    uint32_t numCopiedConnections = 0;
    for (const auto* block : blockIdMap.keys())
    {
        uint32_t sourceBlockId = 0;
        if (!blockIdMap.find(block, sourceBlockId))
            continue;
        TRACE_INFO("Sourceblock '{}': {}", block->chooseTitle(), sourceBlockId);
        for (const auto& orgSourceSocket : block->sockets())
        {
            TRACE_INFO("SourceSocket {}", orgSourceSocket->name());
            for (const auto& orgCon : orgSourceSocket->connections())
            {
                if (auto orgTargetSocket = orgCon->otherSocket(orgSourceSocket))
                {
                    TRACE_INFO("SourceConnection: to '{}'", orgTargetSocket->name());

                    // map target block
                    uint32_t targetBlockId = 0;
                    if (blockIdMap.find(orgTargetSocket->block(), targetBlockId))
                    {
                        TRACE_INFO("TargetBlockID: '{}' {}", orgTargetSocket->block()->chooseTitle(), targetBlockId);

                        // copy only half of connections (they are symmetric)
                        if (orgSourceSocket < orgTargetSocket)
                        {
                            TRACE_INFO("ODD");
                            continue;
                        }

                        // connect new block
                        auto* newSourceBlock = newBlocks[sourceBlockId];
                        auto* newTargetBlock = newBlocks[targetBlockId];

                        // find source socket
                        auto* newSourceSocket = newSourceBlock->findSocket(orgSourceSocket->name());
                        if (!newSourceSocket)
                        {
                            TRACE_ERROR("Unable to find socket '{}' in cloned block '{}'", orgSourceSocket->name(), newSourceBlock->chooseTitle());
                            return nullptr;
                        }

                        // find target socket
                        auto* newTargetSocket = newTargetBlock->findSocket(orgTargetSocket->name());
                        if (!newTargetSocket)
                        {
                            TRACE_ERROR("Unable to find socket '{}' in cloned block '{}'", orgTargetSocket->name(), orgTargetSocket->block()->chooseTitle());
                            return nullptr;
                        }

                        // connect
                        if (!newSourceSocket->connectTo(newTargetSocket))
                        {
                            TRACE_ERROR("Unable to connect socket '{}' in block '{}' with socket '{}' in cloned block '{}'",
                                orgSourceSocket->name(), newSourceBlock->chooseTitle(),
                                orgTargetSocket->name(), orgTargetSocket->block()->chooseTitle());
                            return nullptr;
                        }

                        TRACE_INFO("Reconnected '{}' in block '{}' with socket '{}' in cloned block '{}'",
                            orgSourceSocket->name(), newSourceBlock->chooseTitle(),
                            orgTargetSocket->name(), orgTargetSocket->block()->chooseTitle());
                        numCopiedConnections += 1;
                    }
                    else
                    {
                        numDroppedConnections += 1;
                    }
                }
            }
        }
    }

    TRACE_INFO("Copied sub graph of {} (of {} total). Copiled {} connections, {} dropped", newBlocks.size(), m_blocks.size(), numCopiedConnections, numDroppedConnections);
    return newContainer;
}

//--
        
END_BOOMER_NAMESPACE_EX(graph)
