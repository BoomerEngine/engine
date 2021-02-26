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

#include "core/containers/include/hashSet.h"
#include "core/graph/include/graphObserver.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

class GraphEditorNode;
class GraphEditorBlockSocketTracker;
class GraphEditorDragDropPreviewHandler;

//--

// get a title for block class (string only)
extern ENGINE_UI_API StringBuf FormatBlockClassDisplayTitle(ClassType blockClass);

// format a display string (containing block icon, name/title and the group tag)
extern ENGINE_UI_API StringBuf FormatBlockClassDisplayString(ClassType blockClass, bool includeGroupTag = true);

// create helper element that is a detail block tooltip 
extern ENGINE_UI_API ElementPtr CreateGraphBlockTooltip(SpecificClassType<graph::Block> block);

//--

// drag&drop data with block class
class ENGINE_UI_API GraphBlockClassDragDropData : public DragDropData_String
{
    RTTI_DECLARE_VIRTUAL_CLASS(GraphBlockClassDragDropData, DragDropData_String);

public:
    GraphBlockClassDragDropData(SpecificClassType<graph::Block> blockClass);

    INLINE SpecificClassType<graph::Block> blockClass() const { return m_blockClass; }

private:
    SpecificClassType<graph::Block>  m_blockClass;
};

//--

struct GraphNodePlacement
{
    graph::BlockPtr block;
    Vector2 position;
};

struct ENGINE_UI_API GraphConnectionTarget
{
    graph::Block* block;
    StringID name;

    GraphConnectionTarget();
    GraphConnectionTarget(const graph::Socket* source);

    graph::Socket* resolve() const;

    INLINE bool operator==(const GraphConnectionTarget& other) const { return block == other.block && name == other.name; }
    INLINE bool operator!=(const GraphConnectionTarget& other) const { return !operator==(other); }
};

struct ENGINE_UI_API GraphConnection
{
    GraphConnectionTarget source;
    GraphConnectionTarget target;

    GraphConnection();
    GraphConnection(const graph::Connection* con);
    GraphConnection(const graph::Socket* source, const graph::Socket* destination);

    INLINE bool operator==(const GraphConnection& other) const { return (source == other.source && target == other.target) || (source == other.target && target == other.source); }
    INLINE bool operator!=(const GraphConnection& other) const { return !operator==(other); }
};

//--

/// graph editor - edits the blocks and connections in the graph container
class ENGINE_UI_API GraphEditor : public VirtualArea, public graph::IGraphObserver
{
    RTTI_DECLARE_VIRTUAL_CLASS(GraphEditor, VirtualArea);

public:
    GraphEditor();
    virtual ~GraphEditor();

    //--

    // get connections from the connection clipboard
    //INLINE Array<graph::GraphConnection>& connectionClipboard() { return m_connectionClipboard; }

    // current graph being edited
    INLINE const graph::ContainerPtr& graph() const { return m_graph; }

    // current action history
    INLINE const ActionHistoryPtr& actionHistory() const { return m_actionHistory;  }
        

    //--

    // bind the graph document we want to edit
    void bindGraph(const graph::ContainerPtr& graph);

    // bind action history (NOTE: unsafe if there are already undo actions)
    void bindActionHistory(const ActionHistoryPtr& actionHistory);

    //--

    // enumerate selected blocks
    void enumerateSelectedBlocks(const std::function<bool(graph::Block*)>& enumFunc) const;

    // find socket at given position
    const graph::Socket* findSocketAtAbsolutePos(const Point& pos) const;

    // calculate link placement (point and direction) for given socket
    // NOTE: slow, to be used only in non-persistent situations
    bool calcSocketLinkPlacement(const graph::Socket* socket, Position& outSocketAbsolutePosition, Vector2& outSocketDirection) const;

    /// apply selection state directly, without any undo/redo step creation
    void applySelection(const Array<graph::BlockPtr>& selection, bool postEvent = true);

    /// apply position state directly, without any undo/redo step creation
    void applyPlacement(const Array<GraphNodePlacement>& placement);

    /// apply connections - remove some, add some, all as a transaction
    void applyConnections(const Array<GraphConnection>& connectionsToRemove, const Array<GraphConnection>& connectionsToAdd);

    /// apply object - remove some, add some, all as a transaction
    void applyObjects(const Array<graph::BlockPtr>& blocksToRemove, const Array<graph::BlockPtr>& blocksToAdd);

    //--

    // build context menu for a block
    virtual void buildBlockPopupMenu(GraphEditorBlockNode* node, MenuButtonContainer& menu);

    // build context menu for a socket
    virtual void buildSocketPopupMenu(GraphEditorBlockNode* node, graph::Socket* socket, MenuButtonContainer& menu);

    // build context menu for a socket
    virtual void buildGenericPopupMenu(const Point& point, MenuButtonContainer& menu);

    //--

    // create undo/redo action to connect one socket to another
    bool actionConnectSockets(graph::Socket* source, graph::Socket* target);

    // remove connections at block
    bool actionRemoveBlockConnections(graph::Block* block);

    // remove specific socket connection
    bool actionRemoveSocketConnection(graph::Socket* socket, graph::Socket* target);

    // remove connections at socket
    bool actionRemoveSocketConnections(graph::Socket* socket);

    // copy connections of socket
    bool actionCopySocketConnections(graph::Socket* socket);

    // cut connections of socket
    bool actionCutSocketConnections(graph::Socket* socket);

    // paste connections into socket replacing current ones
    bool actionPasteSocketConnections(graph::Socket* socket);

    // paste connections into socket merging with current ones
    bool actionMergeSocketConnections(graph::Socket* socket);

    // create block
    bool actionCreateBlock(ClassType blockClass, const VirtualPosition& virtualPos);

    // delete given block(s)
    bool actionRemoveBlocks(const Array<graph::Block*>& blocks, StringView desc = "");

    // delete given block(s)
    bool actionRemoveBlocks(const Array<graph::BlockPtr>& blocks, StringView desc = "");

    // delete selection
    bool actionDeleteSelection();

    // cut selection
    bool actionCutSelection();

    // copy current selection (not actually an action because it does not leave undo step)
    bool actionCopySelection();

    // paste selection 
    bool actionPasteSelection(bool useSpecificVirtualPos, const VirtualPosition& virtualPos);

    // change selection
    bool actionChangeSelection(const Array<const graph::Block*>& oldSelection, const Array<const graph::Block*>& newSelection);

    // create new block (and autoconnect it to socket)
    bool actionCreateConnectedBlock(ClassType blockClass, StringID blockSocket, const VirtualPosition& virtualPos, const graph::Block* otherBlock, StringID otherBlockSocket);

    //--

    // check if we can paste connections into socket replacing current ones
    bool testPasteSocketConnections(const graph::Socket* socket) const;

    // paste connections into socket merging with current ones
    bool testMergeSocketConnections(const graph::Socket* socket) const;

    // open UI to to create a block at given absolute position in graph and auto connect it to given socket (NULL if it's a new block)
    bool tryCreateNewConnectedBlockAtPosition(graph::Socket* socket, const Point& absolutePosition);

private:
    // undo/redo action history
    ActionHistoryPtr m_actionHistory;

    // current graph being edited
    graph::ContainerPtr m_graph;

    // node visualizations
    HashMap<const graph::Block*, GraphEditorBlockNode*> m_nodesMap;
    Array<GraphEditorNodePtr> m_nodes;

    // hover
    RefWeakPtr<GraphEditorBlockNode> m_hoverBlock = nullptr;
    RefWeakPtr<graph::Socket> m_hoverSocket = nullptr;
    Position m_lastHoverPosition;
    bool m_lastHoverPositionValid = false;

    // context menu
    GraphEditorBlockNode* m_contextBlock = nullptr;
    const graph::Socket* m_contextSocket = nullptr;

    // connection clipboard
    Array<GraphConnectionTarget> m_connectionClipboard;


    Array<graph::BlockPtr> currentSelection() const;

    //--

    void updateHover(GraphEditorBlockNode* block, const graph::Socket* socket);
    void resetHover();
    void refreshHoverFromLastValidPosition();
    void refreshHoverFromPosition(const Position& pos);

    //--

    // new block preview (drag&drop)
    GraphEditorNodePtr m_previewBlock;

    bool createPreviewNode(const Point& absolutePosition, ClassType blockClass);
    void movePreviewNode(const Point& absolutePosition);
    void removePreviewNode();

    friend class GraphEditorDragDropPreviewHandler;

    //--

    virtual bool handleMouseMovement(const input::MouseMovementEvent& evt) override;
    virtual void handleHoverLeave(const Position& absolutePosition) override;
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const input::MouseClickEvent& evt) override;
    virtual InputActionPtr handleOverlayMouseClick(const ElementArea& area, const input::MouseClickEvent& evt) override;
    virtual bool handleCursorQuery(const ElementArea& area, const Position& absolutePosition, input::CursorType& outCursorType) const override;
    virtual void renderCustomOverlayElements(HitCache& hitCache, DataStash& stash, const ElementArea& outerArea, const ElementArea& outerClipArea, canvas::Canvas& canvas, float mergedOpacity) override;
    virtual bool handleContextMenu(const ElementArea& area, const Position& absolutePosition, input::KeyMask controlKeys) override;
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
    virtual void handleBlockAdded(graph::Block* block) override final;
    virtual void handleBlockRemoved(graph::Block* block) override final;
    virtual void handleBlockStyleChanged(graph::Block* block) override final;
    virtual void handleBlockLayoutChanged(graph::Block* block) override final;
    virtual void handleBlockConnectionsChanged(graph::Block* block) override final;

    // VirtualArea
    virtual bool actionChangeSelection(const Array<const VirtualAreaElement*>& oldSelection, const Array<const VirtualAreaElement*>& newSelection) override;
    virtual bool actionMoveElements(const Array<VirtualAreaElementPositionState>& oldPositions, const Array<VirtualAreaElementPositionState>& newPositions) override;
};

//--

END_BOOMER_NAMESPACE_EX(ui)
