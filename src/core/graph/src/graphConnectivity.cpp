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
#include "graphContainer.h"
#include "graphConnectivity.h"

BEGIN_BOOMER_NAMESPACE_EX(graph)

Connectivity::Connectivity(Container& graph)
{
    createNodes(graph);
    createConnections();
}

Connectivity::~Connectivity()
{
    m_nodes.clearPtr();
}

void Connectivity::resetDepths()
{
    // reset the depths and build list of nodes to visit
    for (auto node  : m_nodes)
    {
        node->m_depth = -1;
        node->m_island = -1;
        node->m_marker = 0;
    }
}

int Connectivity::followOutputLinks(Node* node, int depth)
{
    // recursive cycle found
    if (node->m_marker)
    {
        // TODO: more error!
        TRACE_ERROR("Graph cycle found");
        return 0;
    }

    // mark node as "in visit" to detect cycles
    node->m_marker = true;

    // should we descend ?
    if (node->m_depth < depth)
    {
        // update node depth of necessary
        if (depth > node->m_depth)
            node->m_depth = depth;

        // follow the inputs
        for (auto& edge : node->m_inputEdges)
        {
            auto childDepth = followOutputLinks(edge.m_target, node->m_depth + 1);
            node->m_depth = std::max<int>(node->m_depth, childDepth - 1);
        }
    }

    // node is no longer visited
    node->m_marker = false;
    return node->m_depth;
}

void Connectivity::buildReachabilityList(const Array<Block*>& outputList, Array<Block*>& outList)
{
    // reset helper data
    resetDepths();

    // flood fill from the provided target output blocks
    for (auto outputBlock  : outputList)
    {
        Node* node = nullptr;
        if (m_nodeMap.find(outputBlock, node))
        {
            followOutputLinks(node, 0);
        }
    }

    // count blocks that have proper depth set
    uint32_t numBlocks = 0;
    uint32_t maxDepth = 0;
    for (auto node  : m_nodes)
    {
        if (node->m_depth != -1)
        {
            maxDepth = std::max<uint32_t>(maxDepth, node->m_depth);
            numBlocks += 1;
        }
    }

    // extract blocks
    Array<Node*> tempNodes;
    tempNodes.reserve(numBlocks);
    for (auto node  : m_nodes)
    {
        if (node->m_depth != -1)
            tempNodes.pushBack(node);
    }

    // sort blocks by the depth value so we have a chain
    std::sort(tempNodes.begin(), tempNodes.end(), [](const Node* a, const Node* b) { return a->m_depth > b->m_depth; });

    // extract the actual block pointers
    outList.reserve(numBlocks);
    for (auto node  : tempNodes)
        outList.pushBack(node->m_block);

    // stuff extracted
    TRACE_INFO("Extracted {} blocks out of {} starting from {} outputs, largest depth {}", outList.size(), m_nodes.size(), outputList.size(), maxDepth);
}

void Connectivity::createNodes(Container& graph)
{
    // preallocate memory in the containers
    m_nodes.reserve(graph.blocks().size());
    m_nodeMap.reserve(graph.blocks().size());

    // create a block map
    for (auto& block : graph.blocks())
    {
        // create node mapping
        auto node = new Node(block.get());
        m_nodes.pushBack(node);
        m_nodeMap.set(block.get(), node);

        // make sure the block has valid layout
        //node->block->
    }
}

void Connectivity::resolveConnection(const Connection* con, const Socket* source, Array<Edge>& outEdges)
{
    if (source)
    {
        if (auto other = con->otherSocket(source))
        {
            auto& edge = outEdges.emplaceBack();
            edge.m_sourceName = source->name();
            edge.m_targetName = other->name();

            auto targetBlock = other->block();
            m_nodeMap.find(targetBlock, edge.m_target);
            ASSERT_EX(edge.m_target != nullptr, "Connection uses a block that is not in graph");
        }
    }
}

void Connectivity::createConnections()
{
    uint32_t numConnections = 0;
    for (auto node  : m_nodes)
    {
        // get the block layout
        auto& layout = node->m_block;

        // follow the sockets
        for (auto& socket : layout->sockets())
        {
            if (socket->info().m_direction == SocketDirection::Input)
            {
                // resolve the target node
                auto &connections = socket->connections();
                for (auto &con : connections)
                {
                    resolveConnection(con, socket, node->m_inputEdges);
                    numConnections += 1;
                }
            }
            else if (socket->info().m_direction == SocketDirection::Output)
            {
                // resolve the target node
                auto &connections = socket->connections();
                for (auto &con : connections)
                {
                    resolveConnection(con, socket, node->m_outputEdges);
                    numConnections += 1;
                }
            }
        }
    }

    TRACE_INFO("Collected {} connections for connectivity", numConnections);
}

END_BOOMER_NAMESPACE_EX(graph)
