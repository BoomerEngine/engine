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

namespace ui
{
    //--

    class GraphEditorBlockSocketTracker;

    //--

    /// layout data for socket, NOTE: all positions are relative to block placement
    struct GrapgEditorSocketInternalLayout
    {
        const base::graph::Socket* ptr = nullptr;

        base::StringBuf textLabel;
        base::Vector2 textPlacement; // socket UI placement - usually the text label

        base::Vector2 socketOffset; // socket "active end" placement
        base::Vector2 socketSize; // socket "active end" size
        base::Vector2 linkPoint; // position of the connection end point
        base::Vector2 linkDir; // direction (tangent) of the link at the point

        base::graph::SocketPlacement socketPlacement;
        base::graph::SocketShape socketShape;
        base::Color socketColor;
        base::Color linkColor;
    };

    // layout data for the node
    struct GraphEditorNodeInternalLayout
    {
        bool valid = false;
        bool hasTitle = false;

        base::graph::BlockShape shape = base::graph::BlockShape::Rectangle;

        base::Vector2 blockSize;
        base::Vector2 titleSize;
        base::Vector2 clientOffset;
        base::Vector2 clientSize;
        base::Vector2 payloadOffset;
        base::Vector2 payloadSize;
        base::Color titleColor;
        base::Color borderColor;

        base::HashMap<const base::graph::Socket*, GrapgEditorSocketInternalLayout> sockets;

        //base::Vector2 clientSize;
        //base::Rect blockRect; // total window area, placed at 0,0
        //base::Rect titleRect; // area of the title bar
        //base::Rect iconRect;
        //base::Rect clientRect;
        //base::Rect innerRect;
    };

    //--

    /// base node in the graph editor, we usually have 3 major classes: block nodes (data nodes), comments blocks and notes
    class BASE_UI_API IGraphEditorNode : public VirtualAreaElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IGraphEditorNode, VirtualAreaElement);

    public:
        IGraphEditorNode();
        virtual ~IGraphEditorNode();
    };

    //--

    /// socket label/data
    class BASE_UI_API GraphEditorSocketLabel : public TextLabel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GraphEditorSocketLabel, TextLabel);

    public:
        GraphEditorSocketLabel(const base::graph::SocketPtr& socket);

        INLINE const base::graph::SocketPtr& socket() const { return m_socket; }

    private:
        base::graph::SocketPtr m_socket;
    };

    //--

    /// socket position tracker
    class BASE_UI_API GraphEditorBlockSocketTracker : public base::IReferencable
    {
    public:
        GraphEditorBlockSocketTracker(const base::graph::Socket* socket);

        INLINE bool valid() const { return m_valid; }

        INLINE const base::Vector2& linkPos() const { return m_linkPos; }
        INLINE const base::Vector2& linkDir() const { return m_linkDir; }

        INLINE const base::RefWeakPtr<base::graph::Socket>& socket() const { return m_socket; }

        void update(const base::Vector2& pos, const base::Vector2& dir);
        void invalidate();

    private:
        base::RefWeakPtr<GraphEditorBlockNode> m_node;
        base::RefWeakPtr<base::graph::Socket> m_socket;
        base::Vector2 m_linkPos;
        base::Vector2 m_linkDir;
        bool m_valid;
    };

    //--

    /// visualization of the node in the graph
    class BASE_UI_API GraphEditorBlockNode : public IGraphEditorNode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GraphEditorBlockNode, IGraphEditorNode);

    public:
        GraphEditorBlockNode(base::graph::Block* block);
        virtual ~GraphEditorBlockNode();

        /// block this node visualizes
        INLINE const base::graph::BlockPtr& block() const { return m_block; }

        //---

        // refresh block style
        void refreshBlockStyle();

        // suck in the new sockets
        void refreshBlockSockets();

        //---

        // get socket at given position
        const base::graph::Socket* socketAtAbsolutePos(const Position& pos) const;

        // get link information from socket, NOTE: slow
        bool calcSocketLinkPlacement(const base::graph::Socket* socket, Position& outLocalPosition, base::Vector2& outSocketDirection) const;

        // update the hovered socket (for visualization only)
        void updateHoverSocket(const base::graph::Socket* socket);

        // create a socket tracker for given socket
        base::RefPtr<GraphEditorBlockSocketTracker> createSocketTracker(const base::graph::Socket* socket);

        // draw connections of this block
        void drawConnections(const ElementArea& outerArea, const ElementArea& outerClipArea, base::canvas::Canvas& canvas, float mergedOpacity);

        //--

        // move block to position
        virtual void virtualPosition(const VirtualPosition& pos, bool updateInContainer = true) override;

    private:
        base::graph::BlockPtr m_block;
        mutable GraphEditorNodeInternalLayout m_layout;

        const base::graph::Socket* m_hoverSocketPreview = nullptr;

        TextLabelPtr m_title;
        ElementPtr m_payload;

        mutable base::Array<base::RefPtr<GraphEditorBlockSocketTracker>> m_trackers;

        void invalidateBlockLayout();
        void updateTrackedSockedPositions() const;

        virtual void updateActualPosition(const base::Vector2& pos) override;

        virtual void computeSize(Size& outSize) const override;
        virtual void prepareBoundaryGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder, float inset) const override;
        virtual void prepareBackgroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const override;
        virtual void prepareShadowGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const override;
        virtual void renderCustomOverlayElements(HitCache& hitCache, DataStash& stash, const ElementArea& outerArea, const ElementArea& outerClipArea, base::canvas::Canvas& canvas, float mergedOpacity) override;

        virtual bool adjustBackgroundStyle(base::canvas::RenderStyle& outStyle, float& outBorderWidth) const override;
        virtual bool adjustBorderStyle(base::canvas::RenderStyle& outStyle, float& outBorderWidth) const override;

        void prepareBlockOutline(DataStash& stash, const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder, float inset, float titleHeightLimit) const;
        Size computeLayoutWithTitle(const Size& innerContentSize, float innerPadding, GraphEditorNodeInternalLayout& outLayout) const;
        Size computeLayoutNoTitle(const Size& innerContentSize, float innerPadding, bool adjustForSlant, GraphEditorNodeInternalLayout& outLayout) const;
        Size computeLayoutCircleLike(const Size& innerContentSize, float innerPadding, GraphEditorNodeInternalLayout& outLayout) const;
    };

    //--

} // ui
