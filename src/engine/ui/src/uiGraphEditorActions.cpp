/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
*
***/

#include "build.h"
#include "uiRenderer.h"
#include "uiInputAction.h"
#include "uiGraphEditor.h"
#include "uiGraphEditorNode.h"
#include "core/object/include/action.h"
#include "core/graph/include/graphBlock.h"
#include "core/graph/include/graphSocket.h"
#include "core/graph/include/graphConnection.h"
#include "core/object/include/actionHistory.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

class GraphEditorChangeSelectionAction : public IAction
{
public:
    GraphEditorChangeSelectionAction(GraphEditor* editor, const Array<const graph::Block*>& oldSelection, const Array<const graph::Block*>& newSelection)
        : m_editor(editor)
    {
        m_oldSelection.reserve(oldSelection.size());
        for (const auto* ptr : oldSelection)
            m_oldSelection.emplaceBack(AddRef(ptr));

        m_newSelection.reserve(newSelection.size());
        for (const auto* ptr : newSelection)
            m_newSelection.emplaceBack(AddRef(ptr));
    }

    virtual StringBuf description() const override
    {
        if (m_newSelection.empty())
            return StringBuf("Clear selection");

        if (m_newSelection.size() == 1)
            return TempString("Select '{}'", m_newSelection[0]->chooseTitle());

        return TempString("Select {} blocks", m_newSelection.size());
    }

    virtual bool execute() override
    {
        if (auto editor = m_editor.lock())
            editor->applySelection(m_newSelection);
        return true;
    }

    virtual bool undo() override
    {
        if (auto editor = m_editor.lock())
            editor->applySelection(m_oldSelection);
        return true;
    }

private:
    GraphEditorWeakPtr m_editor;
    Array<graph::BlockPtr> m_oldSelection;
    Array<graph::BlockPtr> m_newSelection;
};

bool GraphEditor::actionChangeSelection(const Array<const graph::Block*>& oldSelection, const Array<const graph::Block*>& newSelection)
{
    auto action = RefNew<GraphEditorChangeSelectionAction>(this, oldSelection, newSelection);
    return m_actionHistory->execute(action);
}

//--

GraphConnectionTarget::GraphConnectionTarget()
{}

GraphConnectionTarget::GraphConnectionTarget(const graph::Socket* source)
{
    if (source)
    {
        block = source->block();
        name = source->name();
    }
}

graph::Socket* GraphConnectionTarget::resolve() const
{
    if (block)
        return block->findSocket(name);
    return nullptr;
}

//--

GraphConnection::GraphConnection()
{}

static bool SocketOrder(const graph::Socket* sourceSocket, const graph::Socket* destinationSocket)
{
    return sourceSocket < destinationSocket;
}

static bool SocketOrder(const graph::Connection* con)
{
    return false;//
}

GraphConnection::GraphConnection(const graph::Connection* con)
    : source(SocketOrder(con) ? con->first() : con->second())
    , target(SocketOrder(con) ? con->second() : con->first())
{
}

GraphConnection::GraphConnection(const graph::Socket* sourceSocket, const graph::Socket* destinationSocket)
    : source(SocketOrder(sourceSocket, destinationSocket) ? sourceSocket : destinationSocket)
    , target(SocketOrder(sourceSocket, destinationSocket) ? destinationSocket : sourceSocket)
{
}

void GraphEditor::applyConnections(const Array<GraphConnection>& connectionsToRemove, const Array<GraphConnection>& connectionsToAdd)
{
    // TODO: transaction ?

    //m_graph->beginConnectionsChange();

    for (const auto& con : connectionsToRemove)
    {
        auto* sourceSocket = con.source.resolve();
        auto* targetSocket = con.target.resolve();
        if (sourceSocket && targetSocket)
            sourceSocket->removeAllConnectionsToSocket(targetSocket);
    }

    for (const auto& con : connectionsToAdd)
    {
        auto* sourceSocket = con.source.resolve();
        auto* targetSocket = con.target.resolve();
        if (sourceSocket && targetSocket)
            sourceSocket->connectTo(targetSocket);
    }

    //m_graph->endConnectionsChange();
}

class GraphEditorChangeConnectionsAction : public IAction
{
public:
    GraphEditorChangeConnectionsAction(GraphEditor* editor, const Array<GraphConnection>& oldConnections, const Array<GraphConnection>& newConnections, const StringBuf& desc)
        : m_editor(editor)
        , m_oldConnections(oldConnections)
        , m_newConnections(newConnections)
        , m_description(desc)
    {
    }

    virtual StringBuf description() const override
    {
        return m_description;
    }

    virtual bool execute() override
    {
        if (auto editor = m_editor.lock())
            editor->applyConnections(m_oldConnections, m_newConnections);
        return true;
    }

    virtual bool undo() override
    {
        if (auto editor = m_editor.lock())
            editor->applyConnections(m_newConnections, m_oldConnections);
        return true;
    }

private:
    GraphEditorWeakPtr m_editor;
    StringBuf m_description;
    Array<GraphConnection> m_oldConnections;
    Array<GraphConnection> m_newConnections;
};

//--

bool GraphEditor::actionConnectSockets(graph::Socket* source, graph::Socket* target)
{
    if (source->hasConnectionsToSocket(target) || target->hasConnectionsToSocket(source))
        return true;

    InplaceArray<graph::Connection*, 20> additionalRemovedConnections;
    if (!graph::Socket::CanConnect(*source, *target, &additionalRemovedConnections))
        return false;

    InplaceArray<GraphConnection, 20> oldConnections;
    for (const auto* con : additionalRemovedConnections)
        oldConnections.emplaceBack(con);

    InplaceArray<GraphConnection, 1> newConnections;
    newConnections.emplaceBack(source, target);

    auto desc = StringBuf(TempString("Connect '{}' with '{}'", source->name(), target->name()));
    auto action = RefNew<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
    return actionHistory()->execute(action);
}

bool GraphEditor::actionRemoveBlockConnections(graph::Block* block)
{
    if (!block->hasConnections())
        return true;

    InplaceArray<GraphConnection, 20> oldConnections;
    for (const auto& socket : block->sockets())
        for (const auto& con : socket->connections())
            oldConnections.pushBackUnique(GraphConnection(con));

    InplaceArray<GraphConnection, 1> newConnections; // nothing

    auto desc = StringBuf(TempString("Remove all connection of block '{}'", block->chooseTitle()));
    auto action = RefNew<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
    return actionHistory()->execute(action);
}

bool GraphEditor::actionRemoveSocketConnection(graph::Socket* socket, graph::Socket* target)
{
    InplaceArray<GraphConnection, 20> oldConnections;
    InplaceArray<GraphConnection, 20> newConnections;

    for (const auto& con : socket->connections())
    {
        oldConnections.emplaceBack(con);

        if (!con->match(socket, target))
            newConnections.emplaceBack(con);
    }

    if (oldConnections.size() == newConnections.size() || oldConnections.empty())
        return true;

    auto desc = StringBuf(TempString("Remove socket connection to '{}'", target->name()));
    auto action = RefNew<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
    return actionHistory()->execute(action);
}

bool GraphEditor::actionRemoveSocketConnections(graph::Socket* socket)
{
    if (!socket->hasConnections())
        return true;

    InplaceArray<GraphConnection, 20> oldConnections;
    for (const auto& con : socket->connections())
        oldConnections.emplaceBack(con);

    InplaceArray<GraphConnection, 1> newConnections; // nothing

    auto desc = StringBuf(TempString("Remove all connection of socket '{}'", socket->name()));
    auto action = RefNew<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
    return actionHistory()->execute(action);
}

bool GraphEditor::actionCopySocketConnections(graph::Socket* socket)
{
    m_connectionClipboard.reset();
    for (const auto& con : socket->connections())
        if (auto other = con->otherSocket(socket))
            m_connectionClipboard.emplaceBack(other);
    return true;
}

bool GraphEditor::actionCutSocketConnections(graph::Socket* socket)
{
    InplaceArray<GraphConnection, 1> oldConnections;

    m_connectionClipboard.reset();
    for (const auto& con : socket->connections())
    {
        oldConnections.emplaceBack(con);
        if (auto other = con->otherSocket(socket))
            m_connectionClipboard.emplaceBack(other);
    }

    if (m_connectionClipboard.empty())
        return true;

    InplaceArray<GraphConnection, 1> newConnections; // nothing

    auto desc = StringBuf(TempString("Cut all connection of socket '{}'", socket->name()));
    auto action = RefNew<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
    return actionHistory()->execute(action);
}

bool GraphEditor::testPasteSocketConnections(const graph::Socket* socket) const
{
    if (!socket)
        return false;

    if (m_connectionClipboard.empty())
        return true;

    InplaceArray<graph::Connection*, 10> removedConnections;
    for (const auto& con : socket->connections())
        removedConnections.emplaceBack(con);

    for (const auto& con : m_connectionClipboard)
        if (auto* target = con.resolve())
            if (graph::Socket::CanConnect(*socket, *target, &removedConnections))
                return true;

    return false; // no connections can be pasted
}

bool GraphEditor::actionPasteSocketConnections(graph::Socket* socket)
{
    if (m_connectionClipboard.empty())
        return true;

    InplaceArray<GraphConnection, 10> oldConnections;
    InplaceArray<graph::Connection*, 10> removedConnections;

    for (const auto& con : socket->connections())
    {
        removedConnections.emplaceBack(con);
        oldConnections.emplaceBack(con);
    }

    InplaceArray<GraphConnection, 10> newConnections;
    for (const auto& con : m_connectionClipboard)
        if (auto* target = con.resolve())
            if (graph::Socket::CanConnect(*socket, *target, &removedConnections))
                newConnections.emplaceBack(socket, target);

    auto desc = StringBuf(TempString("Paste connections to socket '{}'", socket->name()));
    auto action = RefNew<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
    return actionHistory()->execute(action);
}

bool GraphEditor::testMergeSocketConnections(const graph::Socket* socket) const
{
    if (!socket)
        return false;

    if (m_connectionClipboard.empty())
        return true;

    for (const auto& con : m_connectionClipboard)
        if (auto* target = con.resolve())
            if (graph::Socket::CanConnect(*socket, *target))
                return true;

    return false; // no connections can be merged
}

bool GraphEditor::actionMergeSocketConnections(graph::Socket* socket)
{
    if (m_connectionClipboard.empty())
        return true;

    InplaceArray<GraphConnection, 10> oldConnections;
    InplaceArray<GraphConnection, 10> newConnections;
    for (const auto& con : socket->connections())
    {
        oldConnections.emplaceBack(con);
        newConnections.emplaceBack(con);
    }

    bool newConnectionsMerged = false;
    for (const auto& con : m_connectionClipboard)
        if (auto* target = con.resolve())
            if (graph::Socket::CanConnect(*socket, *target))
                newConnections.emplaceBack(socket, target);

    if (newConnections.size() == oldConnections.size())
        return true; // nothing new added

    auto desc = StringBuf(TempString("Merge connections into socket '{}'", socket->name()));
    auto action = RefNew<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
    return actionHistory()->execute(action);
}

//--

void GraphEditor::applyObjects(const Array<graph::BlockPtr>& blocksToRemove, const Array<graph::BlockPtr>& blocksToAdd)
{
    for (const auto& block : blocksToRemove)
    {
        DEBUG_CHECK_EX(!block->hasConnections(), "Removing block with connections");
        m_graph->removeBlock(block);
    }

    for (const auto& block : blocksToAdd)
    {
        DEBUG_CHECK_EX(!block->hasConnections(), "Adding block with connections");
        m_graph->addBlock(block);
    }
}

class GraphEditorCreateBlock : public IAction
{
public:
    GraphEditorCreateBlock(GraphEditor* editor, const Array<graph::BlockPtr>& oldSelection, const graph::BlockPtr& block)
        : m_editor(editor)
        , m_oldSelection(oldSelection)
    {
        m_desc = TempString("Create '{}'", block->chooseTitle());
        m_newBlocks.pushBack(block);
    }

    virtual StringBuf description() const override
    {
        return m_desc;            
    }

    virtual bool execute() override
    {
        if (auto editor = m_editor.lock())
        {
            Array<graph::BlockPtr> empty;
            editor->applyObjects(empty, m_newBlocks);
            editor->applySelection(m_newBlocks);
        }
        return true;
    }

    virtual bool undo() override
    {
        if (auto editor = m_editor.lock())
        {
            Array<graph::BlockPtr> empty;
            editor->applySelection(m_oldSelection);
            editor->applyObjects(m_newBlocks, empty);
        }
        return true;
    }

private:
    GraphEditorWeakPtr m_editor;
    StringBuf m_desc;
    Array<graph::BlockPtr> m_newBlocks;
    Array<graph::BlockPtr> m_oldSelection;
};

Array<graph::BlockPtr> GraphEditor::currentSelection() const
{
    Array<graph::BlockPtr> ret;

    enumSelectedElements([this, &ret](VirtualAreaElement* elem)
        {
            if (const auto* node = rtti_cast<GraphEditorBlockNode>(elem))
                ret.pushBack(node->block());
            return false;
        });


    return ret;
}

bool GraphEditor::actionCreateBlock(ClassType blockClass, const VirtualPosition& virtualPos)
{
    if (!m_graph->canAddBlockOfClass(blockClass))
        return false;

    auto block = blockClass->create<graph::Block>();
    block->position(virtualPos);

    auto action = RefNew<GraphEditorCreateBlock>(this, currentSelection(), block);
    return actionHistory()->execute(action);
}

bool GraphEditor::actionCreateConnectedBlock(ClassType blockClass, StringID blockSocket, const VirtualPosition& virtualPos, const graph::Block* otherBlock, StringID otherBlockSocket)
{
    if (!m_graph->canAddBlockOfClass(blockClass))
        return false;

    auto block = blockClass->create<graph::Block>();
    block->position(virtualPos);

    auto action = RefNew<GraphEditorCreateBlock>(this, currentSelection(), block);
    if (!actionHistory()->execute(action))
        return false;

    if (otherBlock)
    {
        auto* sourceSocket = otherBlock->findSocket(otherBlockSocket);
        auto* targetSocket = block->findSocket(blockSocket);
        if (sourceSocket && targetSocket)
        {
            return actionConnectSockets(sourceSocket, targetSocket);
        }
    }

    return true;
}

//--

class GraphEditorDeleteBlocks : public IAction
{
public:
    GraphEditorDeleteBlocks(GraphEditor* editor, const Array<graph::BlockPtr>& oldSelection, const Array<graph::BlockPtr>& newSelection, const Array<graph::BlockPtr>& blocksToRemove, const Array<GraphConnection>& oldConnections, const StringBuf& desc)
        : m_editor(editor)
        , m_oldSelection(oldSelection)
        , m_oldConnections(oldConnections)
        , m_newSelection(newSelection)
        , m_deletedBlocks(blocksToRemove)
        , m_desc(desc)
    {
    }

    virtual StringBuf description() const override
    {
        return m_desc;
    }

    virtual bool execute() override
    {
        if (auto editor = m_editor.lock())
        {
            Array<graph::BlockPtr> empty;
            Array<GraphConnection> emptyConnections;
            editor->applyConnections(m_oldConnections, emptyConnections);
            editor->applyObjects(m_deletedBlocks, empty);
            editor->applySelection(m_newSelection);
        }
        return true;
    }

    virtual bool undo() override
    {
        if (auto editor = m_editor.lock())
        {
            Array<graph::BlockPtr> empty;
            Array<GraphConnection> emptyConnections;
            editor->applyObjects(empty, m_deletedBlocks);
            editor->applyConnections(emptyConnections, m_oldConnections);
            editor->applySelection(m_oldSelection);
        }
        return true;
    }

private:
    GraphEditorWeakPtr m_editor;
    StringBuf m_desc;
    Array<graph::BlockPtr> m_deletedBlocks;
    Array<graph::BlockPtr> m_oldSelection;
    Array<graph::BlockPtr> m_newSelection;
    Array<GraphConnection> m_oldConnections;
};

bool GraphEditor::actionRemoveBlocks(const Array<graph::BlockPtr>& blocks, StringView desc)
{
    if (blocks.empty())
        return true;

    HashSet<graph::Block*> blocksToDeleteSet;
    blocksToDeleteSet.reserve(blocks.size());
    for (const auto& ptr : blocks)
        blocksToDeleteSet.insert(ptr);

    auto oldSelection = currentSelection();
    Array<graph::BlockPtr> newSelection;
    for (const auto& ptr : oldSelection)
        if (!blocksToDeleteSet.contains(ptr))
            newSelection.pushBack(ptr);

    InplaceArray<GraphConnection, 256> oldConnections;
    for (auto* block : blocksToDeleteSet.keys())
    {
        for (const auto& socket : block->sockets())
        {
            for (const auto& con : socket->connections())
            {
                oldConnections.pushBackUnique(GraphConnection(con));
            }
        }
    }

    auto action = RefNew<GraphEditorDeleteBlocks>(this, oldSelection, newSelection, blocks, oldConnections, StringBuf(desc));
    return actionHistory()->execute(action);
}

bool GraphEditor::actionRemoveBlocks(const Array<graph::Block*>& blocks, StringView desc)
{
    if (blocks.empty())
        return true;

    InplaceArray<graph::BlockPtr, 32> blockPtrs;
    for (auto* block : blocks)
        if (block)
            blockPtrs.emplaceBack(AddRef(block));

    return actionRemoveBlocks(blockPtrs, desc);
}

bool GraphEditor::actionDeleteSelection()
{
    return actionRemoveBlocks(currentSelection(), "Delete selection");
}

bool GraphEditor::actionCutSelection()
{
    if (!m_selection.empty())
    {
        if (auto subGraph = m_graph->extractSubGraph(currentSelection()))
        {
            clipboard().storeObject(subGraph);
            return actionRemoveBlocks(currentSelection(), "Cut selection");
        }
    }

    return false;
}

bool GraphEditor::actionCopySelection()
{
    if (!m_selection.empty())
    {
        if (auto subGraph = m_graph->extractSubGraph(currentSelection()))
        {
            clipboard().storeObject(subGraph);
            return true;
        }
    }

    return false;
}

//--

static void DisassembleContainer(graph::Container& container, Array<graph::BlockPtr>& outBlocks, Array<GraphConnection>& outConnections)
{
    auto blocks = container.blocks();
    for (const auto& block : blocks)
        for (const auto& socket : block->sockets())
            for (const auto& con : socket->connections())
                outConnections.pushBackUnique(GraphConnection(con));

    for (const auto& block : blocks)
        block->breakAllConnections();

    for (const auto& block : blocks)
    {
        container.removeBlock(block);
        DEBUG_CHECK(!block->parent());
        DEBUG_CHECK(!block->hasConnections());
        outBlocks.pushBack(block);
    }
}

class GraphEditorPasteBlocks : public IAction
{
public:
    GraphEditorPasteBlocks(GraphEditor* editor, const Array<graph::BlockPtr>& oldSelection, const Array<graph::BlockPtr>& blocksToPaste, const Array<GraphConnection>& newConnections, const StringBuf& desc)
        : m_editor(editor)
        , m_oldSelection(oldSelection)
        , m_pastedBlocks(blocksToPaste)
        , m_newConnections(newConnections)
        , m_desc(desc)
    {
    }

    virtual StringBuf description() const override
    {
        return m_desc;
    }

    virtual bool execute() override
    {
        if (auto editor = m_editor.lock())
        {
            Array<graph::BlockPtr> empty;
            Array<GraphConnection> emptyConnections;
            editor->applyObjects(empty, m_pastedBlocks);
            editor->applyConnections(emptyConnections, m_newConnections);
            editor->applySelection(m_pastedBlocks);
        }
        return true;
    }

    virtual bool undo() override
    {
        if (auto editor = m_editor.lock())
        {
            Array<graph::BlockPtr> empty;
            Array<GraphConnection> emptyConnections;
            editor->applySelection(m_oldSelection);
            editor->applyConnections(m_newConnections, emptyConnections);
            editor->applyObjects(m_pastedBlocks, empty);
        }
        return true;
    }

private:
    GraphEditorWeakPtr m_editor;
    StringBuf m_desc;
    Array<graph::BlockPtr> m_pastedBlocks;
    Array<graph::BlockPtr> m_oldSelection;
    Array<graph::BlockPtr> m_newSelection;
    Array<GraphConnection> m_newConnections;
};

bool GraphEditor::actionPasteSelection(bool useSpecificVirtualPos, const VirtualPosition& virtualPos)
{
    if (auto graph = clipboard().loadObject<graph::Container>())
    {
        InplaceArray<graph::BlockPtr, 256> pastedBlocks;
        InplaceArray<GraphConnection, 512> pastedConnections;
        DisassembleContainer(*graph, pastedBlocks, pastedConnections);
        TRACE_INFO("Extracted '{}' blocks and '{}' connections from clipboard data", pastedBlocks.size(), pastedConnections.size());

        if (!pastedBlocks.empty())
        {
            auto oldSelection = currentSelection();

            auto desc = StringBuf(TempString("Paste {} block(s)", pastedBlocks.size()));
            auto action = RefNew<GraphEditorPasteBlocks>(this, oldSelection, pastedBlocks, pastedConnections, desc);
            return actionHistory()->execute(action);
        }
    }

    return false;
}

//--

bool GraphEditor::actionChangeSelection(const Array<const VirtualAreaElement*>& oldSelection, const Array<const VirtualAreaElement*>& newSelection)
{
    InplaceArray<const graph::Block*, 256> oldBlocks, newBlocks;
    oldBlocks.reserve(oldSelection.size());
    newBlocks.reserve(newSelection.size());

    for (const auto* elem : oldSelection)
        if (const auto* node = rtti_cast<GraphEditorBlockNode>(elem))
            oldBlocks.pushBack(node->block());

    for (const auto* elem : newSelection)
        if (const auto* node = rtti_cast<GraphEditorBlockNode>(elem))
            newBlocks.pushBack(node->block());

    return actionChangeSelection(oldBlocks, newBlocks);
}

void GraphEditor::applySelection(const Array<graph::BlockPtr>& selection, bool postEvent /*= true*/)
{
    InplaceArray<const VirtualAreaElement*, 256> selectedNodes;
    selectedNodes.reserve(selection.size());

    for (const auto& block : selection)
        if (auto node = m_nodesMap.findSafe(block, nullptr))
            selectedNodes.pushBack(node);

    VirtualArea::applySelection(selectedNodes, postEvent);
}

//--

class GraphEditorMoveBlocks : public IAction
{
public:
    GraphEditorMoveBlocks(GraphEditor* editor, const Array<GraphNodePlacement>& oldPlacement, const Array<GraphNodePlacement>& newPlacement)
        : m_editor(editor)
        , m_oldPlacement(oldPlacement)
        , m_newPlacement(newPlacement)
    {
    }

    virtual StringBuf description() const override
    {
        return TempString("Move {} block(s)", m_oldPlacement.size());
    }

    virtual bool execute() override
    {
        if (auto editor = m_editor.lock())
            editor->applyPlacement(m_newPlacement);
        return true;
    }

    virtual bool undo() override
    {
        if (auto editor = m_editor.lock())
            editor->applyPlacement(m_oldPlacement);
        return true;
    }

private:
    GraphEditorWeakPtr m_editor;
    Array<GraphNodePlacement> m_oldPlacement;
    Array<GraphNodePlacement>  m_newPlacement;
};

void GraphEditor::applyPlacement(const Array<GraphNodePlacement>& placement)
{
    InplaceArray<VirtualAreaElementPositionState, 256> nodePlacement;
    nodePlacement.reserve(placement.size());

    for (const auto& entry : placement)
    {
        if (auto node = m_nodesMap.findSafe(entry.block, nullptr))
        {
            auto& nodeEntry = nodePlacement.emplaceBack();
            nodeEntry.elem = node;
            nodeEntry.position = entry.position;
        }
    }

    applyMove(nodePlacement);
}
    
bool GraphEditor::actionMoveElements(const Array<VirtualAreaElementPositionState>& oldPositions, const Array<VirtualAreaElementPositionState>& newPositions)
{
    InplaceArray<GraphNodePlacement, 256> oldPlacement;
    for (const auto& entry : oldPositions)
    {
        if (auto node = rtti_cast<GraphEditorBlockNode>(entry.elem))
        {
            auto& placementEntry = oldPlacement.emplaceBack();
            placementEntry.block = node->block();
            placementEntry.position = entry.position;
        }
    }

    InplaceArray<GraphNodePlacement, 256> newPlacement;
    for (const auto& entry : newPositions)
    {
        if (auto node = rtti_cast<GraphEditorBlockNode>(entry.elem))
        {
            auto& placementEntry = newPlacement.emplaceBack();
            placementEntry.block = node->block();
            placementEntry.position = entry.position;
        }
    }

    auto action = RefNew<GraphEditorMoveBlocks>(this, oldPlacement, newPlacement);
    return m_actionHistory->execute(action);
}

//--

END_BOOMER_NAMESPACE_EX(ui)
