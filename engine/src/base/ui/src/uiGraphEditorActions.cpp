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
#include "base/object/include/action.h"
#include "base/graph/include/graphBlock.h"
#include "base/graph/include/graphSocket.h"
#include "base/graph/include/graphConnection.h"
#include "base/object/include/actionHistory.h"

namespace ui
{
    //--

    class GraphEditorChangeSelectionAction : public base::IAction
    {
    public:
        GraphEditorChangeSelectionAction(GraphEditor* editor, const base::Array<const base::graph::Block*>& oldSelection, const base::Array<const base::graph::Block*>& newSelection)
            : m_editor(editor)
        {
            m_oldSelection.reserve(oldSelection.size());
            for (const auto* ptr : oldSelection)
                m_oldSelection.emplaceBack(AddRef(ptr));

            m_newSelection.reserve(newSelection.size());
            for (const auto* ptr : newSelection)
                m_newSelection.emplaceBack(AddRef(ptr));
        }

        virtual base::StringBuf description() const override
        {
            if (m_newSelection.empty())
                return base::StringBuf("Clear selection");

            if (m_newSelection.size() == 1)
                return base::TempString("Select '{}'", m_newSelection[0]->chooseTitle());

            return base::TempString("Select {} blocks", m_newSelection.size());
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
        base::Array<base::graph::BlockPtr> m_oldSelection;
        base::Array<base::graph::BlockPtr> m_newSelection;
    };

    bool GraphEditor::actionChangeSelection(const base::Array<const base::graph::Block*>& oldSelection, const base::Array<const base::graph::Block*>& newSelection)
    {
        auto action = base::CreateSharedPtr<GraphEditorChangeSelectionAction>(this, oldSelection, newSelection);
        return m_actionHistory->execute(action);
    }

    //--

    GraphConnectionTarget::GraphConnectionTarget()
    {}

    GraphConnectionTarget::GraphConnectionTarget(const base::graph::Socket* source)
    {
        if (source)
        {
            block = source->block();
            name = source->name();
        }
    }

    base::graph::Socket* GraphConnectionTarget::resolve() const
    {
        if (block)
            return block->findSocket(name);
        return nullptr;
    }

    //--

    GraphConnection::GraphConnection()
    {}

    static bool SocketOrder(const base::graph::Socket* sourceSocket, const base::graph::Socket* destinationSocket)
    {
        return sourceSocket < destinationSocket;
    }

    static bool SocketOrder(const base::graph::Connection* con)
    {
        return false;//
    }

    GraphConnection::GraphConnection(const base::graph::Connection* con)
        : source(SocketOrder(con) ? con->first() : con->second())
        , target(SocketOrder(con) ? con->second() : con->first())
    {
    }

    GraphConnection::GraphConnection(const base::graph::Socket* sourceSocket, const base::graph::Socket* destinationSocket)
        : source(SocketOrder(sourceSocket, destinationSocket) ? sourceSocket : destinationSocket)
        , target(SocketOrder(sourceSocket, destinationSocket) ? destinationSocket : sourceSocket)
    {
    }

    void GraphEditor::applyConnections(const base::Array<GraphConnection>& connectionsToRemove, const base::Array<GraphConnection>& connectionsToAdd)
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

    class GraphEditorChangeConnectionsAction : public base::IAction
    {
    public:
        GraphEditorChangeConnectionsAction(GraphEditor* editor, const base::Array<GraphConnection>& oldConnections, const base::Array<GraphConnection>& newConnections, const base::StringBuf& desc)
            : m_editor(editor)
            , m_oldConnections(oldConnections)
            , m_newConnections(newConnections)
            , m_description(desc)
        {
        }

        virtual base::StringBuf description() const override
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
        base::StringBuf m_description;
        base::Array<GraphConnection> m_oldConnections;
        base::Array<GraphConnection> m_newConnections;
    };

    //--

    bool GraphEditor::actionConnectSockets(base::graph::Socket* source, base::graph::Socket* target)
    {
        if (source->hasConnectionsToSocket(target) || target->hasConnectionsToSocket(source))
            return true;

        base::InplaceArray<base::graph::Connection*, 20> additionalRemovedConnections;
        if (!base::graph::Socket::CanConnect(*source, *target, &additionalRemovedConnections))
            return false;

        base::InplaceArray<GraphConnection, 20> oldConnections;
        for (const auto* con : additionalRemovedConnections)
            oldConnections.emplaceBack(con);

        base::InplaceArray<GraphConnection, 1> newConnections;
        newConnections.emplaceBack(source, target);

        auto desc = base::StringBuf(base::TempString("Connect '{}' with '{}'", source->name(), target->name()));
        auto action = base::CreateSharedPtr<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
        return actionHistory()->execute(action);
    }

    bool GraphEditor::actionRemoveBlockConnections(base::graph::Block* block)
    {
        if (!block->hasConnections())
            return true;

        base::InplaceArray<GraphConnection, 20> oldConnections;
        for (const auto& socket : block->sockets())
            for (const auto& con : socket->connections())
                oldConnections.pushBackUnique(GraphConnection(con));

        base::InplaceArray<GraphConnection, 1> newConnections; // nothing

        auto desc = base::StringBuf(base::TempString("Remove all connection of block '{}'", block->chooseTitle()));
        auto action = base::CreateSharedPtr<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
        return actionHistory()->execute(action);
    }

    bool GraphEditor::actionRemoveSocketConnection(base::graph::Socket* socket, base::graph::Socket* target)
    {
        base::InplaceArray<GraphConnection, 20> oldConnections;
        base::InplaceArray<GraphConnection, 20> newConnections;

        for (const auto& con : socket->connections())
        {
            oldConnections.emplaceBack(con);

            if (!con->match(socket, target))
                newConnections.emplaceBack(con);
        }

        if (oldConnections.size() == newConnections.size() || oldConnections.empty())
            return true;

        auto desc = base::StringBuf(base::TempString("Remove socket connection to '{}'", target->name()));
        auto action = base::CreateSharedPtr<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
        return actionHistory()->execute(action);
    }

    bool GraphEditor::actionRemoveSocketConnections(base::graph::Socket* socket)
    {
        if (!socket->hasConnections())
            return true;

        base::InplaceArray<GraphConnection, 20> oldConnections;
        for (const auto& con : socket->connections())
            oldConnections.emplaceBack(con);

        base::InplaceArray<GraphConnection, 1> newConnections; // nothing

        auto desc = base::StringBuf(base::TempString("Remove all connection of socket '{}'", socket->name()));
        auto action = base::CreateSharedPtr<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
        return actionHistory()->execute(action);
    }

    bool GraphEditor::actionCopySocketConnections(base::graph::Socket* socket)
    {
        m_connectionClipboard.reset();
        for (const auto& con : socket->connections())
            if (auto other = con->otherSocket(socket))
                m_connectionClipboard.emplaceBack(other);
        return true;
    }

    bool GraphEditor::actionCutSocketConnections(base::graph::Socket* socket)
    {
        base::InplaceArray<GraphConnection, 1> oldConnections;

        m_connectionClipboard.reset();
        for (const auto& con : socket->connections())
        {
            oldConnections.emplaceBack(con);
            if (auto other = con->otherSocket(socket))
                m_connectionClipboard.emplaceBack(other);
        }

        if (m_connectionClipboard.empty())
            return true;

        base::InplaceArray<GraphConnection, 1> newConnections; // nothing

        auto desc = base::StringBuf(base::TempString("Cut all connection of socket '{}'", socket->name()));
        auto action = base::CreateSharedPtr<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
        return actionHistory()->execute(action);
    }

    bool GraphEditor::testPasteSocketConnections(const base::graph::Socket* socket) const
    {
        if (!socket)
            return false;

        if (m_connectionClipboard.empty())
            return true;

        base::InplaceArray<base::graph::Connection*, 10> removedConnections;
        for (const auto& con : socket->connections())
            removedConnections.emplaceBack(con);

        for (const auto& con : m_connectionClipboard)
            if (auto* target = con.resolve())
                if (base::graph::Socket::CanConnect(*socket, *target, &removedConnections))
                    return true;

        return false; // no connections can be pasted
    }

    bool GraphEditor::actionPasteSocketConnections(base::graph::Socket* socket)
    {
        if (m_connectionClipboard.empty())
            return true;

        base::InplaceArray<GraphConnection, 10> oldConnections;
        base::InplaceArray<base::graph::Connection*, 10> removedConnections;

        for (const auto& con : socket->connections())
        {
            removedConnections.emplaceBack(con);
            oldConnections.emplaceBack(con);
        }

        base::InplaceArray<GraphConnection, 10> newConnections;
        for (const auto& con : m_connectionClipboard)
            if (auto* target = con.resolve())
                if (base::graph::Socket::CanConnect(*socket, *target, &removedConnections))
                    newConnections.emplaceBack(socket, target);

        auto desc = base::StringBuf(base::TempString("Paste connections to socket '{}'", socket->name()));
        auto action = base::CreateSharedPtr<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
        return actionHistory()->execute(action);
    }

    bool GraphEditor::testMergeSocketConnections(const base::graph::Socket* socket) const
    {
        if (!socket)
            return false;

        if (m_connectionClipboard.empty())
            return true;

        for (const auto& con : m_connectionClipboard)
            if (auto* target = con.resolve())
                if (base::graph::Socket::CanConnect(*socket, *target))
                    return true;

        return false; // no connections can be merged
    }

    bool GraphEditor::actionMergeSocketConnections(base::graph::Socket* socket)
    {
        if (m_connectionClipboard.empty())
            return true;

        base::InplaceArray<GraphConnection, 10> oldConnections;
        base::InplaceArray<GraphConnection, 10> newConnections;
        for (const auto& con : socket->connections())
        {
            oldConnections.emplaceBack(con);
            newConnections.emplaceBack(con);
        }

        bool newConnectionsMerged = false;
        for (const auto& con : m_connectionClipboard)
            if (auto* target = con.resolve())
                if (base::graph::Socket::CanConnect(*socket, *target))
                    newConnections.emplaceBack(socket, target);

        if (newConnections.size() == oldConnections.size())
            return true; // nothing new added

        auto desc = base::StringBuf(base::TempString("Merge connections into socket '{}'", socket->name()));
        auto action = base::CreateSharedPtr<GraphEditorChangeConnectionsAction>(this, oldConnections, newConnections, desc);
        return actionHistory()->execute(action);
    }

    //--

    void GraphEditor::applyObjects(const base::Array<base::graph::BlockPtr>& blocksToRemove, const base::Array<base::graph::BlockPtr>& blocksToAdd)
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

    class GraphEditorCreateBlock : public base::IAction
    {
    public:
        GraphEditorCreateBlock(GraphEditor* editor, const base::Array<base::graph::BlockPtr>& oldSelection, const base::graph::BlockPtr& block)
            : m_editor(editor)
            , m_oldSelection(oldSelection)
        {
            m_desc = base::TempString("Create '{}'", block->chooseTitle());
            m_newBlocks.pushBack(block);
        }

        virtual base::StringBuf description() const override
        {
            return m_desc;            
        }

        virtual bool execute() override
        {
            if (auto editor = m_editor.lock())
            {
                base::Array<base::graph::BlockPtr> empty;
                editor->applyObjects(empty, m_newBlocks);
                editor->applySelection(m_newBlocks);
            }
            return true;
        }

        virtual bool undo() override
        {
            if (auto editor = m_editor.lock())
            {
                base::Array<base::graph::BlockPtr> empty;
                editor->applySelection(m_oldSelection);
                editor->applyObjects(m_newBlocks, empty);
            }
            return true;
        }

    private:
        GraphEditorWeakPtr m_editor;
        base::StringBuf m_desc;
        base::Array<base::graph::BlockPtr> m_newBlocks;
        base::Array<base::graph::BlockPtr> m_oldSelection;
    };

    base::Array<base::graph::BlockPtr> GraphEditor::currentSelection() const
    {
        base::Array<base::graph::BlockPtr> ret;

        enumSelectedElements([this, &ret](VirtualAreaElement* elem)
            {
                if (const auto* node = base::rtti_cast<GraphEditorBlockNode>(elem))
                    ret.pushBack(node->block());
                return false;
            });


        return ret;
    }

    bool GraphEditor::actionCreateBlock(base::ClassType blockClass, const VirtualPosition& virtualPos)
    {
        if (!m_graph->canAddBlockOfClass(blockClass))
            return false;

        auto block = blockClass->create<base::graph::Block>();
        block->position(virtualPos);

        auto action = base::CreateSharedPtr<GraphEditorCreateBlock>(this, currentSelection(), block);
        return actionHistory()->execute(action);
    }

    bool GraphEditor::actionCreateConnectedBlock(base::ClassType blockClass, base::StringID blockSocket, const VirtualPosition& virtualPos, const base::graph::Block* otherBlock, base::StringID otherBlockSocket)
    {
        if (!m_graph->canAddBlockOfClass(blockClass))
            return false;

        auto block = blockClass->create<base::graph::Block>();
        block->position(virtualPos);

        auto action = base::CreateSharedPtr<GraphEditorCreateBlock>(this, currentSelection(), block);
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

    class GraphEditorDeleteBlocks : public base::IAction
    {
    public:
        GraphEditorDeleteBlocks(GraphEditor* editor, const base::Array<base::graph::BlockPtr>& oldSelection, const base::Array<base::graph::BlockPtr>& newSelection, const base::Array<base::graph::BlockPtr>& blocksToRemove, const base::Array<GraphConnection>& oldConnections, const base::StringBuf& desc)
            : m_editor(editor)
            , m_oldSelection(oldSelection)
            , m_oldConnections(oldConnections)
            , m_newSelection(newSelection)
            , m_deletedBlocks(blocksToRemove)
            , m_desc(desc)
        {
        }

        virtual base::StringBuf description() const override
        {
            return m_desc;
        }

        virtual bool execute() override
        {
            if (auto editor = m_editor.lock())
            {
                base::Array<base::graph::BlockPtr> empty;
                base::Array<GraphConnection> emptyConnections;
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
                base::Array<base::graph::BlockPtr> empty;
                base::Array<GraphConnection> emptyConnections;
                editor->applyObjects(empty, m_deletedBlocks);
                editor->applyConnections(emptyConnections, m_oldConnections);
                editor->applySelection(m_oldSelection);
            }
            return true;
        }

    private:
        GraphEditorWeakPtr m_editor;
        base::StringBuf m_desc;
        base::Array<base::graph::BlockPtr> m_deletedBlocks;
        base::Array<base::graph::BlockPtr> m_oldSelection;
        base::Array<base::graph::BlockPtr> m_newSelection;
        base::Array<GraphConnection> m_oldConnections;
    };

    bool GraphEditor::actionRemoveBlocks(const base::Array<base::graph::BlockPtr>& blocks, base::StringView desc)
    {
        if (blocks.empty())
            return true;

        base::HashSet<base::graph::Block*> blocksToDeleteSet;
        blocksToDeleteSet.reserve(blocks.size());
        for (const auto& ptr : blocks)
            blocksToDeleteSet.insert(ptr);

        auto oldSelection = currentSelection();
        base::Array<base::graph::BlockPtr> newSelection;
        for (const auto& ptr : oldSelection)
            if (!blocksToDeleteSet.contains(ptr))
                newSelection.pushBack(ptr);

        base::InplaceArray<GraphConnection, 256> oldConnections;
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

        auto action = base::CreateSharedPtr<GraphEditorDeleteBlocks>(this, oldSelection, newSelection, blocks, oldConnections, base::StringBuf(desc));
        return actionHistory()->execute(action);
    }

    bool GraphEditor::actionRemoveBlocks(const base::Array<base::graph::Block*>& blocks, base::StringView desc)
    {
        if (blocks.empty())
            return true;

        base::InplaceArray<base::graph::BlockPtr, 32> blockPtrs;
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
        if (!renderer())
            return false;

        auto subGraph = m_graph->extractSubGraph(currentSelection());
        if (!subGraph)
            return false;

        if (!renderer()->storeObjectToClipboard(subGraph))
            return false;

        if (m_selection.empty())
            return true;

        return actionRemoveBlocks(currentSelection(), "Cut selection");
    }

    bool GraphEditor::actionCopySelection()
    {
        if (m_selection.empty())
            return true;

        if (!renderer())
            return false;

        auto subGraph = m_graph->extractSubGraph(currentSelection());
        if (!subGraph)
            return false;

        return renderer()->storeObjectToClipboard(subGraph);
    }

    //--

    static void DisassembleContainer(base::graph::Container& container, base::Array<base::graph::BlockPtr>& outBlocks, base::Array<GraphConnection>& outConnections)
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

    class GraphEditorPasteBlocks : public base::IAction
    {
    public:
        GraphEditorPasteBlocks(GraphEditor* editor, const base::Array<base::graph::BlockPtr>& oldSelection, const base::Array<base::graph::BlockPtr>& blocksToPaste, const base::Array<GraphConnection>& newConnections, const base::StringBuf& desc)
            : m_editor(editor)
            , m_oldSelection(oldSelection)
            , m_pastedBlocks(blocksToPaste)
            , m_newConnections(newConnections)
            , m_desc(desc)
        {
        }

        virtual base::StringBuf description() const override
        {
            return m_desc;
        }

        virtual bool execute() override
        {
            if (auto editor = m_editor.lock())
            {
                base::Array<base::graph::BlockPtr> empty;
                base::Array<GraphConnection> emptyConnections;
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
                base::Array<base::graph::BlockPtr> empty;
                base::Array<GraphConnection> emptyConnections;
                editor->applySelection(m_oldSelection);
                editor->applyConnections(m_newConnections, emptyConnections);
                editor->applyObjects(m_pastedBlocks, empty);
            }
            return true;
        }

    private:
        GraphEditorWeakPtr m_editor;
        base::StringBuf m_desc;
        base::Array<base::graph::BlockPtr> m_pastedBlocks;
        base::Array<base::graph::BlockPtr> m_oldSelection;
        base::Array<base::graph::BlockPtr> m_newSelection;
        base::Array<GraphConnection> m_newConnections;
    };

    bool GraphEditor::actionPasteSelection(bool useSpecificVirtualPos, const VirtualPosition& virtualPos)
    {
        if (!renderer())
            return false;

        base::ObjectPtr data;
        if (!renderer()->loadObjectFromClipboard(m_graph->cls(), data))
            return false;

        auto graph = base::rtti_cast<base::graph::Container>(data);
        if (!graph)
            return false;

        base::InplaceArray<base::graph::BlockPtr, 256> pastedBlocks;
        base::InplaceArray<GraphConnection, 512> pastedConnections;
        DisassembleContainer(*graph, pastedBlocks, pastedConnections);
        TRACE_INFO("Extracted '{}' blocks and '{}' connections from clipboard data", pastedBlocks.size(), pastedConnections.size());
        if (pastedBlocks.empty())
            return true;

        auto oldSelection = currentSelection();

        auto desc = base::StringBuf(base::TempString("Paste {} block(s)", pastedBlocks.size()));
        auto action = base::CreateSharedPtr<GraphEditorPasteBlocks>(this, oldSelection, pastedBlocks, pastedConnections, desc);
        return actionHistory()->execute(action);
    }

    //--

    bool GraphEditor::actionChangeSelection(const base::Array<const VirtualAreaElement*>& oldSelection, const base::Array<const VirtualAreaElement*>& newSelection)
    {
        base::InplaceArray<const base::graph::Block*, 256> oldBlocks, newBlocks;
        oldBlocks.reserve(oldSelection.size());
        newBlocks.reserve(newSelection.size());

        for (const auto* elem : oldSelection)
            if (const auto* node = base::rtti_cast<GraphEditorBlockNode>(elem))
                oldBlocks.pushBack(node->block());

        for (const auto* elem : newSelection)
            if (const auto* node = base::rtti_cast<GraphEditorBlockNode>(elem))
                newBlocks.pushBack(node->block());

        return actionChangeSelection(oldBlocks, newBlocks);
    }

    void GraphEditor::applySelection(const base::Array<base::graph::BlockPtr>& selection, bool postEvent /*= true*/)
    {
        base::InplaceArray<const VirtualAreaElement*, 256> selectedNodes;
        selectedNodes.reserve(selection.size());

        for (const auto& block : selection)
            if (auto node = m_nodesMap.findSafe(block, nullptr))
                selectedNodes.pushBack(node);

        VirtualArea::applySelection(selectedNodes, postEvent);
    }

    //--

    class GraphEditorMoveBlocks : public base::IAction
    {
    public:
        GraphEditorMoveBlocks(GraphEditor* editor, const base::Array<GraphNodePlacement>& oldPlacement, const base::Array<GraphNodePlacement>& newPlacement)
            : m_editor(editor)
            , m_oldPlacement(oldPlacement)
            , m_newPlacement(newPlacement)
        {
        }

        virtual base::StringBuf description() const override
        {
            return base::TempString("Move {} block(s)", m_oldPlacement.size());
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
        base::Array<GraphNodePlacement> m_oldPlacement;
        base::Array<GraphNodePlacement>  m_newPlacement;
    };

    void GraphEditor::applyPlacement(const base::Array<GraphNodePlacement>& placement)
    {
        base::InplaceArray<VirtualAreaElementPositionState, 256> nodePlacement;
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
    
    bool GraphEditor::actionMoveElements(const base::Array<VirtualAreaElementPositionState>& oldPositions, const base::Array<VirtualAreaElementPositionState>& newPositions)
    {
        base::InplaceArray<GraphNodePlacement, 256> oldPlacement;
        for (const auto& entry : oldPositions)
        {
            if (auto node = base::rtti_cast<GraphEditorBlockNode>(entry.elem))
            {
                auto& placementEntry = oldPlacement.emplaceBack();
                placementEntry.block = node->block();
                placementEntry.position = entry.position;
            }
        }

        base::InplaceArray<GraphNodePlacement, 256> newPlacement;
        for (const auto& entry : newPositions)
        {
            if (auto node = base::rtti_cast<GraphEditorBlockNode>(entry.elem))
            {
                auto& placementEntry = newPlacement.emplaceBack();
                placementEntry.block = node->block();
                placementEntry.position = entry.position;
            }
        }

        auto action = base::CreateSharedPtr<GraphEditorMoveBlocks>(this, oldPlacement, newPlacement);
        return m_actionHistory->execute(action);
    }

    //--

} // ui
