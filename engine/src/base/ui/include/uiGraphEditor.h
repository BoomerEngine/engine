/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
*
***/

#pragma once

#include "uiElement.h"
#include "uiDragDrop.h"
#include "uiVirtualArea.h"

#include "base/containers/include/hashSet.h"
#include "base/graph/include/graphObserver.h"

namespace ui
{
    class GraphEditorNode;
    class GraphEditorBlockSocketTracker;
    class GraphEditorDragDropPreviewHandler;

    //--

    // get a title for block class (string only)
    extern BASE_UI_API base::StringBuf FormatBlockClassDisplayTitle(base::ClassType blockClass);

    // format a display string (containing block icon, name/title and the group tag)
    extern BASE_UI_API base::StringBuf FormatBlockClassDisplayString(base::ClassType blockClass, bool includeGroupTag = true);

    // create helper element that is a detail block tooltip 
    extern BASE_UI_API ElementPtr CreateGraphBlockTooltip(base::SpecificClassType<base::graph::Block> block);

    //--

    // drag&drop data with block class
    class BASE_UI_API GraphBlockClassDragDropData : public DragDropData_String
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GraphBlockClassDragDropData, DragDropData_String);

    public:
        GraphBlockClassDragDropData(base::SpecificClassType<base::graph::Block> blockClass);

        INLINE base::SpecificClassType<base::graph::Block> blockClass() const { return m_blockClass; }

    private:
        base::SpecificClassType<base::graph::Block>  m_blockClass;
    };

    //--

    struct GraphNodePlacement
    {
        base::graph::BlockPtr block;
        base::Vector2 position;
    };

    struct BASE_UI_API GraphConnectionTarget
    {
        base::graph::Block* block;
        base::StringID name;

        GraphConnectionTarget();
        GraphConnectionTarget(const base::graph::Socket* source);

        base::graph::Socket* resolve() const;

        INLINE bool operator==(const GraphConnectionTarget& other) const { return block == other.block && name == other.name; }
        INLINE bool operator!=(const GraphConnectionTarget& other) const { return !operator==(other); }
    };

    struct BASE_UI_API GraphConnection
    {
        GraphConnectionTarget source;
        GraphConnectionTarget target;

        GraphConnection();
        GraphConnection(const base::graph::Connection* con);
        GraphConnection(const base::graph::Socket* source, const base::graph::Socket* destination);

        INLINE bool operator==(const GraphConnection& other) const { return (source == other.source && target == other.target) || (source == other.target && target == other.source); }
        INLINE bool operator!=(const GraphConnection& other) const { return !operator==(other); }
    };

    //--

    /// graph editor - edits the blocks and connections in the graph container
    class BASE_UI_API GraphEditor : public VirtualArea, public base::graph::IGraphObserver
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GraphEditor, VirtualArea);

    public:
        GraphEditor();
        virtual ~GraphEditor();

        //--

        // get connections from the connection clipboard
        //INLINE base::Array<base::graph::GraphConnection>& connectionClipboard() { return m_connectionClipboard; }

        // current graph being edited
        INLINE const base::graph::ContainerPtr& graph() const { return m_graph; }

        // current action history
        INLINE const base::ActionHistoryPtr& actionHistory() const { return m_actionHistory;  }
        

        //--

        // bind the graph document we want to edit
        void bindGraph(const base::graph::ContainerPtr& graph);

        // bind action history (NOTE: unsafe if there are already undo actions)
        void bindActionHistory(const base::ActionHistoryPtr& actionHistory);

        //--

        // enumerate selected blocks
        void enumerateSelectedBlocks(const std::function<bool(base::graph::Block*)>& enumFunc) const;

        // find socket at given position
        const base::graph::Socket* findSocketAtAbsolutePos(const base::Point& pos) const;

        // calculate link placement (point and direction) for given socket
        // NOTE: slow, to be used only in non-persistent situations
        bool calcSocketLinkPlacement(const base::graph::Socket* socket, Position& outSocketAbsolutePosition, base::Vector2& outSocketDirection) const;

        /// apply selection state directly, without any undo/redo step creation
        void applySelection(const base::Array<base::graph::BlockPtr>& selection, bool postEvent = true);

        /// apply position state directly, without any undo/redo step creation
        void applyPlacement(const base::Array<GraphNodePlacement>& placement);

        /// apply connections - remove some, add some, all as a transaction
        void applyConnections(const base::Array<GraphConnection>& connectionsToRemove, const base::Array<GraphConnection>& connectionsToAdd);

        /// apply object - remove some, add some, all as a transaction
        void applyObjects(const base::Array<base::graph::BlockPtr>& blocksToRemove, const base::Array<base::graph::BlockPtr>& blocksToAdd);

        //--

        // build context menu for a block
        virtual void buildBlockPopupMenu(GraphEditorBlockNode* node, MenuButtonContainer& menu);

        // build context menu for a socket
        virtual void buildSocketPopupMenu(GraphEditorBlockNode* node, base::graph::Socket* socket, MenuButtonContainer& menu);

        // build context menu for a socket
        virtual void buildGenericPopupMenu(const base::Point& point, MenuButtonContainer& menu);

        //--

        // create undo/redo action to connect one socket to another
        bool actionConnectSockets(base::graph::Socket* source, base::graph::Socket* target);

        // remove connections at block
        bool actionRemoveBlockConnections(base::graph::Block* block);

        // remove specific socket connection
        bool actionRemoveSocketConnection(base::graph::Socket* socket, base::graph::Socket* target);

        // remove connections at socket
        bool actionRemoveSocketConnections(base::graph::Socket* socket);

        // copy connections of socket
        bool actionCopySocketConnections(base::graph::Socket* socket);

        // cut connections of socket
        bool actionCutSocketConnections(base::graph::Socket* socket);

        // paste connections into socket replacing current ones
        bool actionPasteSocketConnections(base::graph::Socket* socket);

        // paste connections into socket merging with current ones
        bool actionMergeSocketConnections(base::graph::Socket* socket);

        // create block
        bool actionCreateBlock(base::ClassType blockClass, const VirtualPosition& virtualPos);

        // delete given block(s)
        bool actionRemoveBlocks(const base::Array<base::graph::Block*>& blocks, base::StringView desc = "");

        // delete given block(s)
        bool actionRemoveBlocks(const base::Array<base::graph::BlockPtr>& blocks, base::StringView desc = "");

        // delete selection
        bool actionDeleteSelection();

        // cut selection
        bool actionCutSelection();

        // copy current selection (not actually an action because it does not leave undo step)
        bool actionCopySelection();

        // paste selection 
        bool actionPasteSelection(bool useSpecificVirtualPos, const VirtualPosition& virtualPos);

        // change selection
        bool actionChangeSelection(const base::Array<const base::graph::Block*>& oldSelection, const base::Array<const base::graph::Block*>& newSelection);

        // create new block (and autoconnect it to socket)
        bool actionCreateConnectedBlock(base::ClassType blockClass, base::StringID blockSocket, const VirtualPosition& virtualPos, const base::graph::Block* otherBlock, base::StringID otherBlockSocket);

        //--

        // check if we can paste connections into socket replacing current ones
        bool testPasteSocketConnections(const base::graph::Socket* socket) const;

        // paste connections into socket merging with current ones
        bool testMergeSocketConnections(const base::graph::Socket* socket) const;

        // open UI to to create a block at given absolute position in graph and auto connect it to given socket (NULL if it's a new block)
        bool tryCreateNewConnectedBlockAtPosition(base::graph::Socket* socket, const base::Point& absolutePosition);

    private:
        // undo/redo action history
        base::ActionHistoryPtr m_actionHistory;

        // current graph being edited
        base::graph::ContainerPtr m_graph;

        // node visualizations
        base::HashMap<const base::graph::Block*, GraphEditorBlockNode*> m_nodesMap;
        base::Array<GraphEditorNodePtr> m_nodes;

        // hover
        base::RefWeakPtr<GraphEditorBlockNode> m_hoverBlock = nullptr;
        base::RefWeakPtr<base::graph::Socket> m_hoverSocket = nullptr;
        Position m_lastHoverPosition;
        bool m_lastHoverPositionValid = false;

        // context menu
        GraphEditorBlockNode* m_contextBlock = nullptr;
        const base::graph::Socket* m_contextSocket = nullptr;

        // connection clipboard
        base::Array<GraphConnectionTarget> m_connectionClipboard;


        base::Array<base::graph::BlockPtr> currentSelection() const;

        //--

        void updateHover(GraphEditorBlockNode* block, const base::graph::Socket* socket);
        void resetHover();
        void refreshHoverFromLastValidPosition();
        void refreshHoverFromPosition(const Position& pos);

        //--

        // new block preview (drag&drop)
        GraphEditorNodePtr m_previewBlock;

        bool createPreviewNode(const base::Point& absolutePosition, base::ClassType blockClass);
        void movePreviewNode(const base::Point& absolutePosition);
        void removePreviewNode();

        friend class GraphEditorDragDropPreviewHandler;

        //--

        virtual bool handleMouseMovement(const base::input::MouseMovementEvent& evt) override;
        virtual void handleHoverLeave(const Position& absolutePosition) override;
        virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
        virtual InputActionPtr handleOverlayMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
        virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const override;
        virtual void renderCustomOverlayElements(HitCache& hitCache, DataStash& stash, const ElementArea& outerArea, const ElementArea& outerClipArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
        virtual bool handleContextMenu(const ElementArea& area, const Position& absolutePosition) override;
        virtual DragDropHandlerPtr handleDragDrop(const DragDropDataPtr& data, const Position& entryPosition) override;

        //--


        void deleteVisualizations();
        void createVisualizations();

        //--

        void cmdPasteAtPoint();
        void cmdZoomSelection();
        void cmdZoomAll();

        //--

        // IGraphObserver
        virtual void handleBlockAdded(base::graph::Block* block) override final;
        virtual void handleBlockRemoved(base::graph::Block* block) override final;
        virtual void handleBlockStyleChanged(base::graph::Block* block) override final;
        virtual void handleBlockLayoutChanged(base::graph::Block* block) override final;
        virtual void handleBlockConnectionsChanged(base::graph::Block* block) override final;

        // VirtualArea
        virtual bool actionChangeSelection(const base::Array<const VirtualAreaElement*>& oldSelection, const base::Array<const VirtualAreaElement*>& newSelection) override;
        virtual bool actionMoveElements(const base::Array<VirtualAreaElementPositionState>& oldPositions, const base::Array<VirtualAreaElementPositionState>& newPositions) override;
    };

    //--

} // ui
