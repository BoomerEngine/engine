/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
*
***/

#include "build.h"
#include "uiInputAction.h"
#include "uiGraphEditor.h"
#include "uiGraphEditorNode.h"
#include "uiGraphEditorNodeClassSelector.h"
#include "uiRenderer.h"
#include "uiMenuBar.h"

#include "core/graph/include/graphBlock.h"
#include "core/graph/include/graphSocket.h"
#include "core/graph/include/graphConnection.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

void GraphEditor::buildBlockPopupMenu(GraphEditorBlockNode* node, MenuButtonContainer& menu)
{
    if (auto block = node->block())
    {
        if (block->hasConnections())
        {
            menu.createCallback("Remove all connections", "[img:delete]", "Delete") = [this, node]() {
                actionRemoveBlockConnections(node->block());
            };
            menu.createSeparator();
        }

        bool hasUnconnectedSockets = false;
        auto showSocketsPopup = RefNew<MenuButtonContainer>();
        //auto hideSocketsPopup = RefNew<MenuButtonContainer>();

        for (const auto& socket : block->sockets())
        {
            if (!socket->visible())
            {
                showSocketsPopup->createCallback(TempString("Show '{}'", socket->name())) = [this, socket]()
                {
                    socket->updateVisibility(true);
                };
            }
            else if (socket->info().m_hiddableByUser)
            {
                hasUnconnectedSockets = true;
            }
        }

        if (auto popup = showSocketsPopup->convertToPopup())
        {
            menu.createSubMenu(popup, "Show hidden socket");
        }

        if (hasUnconnectedSockets)
        {
            menu.createCallback("Hide unused sockets") = [this, block]() {
                for (const auto& socket : block->sockets())
                {
                    if (socket->visible() && socket->info().m_hiddableByUser && !socket->hasConnections())
                        socket->updateVisibility(false);
                }
            };
        }

        menu.createSeparator();
    }
}

void GraphEditor::buildSocketPopupMenu(GraphEditorBlockNode* node, graph::Socket* socket, MenuButtonContainer& menu)
{
    if (socket->hasConnections())
    {
        menu.createCallback("Remove connections", "[img:delete]") = [this, socket]() {
            actionRemoveSocketConnections(socket);
        };
        menu.createSeparator();

        menu.createCallback("Copy connections", "[img:copy]") = [this, socket]() {
            actionCopySocketConnections(socket);
        };
        menu.createCallback("Cut connections", "[img:cut]") = [this, socket]() {
            actionCutSocketConnections(socket);
        };
    }
    else if (socket->info().m_hiddableByUser)
    {
        menu.createCallback("Hide") = [this, socket]() {
            socket->updateVisibility(false);
        };

        menu.createSeparator();
    }

    if (!m_connectionClipboard.empty())
    { 
        if (testPasteSocketConnections(socket))
        {
            menu.createCallback("Paste connections", "[img:paste]") = [this, socket]() {
                actionPasteSocketConnections(socket);
            };
        }

        if (socket->hasConnections() && testMergeSocketConnections(socket))
        {
            menu.createCallback("Merge connections", "[img:scc_add]") = [this, socket]() {
                actionMergeSocketConnections(socket);
            };
        }

        menu.createSeparator();
    }

    if (socket->hasConnections())
    {
        auto subMenu = RefNew<MenuButtonContainer>();

        for (const auto& con : socket->connections())
        {
            if (auto other = con->otherSocket(socket))
            {
                if (auto block = other->block())
                {
                    subMenu->createCallback(TempString("'{}' in '{}'", other->name(), block->chooseTitle())) = [this, socket, other]() {
                        actionRemoveSocketConnection(socket, other);
                    };
                }
            }
        }

        if (auto popup = subMenu->convertToPopup())
        {
            menu.createSubMenu(popup, "Break connection to", "[img:link_remove]");
            menu.createSeparator();
        }
    }
}

void GraphEditor::buildGenericPopupMenu(const Point& point, MenuButtonContainer& menu)
{
    menu.createCallback("Create new block...", "[img:add]", "Ctrl+N") = [this, point]() {
        tryCreateNewConnectedBlockAtPosition(nullptr, point);
    };
    menu.createSeparator();        
}

//--

bool GraphEditor::tryCreateNewConnectedBlockAtPosition(graph::Socket* socket, const Point& absolutePosition)
{
    if (!m_graph)
        return false;

    auto selfRef = RefWeakPtr<GraphEditor>(this);
    auto virtualPosition = absoluteToVirtual(absolutePosition);
    auto blockRef = socket ? socket->block() : nullptr;
    auto socketName = socket ? socket->name() : StringID();

    auto window = RefNew<BlockClassPickerBox>(*m_graph, socket);
    window->bind(EVENT_GRAPH_BLOCK_CLASS_SELECTED) = [selfRef, virtualPosition, blockRef, socketName](BlockClassPickResult result)
    {
        if (auto editor = selfRef.lock())
        {
            if (blockRef && socketName)
                editor->actionCreateConnectedBlock(result.blockClass, result.socketName, virtualPosition, blockRef, socketName);
            else
                editor->actionCreateBlock(result.blockClass, virtualPosition);
        }
    };
        
    renderer()->runModalLoop(window);
    return true;
}

END_BOOMER_NAMESPACE_EX(ui)
