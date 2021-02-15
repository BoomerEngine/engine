/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: graph #]
*
***/

#include "build.h"
#include "uiGraphEditor.h"
#include "uiGraphEditorNode.h"
#include "uiGraphEditorNodeInnerWidget.h"
#include "uiTextLabel.h"
#include "uiStyleValue.h"
#include "uiImage.h"

#include "base/graph/include/graphBlock.h"
#include "base/graph/include/graphSocket.h"
#include "base/canvas/include/canvasGeometry.h"

namespace ui
{
    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(IGraphEditorNode);
        //RTTI_METADATA(ElementClassNameMetadata).name("GraphEditorNode");
    RTTI_END_TYPE();

    IGraphEditorNode::IGraphEditorNode()
    {}

    IGraphEditorNode::~IGraphEditorNode()
    {}

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(GraphEditorSocketLabel);
        RTTI_METADATA(ElementClassNameMetadata).name("GraphEditorSocket");
    RTTI_END_TYPE();

    GraphEditorSocketLabel::GraphEditorSocketLabel(const base::graph::SocketPtr& socket)
        : m_socket(socket)
    {
        const auto caption = socket->name().view();
        text(caption);
    }

    //--

    GraphEditorBlockSocketTracker::GraphEditorBlockSocketTracker(const base::graph::Socket* socket)
        : m_linkPos(0,0)
        , m_linkDir(0,0)
    {
        m_socket = socket;
    }

    void GraphEditorBlockSocketTracker::update(const base::Vector2& pos, const base::Vector2& dir)
    {
        m_linkPos = pos;
        m_linkDir = dir;
        m_valid = true;
    }

    void GraphEditorBlockSocketTracker::invalidate()
    {
        m_valid = false;
    }

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(GraphEditorBlockNode);
        RTTI_METADATA(ElementClassNameMetadata).name("GraphEditorBlockNode");
    RTTI_END_TYPE();

    GraphEditorBlockNode::GraphEditorBlockNode(base::graph::Block* block)
        : m_block(AddRef(block))
    {
        hitTest();
        virtualPosition(block->position().toVector());
        renderOverlayElements(false);

        m_title = createChildWithType<ui::TextLabel>("GraphEditorNodeTitle"_id);
        m_title->overlay(true);

        m_payload = CreateGraphBlockInnerWidget(block);
        if (m_payload)
        {
            m_payload->overlay(true);
            attachChild(m_payload);
        }
    }

    GraphEditorBlockNode::~GraphEditorBlockNode()
    {}

    const base::graph::Socket* GraphEditorBlockNode::socketAtAbsolutePos(const Position& pos) const
    {
        const auto localPos = pos - cachedDrawArea().absolutePosition();

        for (const auto& socket : m_layout.sockets.values())
        {
            if (localPos.x >= socket.socketOffset.x && localPos.y >= socket.socketOffset.y &&
                localPos.x <= socket.socketOffset.x + socket.socketSize.x && localPos.y <= socket.socketOffset.y + socket.socketSize.y)
            {
                return socket.ptr;
            }
        }

        return nullptr;
    }

    bool GraphEditorBlockNode::calcSocketLinkPlacement(const base::graph::Socket* socket, Position& outLocalPosition, base::Vector2& outSocketDirection) const
    {
        if (const auto* socketInfo = m_layout.sockets.find(socket))
        {
            outLocalPosition = socketInfo->linkPoint;
            outSocketDirection = socketInfo->linkDir;
            return true;
        }

        return false;
    }

    void GraphEditorBlockNode::updateHoverSocket(const base::graph::Socket* socket)
    {
        m_hoverSocketPreview = socket;
        invalidateLayout();
        invalidateGeometry();
    }

    void GraphEditorBlockNode::updateTrackedSockedPositions() const
    {
        auto oldTrackers = std::move(m_trackers);

        for (const auto& tracker : oldTrackers)
        {
            if (auto socket = tracker->socket().lock())
            {
                if (const auto* layoutInfo = m_layout.sockets.find(socket))
                {
                    auto linkActualPos = actualPosition() + layoutInfo->linkPoint;
                    tracker->update(linkActualPos, layoutInfo->linkDir);
                    m_trackers.pushBack(tracker);
                    continue;
                }
            }

            // this tracker is no longer valid - socket is gone
            // hopefully it's not used
            tracker->invalidate();
        }
    }

    void GraphEditorBlockNode::updateActualPosition(const base::Vector2& pos)
    {
        TBaseClass::updateActualPosition(pos);
        updateTrackedSockedPositions();
    }

    void GraphEditorBlockNode::virtualPosition(const VirtualPosition& pos, bool updateInContainer /*= true*/)
    {
        TBaseClass::virtualPosition(pos, updateInContainer);
        m_block->position(pos);
    }

    base::RefPtr<GraphEditorBlockSocketTracker> GraphEditorBlockNode::createSocketTracker(const base::graph::Socket* socket)
    {
        for (const auto& tracker : m_trackers)  
            if (tracker->socket() == socket)
                return tracker;

        auto tracker = base::RefNew<GraphEditorBlockSocketTracker>(socket);
        m_trackers.pushBack(tracker);
        return nullptr;
    }

    void GraphEditorBlockNode::invalidateBlockLayout()
    {
        if (m_layout.valid)
        {
            m_layout.valid = false;
            invalidateLayout();
        }
    }

    //--

    ElementPtr CreateGraphBlockTooltip(base::SpecificClassType<base::graph::Block> block)
    {
        ElementPtr ret = base::RefNew<IElement>();
        ret->layoutVertical();
        ret->customPadding(5, 5, 5, 5);

        // block info
        ret->createChild<ui::TextLabel>(FormatBlockClassDisplayString(block));

        // block preview
        if (auto tempBlock = block.create())
        {
            // create default sockets
            tempBlock->rebuildLayout();

            // create a nice container
            auto conainer = ret->createChildWithType<IElement>("GraphNodePreviewContainer"_id);
            conainer->customHorizontalAligment(ElementHorizontalLayout::Expand);
            conainer->layoutVertical();

            // create node visualization
            auto node = conainer->createChild<GraphEditorBlockNode>(tempBlock);
            node->overlay(false);
            node->hitTest(false);
            node->refreshBlockStyle();
            node->refreshBlockSockets();
            node->customHorizontalAligment(ElementHorizontalLayout::Center);
            node->customVerticalAligment(ElementVerticalLayout::Middle);
            node->customProportion(1.0f);            
        }

        // additional informations
        if (const auto* help = block->findMetadata<base::graph::BlockHelpMetadata>())
            ret->createChild<ui::TextLabel>(help->helpString);

        return ret;
    }

    //--

} // ui
