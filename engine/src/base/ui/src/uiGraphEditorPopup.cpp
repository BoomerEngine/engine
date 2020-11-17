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

#include "base/graph/include/graphBlock.h"
#include "base/graph/include/graphSocket.h"
#include "base/graph/include/graphConnection.h"

namespace ui
{
    //--

    void GraphEditor::buildBlockPopupMenu(GraphEditorBlockNode* node, MenuButtonContainer& menu)
    {
        if (auto block = node->block())
        {
            if (block->hasConnections())
            {
                menu.createCallback("Remove all connections", "[img:delete]", "Del") = [this, node]() {
                    actionRemoveBlockConnections(node->block());
                };
                menu.createSeparator();
            }
        }
    }

    void GraphEditor::buildSocketPopupMenu(GraphEditorBlockNode* node, base::graph::Socket* socket, MenuButtonContainer& menu)
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
            auto subMenu = base::RefNew<MenuButtonContainer>();

            for (const auto& con : socket->connections())
            {
                if (auto other = con->otherSocket(socket))
                {
                    if (auto block = other->block())
                    {
                        subMenu->createCallback(base::TempString("'{}' in '{}'", other->name(), block->chooseTitle())) = [this, socket, other]() {
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

    void GraphEditor::buildGenericPopupMenu(const base::Point& point, MenuButtonContainer& menu)
    {
        menu.createCallback("Create new block...", "[img:add]", "Ctrl+N") = [this, point]() {
            tryCreateNewConnectedBlockAtPosition(nullptr, point);
        };
        menu.createSeparator();        
    }

    //--

    bool GraphEditor::tryCreateNewConnectedBlockAtPosition(base::graph::Socket* socket, const base::Point& absolutePosition)
    {
        if (!m_graph)
            return false;

        auto selfRef = base::RefWeakPtr<GraphEditor>(this);
        auto virtualPosition = absoluteToVirtual(absolutePosition);
        auto blockRef = socket ? socket->block() : nullptr;
        auto socketName = socket ? socket->name() : base::StringID();

        auto window = base::RefNew<BlockClassPickerBox>(*m_graph, socket);
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

} // ui
