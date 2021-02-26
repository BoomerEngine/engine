/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
*
***/

#pragma once

#include "uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

class GraphEditorBlockSocketTracker;

//--

/// layout data for socket, NOTE: all positions are relative to block placement
struct GrapgEditorSocketInternalLayout
{
    const graph::Socket* ptr = nullptr;

    StringBuf textLabel;
    Vector2 textPlacement; // socket UI placement - usually the text label

    Vector2 socketOffset; // socket "active end" placement
    Vector2 socketSize; // socket "active end" size
    Vector2 linkPoint; // position of the connection end point
    Vector2 linkDir; // direction (tangent) of the link at the point

    graph::SocketPlacement socketPlacement;
    graph::SocketShape socketShape;
    Color socketColor;
    Color linkColor;
};

// layout data for the node
struct GraphEditorNodeInternalLayout
{
    bool valid = false;
    bool hasTitle = false;

    graph::BlockShape shape = graph::BlockShape::Rectangle;

    Vector2 blockSize;
    Vector2 titleSize;
    Vector2 clientOffset;
    Vector2 clientSize;
    Vector2 payloadOffset;
    Vector2 payloadSize;
    Color titleColor;
    Color borderColor;

    HashMap<const graph::Socket*, GrapgEditorSocketInternalLayout> sockets;

    //Vector2 clientSize;
    //Rect blockRect; // total window area, placed at 0,0
    //Rect titleRect; // area of the title bar
    //Rect iconRect;
    //Rect clientRect;
    //Rect innerRect;
};

//--

/// base node in the graph editor, we usually have 3 major classes: block nodes (data nodes), comments blocks and notes
class ENGINE_UI_API IGraphEditorNode : public VirtualAreaElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGraphEditorNode, VirtualAreaElement);

public:
    IGraphEditorNode();
    virtual ~IGraphEditorNode();
};

//--

/// socket label/data
class ENGINE_UI_API GraphEditorSocketLabel : public TextLabel
{
    RTTI_DECLARE_VIRTUAL_CLASS(GraphEditorSocketLabel, TextLabel);

public:
    GraphEditorSocketLabel(const graph::SocketPtr& socket);

    INLINE const graph::SocketPtr& socket() const { return m_socket; }

private:
    graph::SocketPtr m_socket;
};

//--

/// socket position tracker
class ENGINE_UI_API GraphEditorBlockSocketTracker : public IReferencable
{
public:
    GraphEditorBlockSocketTracker(const graph::Socket* socket);

    INLINE bool valid() const { return m_valid; }

    INLINE const Vector2& linkPos() const { return m_linkPos; }
    INLINE const Vector2& linkDir() const { return m_linkDir; }

    INLINE const RefWeakPtr<graph::Socket>& socket() const { return m_socket; }

    void update(const Vector2& pos, const Vector2& dir);
    void invalidate();

private:
    RefWeakPtr<GraphEditorBlockNode> m_node;
    RefWeakPtr<graph::Socket> m_socket;
    Vector2 m_linkPos;
    Vector2 m_linkDir;
    bool m_valid;
};

//--

/// visualization of the node in the graph
class ENGINE_UI_API GraphEditorBlockNode : public IGraphEditorNode
{
    RTTI_DECLARE_VIRTUAL_CLASS(GraphEditorBlockNode, IGraphEditorNode);

public:
    GraphEditorBlockNode(graph::Block* block);
    virtual ~GraphEditorBlockNode();

    /// block this node visualizes
    INLINE const graph::BlockPtr& block() const { return m_block; }

    //---

    // refresh block style
    void refreshBlockStyle();

    // suck in the new sockets
    void refreshBlockSockets();

    //---

    // get socket at given position
    const graph::Socket* socketAtAbsolutePos(const Position& pos) const;

    // get link information from socket, NOTE: slow
    bool calcSocketLinkPlacement(const graph::Socket* socket, Position& outLocalPosition, Vector2& outSocketDirection) const;

    // update the hovered socket (for visualization only)
    void updateHoverSocket(const graph::Socket* socket);

    // create a socket tracker for given socket
    RefPtr<GraphEditorBlockSocketTracker> createSocketTracker(const graph::Socket* socket);

    // draw connections of this block
    void drawConnections(const ElementArea& outerArea, const ElementArea& outerClipArea, canvas::Canvas& canvas, float mergedOpacity);

    //--

    // move block to position
    virtual void virtualPosition(const VirtualPosition& pos, bool updateInContainer = true) override;

private:
    graph::BlockPtr m_block;
    mutable GraphEditorNodeInternalLayout m_layout;

    const graph::Socket* m_hoverSocketPreview = nullptr;

    TextLabelPtr m_title;
    ElementPtr m_payload;

    mutable Array<RefPtr<GraphEditorBlockSocketTracker>> m_trackers;

    void invalidateBlockLayout();
    void updateTrackedSockedPositions() const;

    virtual void updateActualPosition(const Vector2& pos) override;

    virtual void computeSize(Size& outSize) const override;
    virtual void prepareBoundaryGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder, float inset) const override;
    virtual void prepareBackgroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const override;
    virtual void prepareShadowGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const override;
    virtual void renderCustomOverlayElements(HitCache& hitCache, DataStash& stash, const ElementArea& outerArea, const ElementArea& outerClipArea, canvas::Canvas& canvas, float mergedOpacity) override;

    virtual bool adjustBackgroundStyle(canvas::RenderStyle& outStyle, float& outBorderWidth) const override;
    virtual bool adjustBorderStyle(canvas::RenderStyle& outStyle, float& outBorderWidth) const override;

    void prepareBlockOutline(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder, float inset, float titleHeightLimit) const;
    Size computeLayoutWithTitle(const Size& innerContentSize, float innerPadding, GraphEditorNodeInternalLayout& outLayout) const;
    Size computeLayoutNoTitle(const Size& innerContentSize, float innerPadding, bool adjustForSlant, GraphEditorNodeInternalLayout& outLayout) const;
    Size computeLayoutCircleLike(const Size& innerContentSize, float innerPadding, GraphEditorNodeInternalLayout& outLayout) const;
};

//--

END_BOOMER_NAMESPACE_EX(ui)
