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
#include "uiTextLabel.h"
#include "uiStyleValue.h"

#include "base/graph/include/graphBlock.h"
#include "base/graph/include/graphSocket.h"
#include "base/graph/include/graphConnection.h"
#include "base/font/include/fontInputText.h"
#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/font/include/fontGlyphBuffer.h"
#include "base/canvas/include/canvas.h"

namespace ui
{
    //--

    base::ConfigProperty<base::Point> cvGraphEditorBlockCaptionMargin("Editor.GraphEditor", "BlockCaptionMargin", base::Point(15, 3));
    base::ConfigProperty<base::Point> cvGraphEditorBlockVerticalSocketMargin("Editor.GraphEditor", "VerticalSocketMargin", base::Point(4, 5));
    base::ConfigProperty<base::Point> cvGraphEditorBlockHorizontalSocketMargin("Editor.GraphEditor", "HorizontalSocketMargin", base::Point(4, 5));
    base::ConfigProperty<base::Point> cvGraphEditorBlockInnerAreaMargin("Editor.GraphEditor", "HorizontalSocketMargin", base::Point(5, 4));
    base::ConfigProperty<base::Point> cvGraphEditorBlockConnectorSize("Editor.GraphEditor", "SocketSize", base::Point(10, 8));
    base::ConfigProperty<int> cvGraphEditorBlockVerticalSeparation("Editor.GraphEditor", "VerticalSeparation", 4);
    base::ConfigProperty<int> cvGraphEditorBlockHorizontalSeparation("Editor.GraphEditor", "HorizontalSeparation", 6);
    base::ConfigProperty<float> cvGraphEditorBlockCircleRadius("Editor.GraphEditor", "CircleRadius", 48);

    //--

    struct TextSizeHelper
    {
        TextSizeHelper(const GraphEditorBlockNode& elem)
            : m_font(elem.evalStyleRef<style::FontFamily>("font-family"_id))
        {
            m_size = std::max<uint32_t>(1, std::floorf(elem.evalStyleValue<float>("font-size"_id, 14.0f) * elem.cachedStyleParams().pixelScale));
            m_color = elem.evalStyleValue<base::Color>("color"_id);
        }

        base::Rect measureText(base::StringView text) const
        {
           base::font::FontStyleParams params;
           params.size = m_size;
           base::font::FontAssemblyParams aparams;
           base::font::FontInputText fontText(text.data(), text.length());
           base::font::FontMetrics metrics;
           m_font.normal->measureText(params, aparams, fontText, metrics);

           base::Rect rect;
           rect.min.x = 0;
           rect.max.x = metrics.textWidth;
           rect.min.y = -(int)std::floor(m_font.normal->relativeAscender() * m_size);
           rect.max.y = -(int)std::floor(m_font.normal->relativeDescender() * m_size);
           return rect;
       }

        void print(base::canvas::GeometryBuilder& builder, base::StringView text) const
        {
            base::font::FontStyleParams params;
            params.size = m_size;
            base::font::FontAssemblyParams aparams;
            aparams.verticalAlignment = base::font::FontAlignmentVertical::Baseline;
            base::font::FontInputText fontText(text.data(), text.length());

            base::font::GlyphBuffer glyphs;
            m_font.normal->renderText(params, aparams, fontText, glyphs);

            builder.print(glyphs);
        }

        INLINE const base::Color& color() const
        {
            return m_color;
        }

    private:
        const style::FontFamily& m_font;
        uint32_t m_size = 12;
        base::Color m_color;
    };   

    struct SocketPlacementInfo
    {
        const base::graph::Socket* socket = nullptr;
        base::Rect textRect;
        Size textSize;
        float placementMin = 0.0f;
        float placementMax = 0.0f;
        float placementCenter = 0.0f;
        float placementTextBaseline = 0.0f;

        INLINE SocketPlacementInfo(const base::graph::Socket* s)
            : socket(s)
        {}
    };

    struct SocketGroupingHelper
    {
        SocketGroupingHelper(const base::graph::Block& block)
        {
            for (const auto& socket : block.sockets())
            {
                if (socket->info().m_visibleByDefault)
                {
                    switch (socket->info().m_placement)
                    {
                        case base::graph::SocketPlacement::Top: top.emplaceBack(socket); break;
                        case base::graph::SocketPlacement::Bottom: bottom.emplaceBack(socket); break;
                        case base::graph::SocketPlacement::Left: left.emplaceBack(socket); break;
                        case base::graph::SocketPlacement::Right: right.emplaceBack(socket); break;
                        case base::graph::SocketPlacement::Title: title.emplaceBack(socket); break;
                        case base::graph::SocketPlacement::Center: center.emplaceBack(socket); break;
                    }
                }
            }
        }

        base::InplaceArray<SocketPlacementInfo, 16> left;
        base::InplaceArray<SocketPlacementInfo, 16> right;
        base::InplaceArray<SocketPlacementInfo, 16> top;
        base::InplaceArray<SocketPlacementInfo, 16> bottom;
        base::InplaceArray<SocketPlacementInfo, 2> title;
        base::InplaceArray<SocketPlacementInfo, 2> center;
    };

    //--

    void GraphEditorBlockNode::refreshBlockStyle()
    {
        m_title->text(m_block->chooseTitle());

        if (const auto* customStyleMetaData = m_block->cls()->findMetadata<base::graph::BlockStyleNameMetadata>())
            if (!customStyleMetaData->name.empty())
                styleType(customStyleMetaData->name);

        switch (m_block->chooseBlockShape())
        {
            case base::graph::BlockShape::RectangleWithTitle:
            case base::graph::BlockShape::RoundedWithTitle:
            case base::graph::BlockShape::SlandedWithTitle:
                addStyleClass("title"_id);
                removeStyleClass("notitle"_id);
                removeStyleClass("circle"_id);
                break;

            case base::graph::BlockShape::Rectangle:
            case base::graph::BlockShape::Rounded:
            case base::graph::BlockShape::Slanded:
                removeStyleClass("title"_id);
                addStyleClass("notitle"_id);
                removeStyleClass("circle"_id);
                break;

            case base::graph::BlockShape::Circle:
            case base::graph::BlockShape::Octagon:
                removeStyleClass("title"_id);
                removeStyleClass("notitle"_id);
                addStyleClass("circle"_id);
                break;
        }

        invalidateLayout();
    }

    void GraphEditorBlockNode::refreshBlockSockets()
    {
        refreshBlockStyle();
    }

    void GraphEditorBlockNode::computeSize(Size& outSize) const
    {
        // inner content size
        const auto payloadSize = m_payload ? m_payload->cachedLayoutParams().calcTotalSize() : Size(0,0);

        // setup layout info
        m_layout = GraphEditorNodeInternalLayout();
        m_layout.valid = true;
        m_layout.hasTitle = false;
        m_layout.shape = m_block->chooseBlockShape();
        m_layout.titleColor = evalStyleValue<base::Color>("title-color"_id, m_block->chooseTitleColor());
        m_layout.borderColor = evalStyleValue<base::Color>("border-color"_id, m_block->chooseBorderColor());

        // switch by layout type
        switch (m_layout.shape)
        {
            case base::graph::BlockShape::RectangleWithTitle:
            case base::graph::BlockShape::SlandedWithTitle:
            case base::graph::BlockShape::RoundedWithTitle:
                m_layout.hasTitle = true;
                outSize = computeLayoutWithTitle(payloadSize, 0.0f, m_layout);
                break;

            case base::graph::BlockShape::Rectangle:
                outSize = computeLayoutNoTitle(payloadSize, 0.0f, false, m_layout);
                break;

            case base::graph::BlockShape::Slanded:
            case base::graph::BlockShape::Rounded:
                outSize = computeLayoutNoTitle(payloadSize, 0.0f, true, m_layout);
                break;

            case base::graph::BlockShape::Circle:
            case base::graph::BlockShape::Octagon:
                outSize = computeLayoutCircleLike(payloadSize, 0.0f, m_layout);
                break;
        }

        updateTrackedSockedPositions();
    }

    static Size ComputeSocketSizeVertical(base::Array<SocketPlacementInfo>& sockets, const TextSizeHelper& textSizeHelper, float pixelScale)
    {
        Size ret(0, 0);

        bool hasSockets = false;
        if (!sockets.empty())
        {
            ret.y += cvGraphEditorBlockVerticalSocketMargin.get().y * pixelScale;

            for (auto& entry : sockets)
            {
                float socketSize = 0.0f;
                if (entry.socket->info().m_displayCaption)
                {
                    entry.textRect = textSizeHelper.measureText(entry.socket->name().view());
                    entry.textSize.x = entry.textRect.width();
                    entry.textSize.y = entry.textRect.height();
                    socketSize = entry.textSize.y;
                }
                else
                {
                    entry.textRect = base::Rect();
                    entry.textSize.x = 0;
                    entry.textSize.y = 0;
                    socketSize = 1.1f * cvGraphEditorBlockConnectorSize.get().y * pixelScale;
                }

                if (hasSockets)
                    ret.y += cvGraphEditorBlockVerticalSeparation.get() * pixelScale;

                entry.placementMin = ret.y;
                entry.placementTextBaseline = ret.y - entry.textRect.min.y;
                entry.placementMax = ret.y + socketSize;
                entry.placementCenter = ret.y + socketSize * 0.5f;

                ret.x = std::max<float>(ret.x, entry.textSize.x);
                ret.y += socketSize;
                hasSockets = true;
            }

            ret.y += cvGraphEditorBlockVerticalSocketMargin.get().y * pixelScale;
        }

        return ret;
    }

    static Size ComputeSocketSizeHorizontal(base::Array<SocketPlacementInfo>& sockets, const TextSizeHelper& textSizeHelper, float pixelScale, bool* outHasCaptions = nullptr)
    {
        Size ret(0, 0);

        bool hasSockets = false;
        if (!sockets.empty())
        {
            ret.x += cvGraphEditorBlockHorizontalSocketMargin.get().x * pixelScale;

            for (auto& entry : sockets)
            {
                float socketSize = 0.0f;
                if (entry.socket->info().m_displayCaption && !entry.socket->name().empty())
                {
                    entry.textRect = textSizeHelper.measureText(entry.socket->name().view());
                    entry.textSize.x = entry.textRect.width();
                    entry.textSize.y = entry.textRect.height();
                    socketSize = entry.textSize.x;

                    if (outHasCaptions)
                        *outHasCaptions = true;
                }
                else
                {
                    entry.textRect = base::Rect();
                    entry.textSize.x = 0;
                    entry.textSize.y = 0;
                    socketSize = 1.1f * cvGraphEditorBlockConnectorSize.get().y * pixelScale;
                }

                if (hasSockets)
                    ret.x += cvGraphEditorBlockHorizontalSeparation.get() * pixelScale;

                entry.placementMin = ret.x;
                entry.placementMax = ret.x + socketSize;
                entry.placementCenter = ret.x + socketSize * 0.5f;

                ret.x += socketSize;
                ret.y = std::max<float>(ret.y, entry.textSize.y);
                
                hasSockets = true;
            }

            for (auto& entry : sockets)
            {
                entry.placementTextBaseline = -entry.textRect.max.y;
            }

            ret.x += cvGraphEditorBlockHorizontalSocketMargin.get().y * pixelScale;
        }

        return ret;
    }

    static void ComputeFinalSocketPlacementRight(const base::Array<SocketPlacementInfo>& sockets, float right, float paddedRight, float top, GraphEditorNodeInternalLayout& outLayout, float pixelScale)
    {
        for (const auto& entry : sockets)
        {
            GrapgEditorSocketInternalLayout socketLayout;

            if (entry.textSize.y && entry.textSize.x)
            {
                socketLayout.textLabel = base::StringBuf(entry.socket->name().view());
                socketLayout.textPlacement.x = paddedRight - entry.textSize.x;
                socketLayout.textPlacement.y = top + entry.placementTextBaseline;
            }

            socketLayout.ptr = entry.socket;
            socketLayout.socketSize = cvGraphEditorBlockConnectorSize.get().toVector() * pixelScale;
            socketLayout.socketOffset.x = right;
            socketLayout.socketOffset.y = top + entry.placementCenter - (socketLayout.socketSize.y * 0.5f);

            socketLayout.linkPoint.x = socketLayout.socketOffset.x + socketLayout.socketSize.x;
            socketLayout.linkPoint.y = socketLayout.socketOffset.y + (socketLayout.socketSize.y * 0.5f);
            socketLayout.linkDir.x = 1.0f;
            socketLayout.linkDir.y = 0.0f;

            socketLayout.socketPlacement = base::graph::SocketPlacement::Right;
            socketLayout.socketShape = entry.socket->info().m_shape;
            socketLayout.socketColor = entry.socket->info().m_socketColor;
            socketLayout.linkColor = entry.socket->info().m_linkColor;

            outLayout.sockets[entry.socket] = socketLayout;
        }
    }

    static void ComputeFinalSocketPlacementLeft(const base::Array<SocketPlacementInfo>& sockets, float left, float paddedLeft, float top, GraphEditorNodeInternalLayout& outLayout, float pixelScale)
    {
        for (const auto& entry : sockets)
        {
            GrapgEditorSocketInternalLayout socketLayout;

            if (entry.textSize.y && entry.textSize.x)
            {
                socketLayout.textLabel = base::StringBuf(entry.socket->name().view());
                socketLayout.textPlacement.x = paddedLeft;
                socketLayout.textPlacement.y = top + entry.placementTextBaseline;
            }

            socketLayout.ptr = entry.socket;
            socketLayout.socketSize = cvGraphEditorBlockConnectorSize.get().toVector() * pixelScale;
            socketLayout.socketOffset.x = left - socketLayout.socketSize.x;
            socketLayout.socketOffset.y = top + entry.placementCenter - (socketLayout.socketSize.y * 0.5f);

            socketLayout.linkPoint.x = socketLayout.socketOffset.x;// +(socketLayout.socketSize.x * 0.5f);
            socketLayout.linkPoint.y = socketLayout.socketOffset.y + (socketLayout.socketSize.y * 0.5f);
            socketLayout.linkDir.x = -1.0f;
            socketLayout.linkDir.y = 0.0f;

            socketLayout.socketPlacement = base::graph::SocketPlacement::Left;
            socketLayout.socketShape = entry.socket->info().m_shape;
            socketLayout.socketColor = entry.socket->info().m_socketColor;
            socketLayout.linkColor = entry.socket->info().m_linkColor;

            outLayout.sockets[entry.socket] = socketLayout;
        }
    }

    static void ComputeFinalSocketPlacementBottom(const base::Array<SocketPlacementInfo>& sockets, float bottom, float paddedBottom, float left, GraphEditorNodeInternalLayout& outLayout, float pixelScale)
    {
        for (const auto& entry : sockets)
        {
            GrapgEditorSocketInternalLayout socketLayout;

            if (entry.textSize.y && entry.textSize.x)
            {
                socketLayout.textLabel = base::StringBuf(entry.socket->name().view());
                socketLayout.textPlacement.x = left + entry.placementCenter - (entry.textSize.x * 0.5f);
                socketLayout.textPlacement.y = paddedBottom + entry.placementTextBaseline;
            }

            socketLayout.ptr = entry.socket;
            socketLayout.socketSize = cvGraphEditorBlockConnectorSize.get().toVector() * pixelScale;
            std::swap(socketLayout.socketSize.x, socketLayout.socketSize.y);
            socketLayout.socketOffset.x = left + entry.placementCenter - (socketLayout.socketSize.x * 0.5f);
            socketLayout.socketOffset.y = bottom;

            socketLayout.linkPoint.x = socketLayout.socketOffset.x + (socketLayout.socketSize.x * 0.5f);
            socketLayout.linkPoint.y = socketLayout.socketOffset.y + socketLayout.socketSize.y;
            socketLayout.linkDir.x = 0.0f;
            socketLayout.linkDir.y = 1.0f;

            socketLayout.socketPlacement = base::graph::SocketPlacement::Bottom;
            socketLayout.socketShape = entry.socket->info().m_shape;
            socketLayout.socketColor = entry.socket->info().m_socketColor;
            socketLayout.linkColor = entry.socket->info().m_linkColor;

            outLayout.sockets[entry.socket] = socketLayout;
        }
    }

    static void ComputeFinalSocketPlacementTop(const base::Array<SocketPlacementInfo>& sockets, float top, float paddedTop, float left, GraphEditorNodeInternalLayout& outLayout, float pixelScale)
    {
        for (const auto& entry : sockets)
        {
            GrapgEditorSocketInternalLayout socketLayout;

            if (entry.textSize.y && entry.textSize.x)
            {
                socketLayout.textLabel = base::StringBuf(entry.socket->name().view());
                socketLayout.textPlacement.x = left + entry.placementCenter - (entry.textSize.x * 0.5f);
                socketLayout.textPlacement.y = paddedTop + entry.placementTextBaseline;
            }

            socketLayout.ptr = entry.socket;
            socketLayout.socketSize = cvGraphEditorBlockConnectorSize.get().toVector() * pixelScale;
            std::swap(socketLayout.socketSize.x, socketLayout.socketSize.y);
            socketLayout.socketOffset.x = left + entry.placementCenter - (socketLayout.socketSize.x * 0.5f);
            socketLayout.socketOffset.y = top - socketLayout.socketSize.y;

            socketLayout.linkPoint.x = socketLayout.socketOffset.x + (socketLayout.socketSize.x * 0.5f);
            socketLayout.linkPoint.y = socketLayout.socketOffset.y;
            socketLayout.linkDir.x = 0.0f;
            socketLayout.linkDir.y = -1.0f;

            socketLayout.socketPlacement = base::graph::SocketPlacement::Top;
            socketLayout.socketShape = entry.socket->info().m_shape;
            socketLayout.socketColor = entry.socket->info().m_socketColor;
            socketLayout.linkColor = entry.socket->info().m_linkColor;

            outLayout.sockets[entry.socket] = socketLayout;
        }
    }

    Size GraphEditorBlockNode::computeLayoutWithTitle(const Size& innerContentSize, float innerPadding, GraphEditorNodeInternalLayout& outLayout) const
    {
        // calculate title content size
        const auto pixelScale = cachedStyleParams().pixelScale;
        const auto titleSize = m_title->cachedLayoutParams().calcTotalSize();

        // padding
        float paddingLeft = evalStyleValue<float>("padding-left"_id, 0.0f) * pixelScale;
        float paddingRight = evalStyleValue<float>("padding-right"_id, 0.0f) * pixelScale;
        float paddingBottom = evalStyleValue<float>("padding-bottom"_id, 0.0f) * pixelScale;

        // calculate clean area size
        float clientAreaWidth = evalStyleValue<float>("min-width"_id, 20.0f) * pixelScale;;
        float clientAreaHeight = evalStyleValue<float>("min-height"_id, 20.0f) * pixelScale;;

        // socket layouts
        bool bottomSocketsHaveCaption = false;
        TextSizeHelper textSizeHelper(*this);
        SocketGroupingHelper sockets(*m_block);
        const auto leftSocketsSize = ComputeSocketSizeVertical(sockets.left, textSizeHelper, pixelScale);
        const auto rightSocketsSize = ComputeSocketSizeVertical(sockets.right, textSizeHelper, pixelScale);
        const auto bottomSocketSize = ComputeSocketSizeHorizontal(sockets.bottom, textSizeHelper, pixelScale, &bottomSocketsHaveCaption);

        // add space for the sockets to the block area
        if (leftSocketsSize.x > 0 || rightSocketsSize.x > 0)
        {
            // accommodate place for both left and right sockets
            clientAreaWidth = leftSocketsSize.x + rightSocketsSize.x;

            // bottom sockets are placed below normal sockets so they won't overlap in the X
            clientAreaWidth = std::max<float>(clientAreaWidth, bottomSocketSize.x);

            // get larger of heights
            clientAreaHeight = std::max<float>(leftSocketsSize.y, rightSocketsSize.y);
            clientAreaHeight += bottomSocketSize.y;

            // extra margin between sides
            clientAreaWidth += 4 * pixelScale;

            // if both left and right sockets are visible add a small margin
            if (leftSocketsSize.x && rightSocketsSize.x)
                clientAreaWidth += 4 * pixelScale;
        }
        // there are no sockets on left or right but there still may be sockets at the bottom
        else
        {
            if (bottomSocketsHaveCaption)
                clientAreaHeight += (2.0f * cvGraphEditorBlockHorizontalSocketMargin.get().y) * pixelScale;
        }

        // inner padding
        //clientAreaWidth += innerPadding * 2.0f;
        //clientAreaHeight += innerPadding * 2.0f;

        // inner area
        if (innerContentSize.x > 0 && innerContentSize.y > 0)
        {
            clientAreaWidth += innerContentSize.x;
            clientAreaHeight = std::max<float>(clientAreaHeight, innerContentSize.y);
        }

        // finally, calculate total size
        float totalWidth = std::max<float>(clientAreaWidth, titleSize.x) + paddingLeft + paddingRight;
        float totalHeight = clientAreaHeight + titleSize.y;

        // place areas
        outLayout.blockSize.x = totalWidth;
        outLayout.blockSize.y = totalHeight;
        outLayout.titleSize.x = totalWidth; // title spans the whole block
        outLayout.titleSize.y = titleSize.y; // title is as high as required

        // place center content
        outLayout.payloadSize = innerContentSize;
        float usableClientAreaWidth = clientAreaWidth - leftSocketsSize.x - rightSocketsSize.x - paddingLeft - paddingRight;
        float usableClientAreaHeight = clientAreaHeight - bottomSocketSize.y - paddingBottom;
        outLayout.payloadOffset.x = paddingLeft + leftSocketsSize.x + (usableClientAreaWidth - innerContentSize.x) / 2.0f;
        outLayout.payloadOffset.y = titleSize.y + (usableClientAreaHeight - innerContentSize.y) / 2.0f;

        // client area
        outLayout.clientOffset.x = paddingLeft;
        outLayout.clientOffset.y = titleSize.y;
        outLayout.clientSize.x = usableClientAreaWidth;
        outLayout.clientSize.y = usableClientAreaHeight;

        // bottom sockets are always centers
        float bottomSocketsOffsetX = (totalWidth - bottomSocketSize.x) * 0.5f;

        // place the sockets
        ComputeFinalSocketPlacementLeft(sockets.left, 0.0f, paddingLeft, titleSize.y, outLayout, pixelScale);
        ComputeFinalSocketPlacementRight(sockets.right, totalWidth, totalWidth - paddingRight, titleSize.y, outLayout, pixelScale);
        ComputeFinalSocketPlacementBottom(sockets.bottom, totalHeight, totalHeight - paddingBottom, bottomSocketsOffsetX, outLayout, pixelScale);

        // return final size computed for the area but without the padding (HACK)
        auto adjSize = outLayout.blockSize;
        adjSize.x -= paddingLeft + paddingRight;
        adjSize.y -= paddingBottom;
        return adjSize;
    }

    Size GraphEditorBlockNode::computeLayoutNoTitle(const Size& innerContentSize, float innerPadding, bool adjustForSlant, GraphEditorNodeInternalLayout& outLayout) const
    {
        // calculate title content size
        const auto pixelScale = cachedStyleParams().pixelScale;
        auto titleSize = m_title->cachedLayoutParams().calcTotalSize();

        // use payload size instead of content
        if (innerContentSize.x > 0 && innerContentSize.y > 0)
            titleSize = innerContentSize;

        // padding
        float paddingLeft = evalStyleValue<float>("padding-left"_id, 0.0f) * pixelScale;
        float paddingRight = evalStyleValue<float>("padding-right"_id, 0.0f) * pixelScale;
        float paddingBottom = evalStyleValue<float>("padding-bottom"_id, 0.0f) * pixelScale;
        float paddingTop = evalStyleValue<float>("padding-top"_id, 0.0f) * pixelScale;

        // calculate clean area size
        float clientAreaWidth = evalStyleValue<float>("min-width"_id, 20.0f) * pixelScale;;
        float clientAreaHeight = evalStyleValue<float>("min-height"_id, 20.0f) * pixelScale;;
        clientAreaWidth = std::max<float>(clientAreaWidth, titleSize.x);
        clientAreaHeight = std::max<float>(clientAreaHeight, titleSize.y);

        // socket layouts
        bool topSocketsHaveCaptions = false;
        bool bottomSocketsHaveCaption = false;
        TextSizeHelper textSizeHelper(*this);
        SocketGroupingHelper sockets(*m_block);
        auto leftSocketsSize = ComputeSocketSizeVertical(sockets.left, textSizeHelper, pixelScale);
        auto rightSocketsSize = ComputeSocketSizeVertical(sockets.right, textSizeHelper, pixelScale);
        auto bottomSocketSize = ComputeSocketSizeHorizontal(sockets.bottom, textSizeHelper, pixelScale, &bottomSocketsHaveCaption);
        auto topSocketSize = ComputeSocketSizeHorizontal(sockets.top, textSizeHelper, pixelScale, &topSocketsHaveCaptions);

        // adjust socket space for slanting
        float slantRadius = evalStyleValue<float>("border-radius"_id, 0.0f) * pixelScale;
        if (adjustForSlant)
        {
            if (leftSocketsSize.y)
                leftSocketsSize.y += slantRadius;
            if (rightSocketsSize.y)
                rightSocketsSize.y += slantRadius;
            if (bottomSocketSize.x)
                bottomSocketSize.x += slantRadius;
            if (topSocketSize.x)
                topSocketSize.x += slantRadius;
        }

        // add space for the sockets to the block area
        if (leftSocketsSize.x > 0 || rightSocketsSize.x > 0)
        {
            // accommodate place for both left and right sockets
            clientAreaWidth = leftSocketsSize.x + rightSocketsSize.x + titleSize.x;

            // bottom sockets are placed below normal sockets so they won't overlap in the X
            clientAreaWidth = std::max<float>(clientAreaWidth, bottomSocketSize.x);
            clientAreaWidth = std::max<float>(clientAreaWidth, topSocketSize.x);

            // get larger of heights
            clientAreaHeight = std::max<float>(leftSocketsSize.y, rightSocketsSize.y);
            clientAreaHeight = std::max<float>(clientAreaHeight, titleSize.y);
            clientAreaHeight += bottomSocketSize.y;
            clientAreaHeight += topSocketSize.y;

            // extra margin between sides
            clientAreaWidth += 4 * pixelScale;

            // if both left and right sockets are visible add a small margin
            if (leftSocketsSize.x && rightSocketsSize.x)
                clientAreaWidth += 4 * pixelScale;
            if (topSocketSize.x && bottomSocketSize.x)
                clientAreaHeight += 4 * pixelScale;
        }
        // there are no sockets on left or right but there still may be sockets at the bottom
        else
        {
            clientAreaHeight = std::max<float>(leftSocketsSize.y, rightSocketsSize.y);
            clientAreaHeight = std::max<float>(clientAreaHeight, titleSize.y);

            if (topSocketsHaveCaptions)
                clientAreaHeight += (2.0f * cvGraphEditorBlockHorizontalSocketMargin.get().y) * pixelScale;

            if (bottomSocketsHaveCaption)
                clientAreaHeight += (2.0f * cvGraphEditorBlockHorizontalSocketMargin.get().y) * pixelScale;
        }

        // inner area
        /*if (innerContentSize.x > 0 && innerContentSize.y > 0)
        {
            //clientAreaWidth += innerContentSize.x;
            clientAreaHeight = std::max<float>(clientAreaHeight, innerContentSize.y + topSocketSize.y + bottomSocketSize.y);
        }*/

        // finally, calculate total size
        float totalWidth = clientAreaWidth + paddingLeft + paddingRight + (innerPadding * 2.0f);
        float totalHeight = clientAreaHeight + paddingTop + paddingBottom + (innerPadding * 2.0f);

        // place areas
        outLayout.blockSize.x = totalWidth;
        outLayout.blockSize.y = totalHeight;
        outLayout.titleSize.x = 0.0f;
        outLayout.titleSize.y = 0.0f;

        // place center content
        outLayout.payloadSize = innerContentSize;
        float usableClientAreaWidth = clientAreaWidth - leftSocketsSize.x - rightSocketsSize.x;
        float usableClientAreaHeight = clientAreaHeight - bottomSocketSize.y - topSocketSize.y;
        outLayout.payloadOffset.x = paddingLeft + innerPadding + leftSocketsSize.x + (usableClientAreaWidth - innerContentSize.x) / 2.0f;
        outLayout.payloadOffset.y = paddingTop + innerPadding + topSocketSize.y + (usableClientAreaHeight - innerContentSize.y) / 2.0f;

        // client area
        outLayout.clientOffset.x = paddingLeft + leftSocketsSize.x + innerPadding;
        outLayout.clientOffset.y = paddingTop + topSocketSize.y + innerPadding;
        outLayout.clientSize.x = usableClientAreaWidth;
        outLayout.clientSize.y = usableClientAreaHeight;

        // bottom sockets are always centers
        float slantMargin = adjustForSlant ? (slantRadius * 0.5f) : 0.0f;
        float bottomSocketsOffsetX = (adjustForSlant ? (slantRadius * 0.5f ) : 0.0f) + (totalWidth - bottomSocketSize.x) * 0.5f;
        float topSocketsOffsetX = (totalWidth - topSocketSize.x) * 0.5f;
        float leftSocketsOffsetY = (totalHeight - leftSocketsSize.y) * 0.5f + slantMargin;
        float rightSocketsOffsetY = (totalHeight - rightSocketsSize.y) * 0.5f + slantMargin;

        // place the sockets
        ComputeFinalSocketPlacementLeft(sockets.left, 0.0f, paddingLeft, leftSocketsOffsetY, outLayout, pixelScale);
        ComputeFinalSocketPlacementRight(sockets.right, totalWidth, totalWidth - paddingRight, rightSocketsOffsetY, outLayout, pixelScale);
        ComputeFinalSocketPlacementBottom(sockets.bottom, totalHeight, totalHeight - paddingBottom, bottomSocketsOffsetX, outLayout, pixelScale);
        ComputeFinalSocketPlacementTop(sockets.top, totalHeight, totalHeight - paddingBottom, topSocketsOffsetX, outLayout, pixelScale);

        // return final size computed for the area but without the padding (HACK)
        auto adjSize = outLayout.blockSize;
        adjSize.x -= paddingLeft + paddingRight;
        adjSize.y -= paddingBottom + paddingTop;
        return adjSize;
    }

    Size GraphEditorBlockNode::computeLayoutCircleLike(const Size& innerContentSize, float innerPadding, GraphEditorNodeInternalLayout& outLayout) const
    {
        const auto pixelScale = cachedStyleParams().pixelScale;

        // padding
        float paddingLeft = evalStyleValue<float>("padding-left"_id, 0.0f) * pixelScale;
        float paddingRight = evalStyleValue<float>("padding-right"_id, 0.0f) * pixelScale;
        float paddingBottom = evalStyleValue<float>("padding-bottom"_id, 0.0f) * pixelScale;
        float paddingTop = evalStyleValue<float>("padding-top"_id, 0.0f) * pixelScale;

        // for circles the area is always the same
        float circleSize = evalStyleValue<float>("width"_id, 48.0f) * pixelScale;

        TextSizeHelper textSizeHelper(*this);
        SocketGroupingHelper sockets(*m_block);
        const auto leftSocketsSize = ComputeSocketSizeVertical(sockets.left, textSizeHelper, pixelScale);
        const auto rightSocketsSize = ComputeSocketSizeVertical(sockets.right, textSizeHelper, pixelScale);
        const auto bottomSocketSize = ComputeSocketSizeHorizontal(sockets.bottom, textSizeHelper, pixelScale);
        const auto topSocketSize = ComputeSocketSizeHorizontal(sockets.top, textSizeHelper, pixelScale);
    
        outLayout.blockSize.x = circleSize;
        outLayout.blockSize.y = circleSize;
        outLayout.titleSize.x = 0;
        outLayout.titleSize.y = 0;
        outLayout.clientOffset.x = 0;
        outLayout.clientOffset.y = 0;
        outLayout.clientSize.x = circleSize;
        outLayout.clientSize.y = circleSize;

        // place center content
        outLayout.payloadSize.x = std::min<float>(innerContentSize.x, circleSize * 0.65f);
        outLayout.payloadSize.y = std::min<float>(innerContentSize.y, circleSize * 0.65f);
        outLayout.payloadOffset.x = (circleSize - outLayout.payloadSize.x) * 0.5f;
        outLayout.payloadOffset.y = (circleSize - outLayout.payloadSize.y) * 0.5f;

        // center all tokens
        float bottomSocketsOffsetX = (circleSize - bottomSocketSize.x) * 0.5f;
        float topSocketsSizeX = (circleSize - topSocketSize.x) * 0.5f;
        float leftSocketsOffsetY = (circleSize - leftSocketsSize.y) * 0.5f;
        float rightSocketsOffsetY = (circleSize - rightSocketsSize.y) * 0.5f;

        // place the sockets
        ComputeFinalSocketPlacementLeft(sockets.left, 0.0f, paddingLeft, leftSocketsOffsetY, outLayout, pixelScale);
        ComputeFinalSocketPlacementRight(sockets.right, circleSize, circleSize - paddingRight, rightSocketsOffsetY, outLayout, pixelScale);
        ComputeFinalSocketPlacementBottom(sockets.bottom, circleSize, circleSize - paddingBottom, bottomSocketsOffsetX, outLayout, pixelScale);
        ComputeFinalSocketPlacementTop(sockets.top, 0.0f, paddingTop, topSocketsSizeX, outLayout, pixelScale);

        // return final size computed for the area but without the padding (HACK)
        /*auto adjSize = outLayout.blockSize;
        adjSize.x -= paddingLeft + paddingRight;
        adjSize.y -= paddingBottom + paddingTop;*/
        return Size(circleSize, circleSize);
    }

    //--

    void GraphEditorBlockNode::prepareBlockOutline(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder, float inset, float titleHeightLimit) const
    {
        DEBUG_CHECK(m_layout.valid);

        float r = evalStyleValue<float>("border-radius"_id, 0.0f) * pixelScale;

        auto ox = drawArea.absolutePosition().x + inset;
        auto oy = drawArea.absolutePosition().y + inset;
        auto sx = drawArea.size().x - inset * 2.0f;
        auto sy = drawArea.size().y - inset * 2.0f;

        switch (m_layout.shape)
        {
            case base::graph::BlockShape::RectangleWithTitle:
            case base::graph::BlockShape::Rectangle:
            {
                if (titleHeightLimit > 0.0f)
                    sy = titleHeightLimit;

                builder.rect(ox, oy, sx, sy);
                break;
            }

            case base::graph::BlockShape::SlandedWithTitle:
            case base::graph::BlockShape::Slanded:
            {
                r -= inset;
                if (titleHeightLimit > 0.0f)
                {
                    builder.moveTo(ox + r, oy);
                    builder.lineTo(ox, oy + r);
                    builder.lineTo(ox, titleHeightLimit);
                    builder.lineTo(ox + sx, titleHeightLimit);
                    builder.lineTo(ox + sx, oy + r);
                    builder.lineTo(ox + sx - r, oy);
                    builder.lineTo(ox + r, oy);
                }
                else
                {
                    builder.moveTo(ox + r, oy);
                    builder.lineTo(ox, oy + r);
                    builder.lineTo(ox, oy + sy - r);
                    builder.lineTo(ox + r, oy + sy);
                    builder.lineTo(ox + sx - r, oy + sy);
                    builder.lineTo(ox + sx, oy + sy - r);
                    builder.lineTo(ox + sx, oy + r);
                    builder.lineTo(ox + sx - r, oy);
                    builder.lineTo(ox + r, oy);
                }
                break;
            }

            case base::graph::BlockShape::RoundedWithTitle:
            case base::graph::BlockShape::Rounded:
            {
                r -= inset * 2.0f;
                if (titleHeightLimit > 0.0f)
                    builder.roundedRectVarying(ox, oy, sx, titleHeightLimit, r, r, 0.0f, 0.0f);
                else
                    builder.roundedRect(ox, oy, sx, sy, r);
                break;
            }

            case base::graph::BlockShape::Circle:
            {
                const auto radX = sx * 0.5f;
                const auto radY = sy * 0.5f;
                builder.ellipse(ox + radX, oy + radY, radX, radY);
                break;
            }

            case base::graph::BlockShape::Octagon:
            {
                const auto xa = (sx / 2.414f) / 1.41421356237f;
                const auto ya = (sy / 2.414f) / 1.41421356237f;
                
                builder.moveTo(ox + xa, oy);
                builder.lineTo(ox, oy + ya);
                builder.lineTo(ox, oy + sy- ya);
                builder.lineTo(ox + xa, oy + sy);
                builder.lineTo(ox + sx - xa, oy + sy);
                builder.lineTo(ox + sx, oy + sy - ya);
                builder.lineTo(ox + sx, oy + ya);
                builder.lineTo(ox + sx - xa, oy);
                builder.lineTo(ox + xa, oy);
                break;
            }
        }
    }

    void GraphEditorBlockNode::prepareBoundaryGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder, float inset) const
    {
        prepareBlockOutline(drawArea, pixelScale, builder, inset, 0.0f);
    }

    static void BuildSocketShape(base::graph::SocketShape shape, const base::Vector2& pos, const base::Vector2& size, base::graph::SocketPlacement dir, base::canvas::GeometryBuilder& builder)
    {
        switch (shape)
        {
            case base::graph::SocketShape::Block:
            {
                builder.beginPath();
                builder.rect(pos.x, pos.y, size.x, size.y);
                builder.fill();
                break;
            }

            case base::graph::SocketShape::Input:
            {
                switch (dir)
                {
                case base::graph::SocketPlacement::Right:
                    builder.beginPath();
                    builder.moveTo(pos.x, pos.y + size.y * 0.5f);
                    builder.lineTo(pos.x + size.x, pos.y + size.y);
                    builder.lineTo(pos.x + size.x, pos.y);
                    builder.fill();
                    break;

                case base::graph::SocketPlacement::Left:
                    builder.beginPath();
                    builder.moveTo(pos.x + size.x, pos.y + size.y * 0.5f);
                    builder.lineTo(pos.x, pos.y);
                    builder.lineTo(pos.x, pos.y + size.y);
                    builder.fill();
                    break;

                case base::graph::SocketPlacement::Bottom:
                    builder.beginPath();
                    builder.moveTo(pos.x + size.x * 0.5f, pos.y);
                    builder.lineTo(pos.x, pos.y + size.y);
                    builder.lineTo(pos.x + size.x, pos.y + size.y);
                    builder.fill();
                    break;

                case base::graph::SocketPlacement::Top:
                    builder.beginPath();
                    builder.moveTo(pos.x + size.x * 0.5f, pos.y + size.y);
                    builder.lineTo(pos.x, pos.y);
                    builder.lineTo(pos.x + size.x, pos.y);
                    builder.fill();
                    break;
                }

                
                break;
            }

            case base::graph::SocketShape::Output:
            {
                switch (dir)
                {
                case base::graph::SocketPlacement::Right:
                    builder.beginPath();
                    builder.moveTo(pos.x + size.x, pos.y + size.y * 0.5f);
                    builder.lineTo(pos.x, pos.y);
                    builder.lineTo(pos.x, pos.y + size.y);
                    builder.fill();
                    break;

                case base::graph::SocketPlacement::Left:
                    builder.beginPath();
                    builder.moveTo(pos.x, pos.y + size.y * 0.5f);
                    builder.lineTo(pos.x + size.x, pos.y + size.y);
                    builder.lineTo(pos.x + size.x, pos.y);
                    builder.fill();
                    break;

                case base::graph::SocketPlacement::Bottom:
                    builder.beginPath();
                    builder.moveTo(pos.x + size.x * 0.5f, pos.y + size.y);
                    builder.lineTo(pos.x, pos.y);
                    builder.lineTo(pos.x + size.x, pos.y);
                    builder.fill();
                    break;

                case base::graph::SocketPlacement::Top:
                    builder.beginPath();
                    builder.moveTo(pos.x + size.x * 0.5f, pos.y);
                    builder.lineTo(pos.x, pos.y + size.y);
                    builder.lineTo(pos.x + size.x, pos.y + size.y);
                    builder.fill();
                    break;
                }


                break;
            }

            case base::graph::SocketShape::Bidirectional:
            {
                switch (dir)
                {
                case base::graph::SocketPlacement::Right:
                case base::graph::SocketPlacement::Left:
                    builder.beginPath();
                    builder.moveTo(pos.x + size.x, pos.y + size.y * 0.10f);
                    builder.lineTo(pos.x, pos.y - size.y * 0.25);
                    builder.lineTo(pos.x, pos.y + size.y * 0.45f);
                    builder.fill();
                    builder.beginPath();
                    builder.moveTo(pos.x, pos.y + size.y * 0.9f);
                    builder.lineTo(pos.x + size.x, pos.y + size.y * 1.25f);
                    builder.lineTo(pos.x + size.x, pos.y + size.y * 0.55f);
                    builder.fill();
                    break;

                case base::graph::SocketPlacement::Top:
                case base::graph::SocketPlacement::Bottom:
                    break;
                }
                break;
            }
        }
    }

    void GraphEditorBlockNode::prepareBackgroundGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const
    {
        // block itself is rendered with normal style
        TBaseClass::prepareBackgroundGeometry(drawArea, pixelScale, builder);

        // title is rendered with title color
        // TODO: title style ?
        if (m_layout.titleSize.y > 0.0f)
        {
            builder.beginPath();
            builder.fillColor(m_layout.titleColor);
            prepareBlockOutline(drawArea, pixelScale, builder, 0.0f, m_layout.titleSize.y);
            builder.fill();
        }

        // add the socket captions
        if (!m_layout.sockets.empty() && pixelScale >= 0.3f)
        {
            TextSizeHelper helper(*this);
            for (const auto& socket : m_layout.sockets.values())
            {
                if (!socket.textLabel.empty())
                {
                    builder.pushTransform();
                    builder.offset(drawArea.absolutePosition().x + socket.textPlacement.x, drawArea.absolutePosition().y + socket.textPlacement.y);
                    builder.fillColor(helper.color());
                    helper.print(builder, socket.textLabel);
                    builder.popTransform();
                }
            }
        }
    }

    bool GraphEditorBlockNode::adjustBackgroundStyle(base::canvas::RenderStyle& outStyle, float& outBorderWidth) const
    {
        if (!m_layout.hasTitle && m_layout.titleColor.a > 0)
        {
            outStyle.innerColor = m_layout.titleColor;
            outStyle.outerColor = m_layout.titleColor * 0.8f;
            return true;
        }

        return false;
    }

    bool GraphEditorBlockNode::adjustBorderStyle(base::canvas::RenderStyle& outStyle, float& outBorderWidth) const
    {
        if (!m_layout.hasTitle && m_layout.borderColor.a > 0)
        {
            if (!hasStyleClass("selected"_id))
            {
                outStyle.innerColor = m_layout.borderColor;
                outStyle.outerColor = m_layout.borderColor;
                return true;
            }
        }

        return false;
    }

    extern ElementArea ArrangeElementLayout(const ElementArea& incomingInnerArea, const ElementLayout& layout, const Size& totalSize);
    extern ElementArea ArrangeAreaLayout(const ElementArea& incomingInnerArea, const Size& totalSize, ElementVerticalLayout verticalAlign, ElementHorizontalLayout horizontalAlign);

    void GraphEditorBlockNode::renderCustomOverlayElements(HitCache& hitCache, const ElementArea& outerArea, const ElementArea& outerClipArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        // title bar
        if (cachedStyleParams().pixelScale > 0.4f)
        {
            if (m_title)
            {
                if (m_layout.titleSize.y > 0)
                {
                    auto titleArea = outerArea.verticalSlice(outerArea.top(), outerArea.top() + m_layout.titleSize.y);
                    auto titlePlacement = ArrangeElementLayout(titleArea, m_title->cachedLayoutParams(), m_layout.titleSize);
                    m_title->render(hitCache, titlePlacement, outerClipArea, canvas, mergedOpacity, nullptr);
                }
                else if (m_layout.clientSize.y > 0 && m_layout.clientSize.x > 0)
                {
                    ElementArea clientArea(outerArea.left() + m_layout.clientOffset.x, outerArea.top() + m_layout.clientOffset.y,
                        outerArea.left() + m_layout.clientOffset.x + m_layout.clientSize.x, outerArea.top() + m_layout.clientOffset.y + m_layout.clientSize.y);

                    auto titlePlacementArea = ArrangeAreaLayout(clientArea, m_title->cachedLayoutParams().calcTotalSize(), ElementVerticalLayout::Middle, ElementHorizontalLayout::Center);
                    m_title->render(hitCache, titlePlacementArea, outerClipArea, canvas, mergedOpacity, nullptr);
                }
            }
        }

        // payload
        if (cachedStyleParams().pixelScale > 0.4f)
        {
            if (m_layout.payloadSize.y > 0.0f && m_payload)
            {
                ElementArea payloadArea(outerArea.left() + m_layout.payloadOffset.x, outerArea.top() + m_layout.payloadOffset.y,
                    outerArea.left() + m_layout.payloadOffset.x + m_layout.payloadSize.x, outerArea.top() + m_layout.payloadOffset.y + m_layout.payloadSize.y);

                auto payloadClipArea = outerClipArea.clipTo(payloadArea);

                canvas.pushScissorRect();
                if (canvas.scissorRect(payloadClipArea.left(), payloadClipArea.top(), payloadClipArea.size().x, payloadClipArea.size().y))
                {
                    m_payload->render(hitCache, payloadArea, payloadClipArea, canvas, mergedOpacity, nullptr);
                    canvas.popScissorRect();
                }
            }
        }
    }

    void GraphEditorBlockNode::prepareShadowGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const
    {
        TBaseClass::prepareShadowGeometry(drawArea, pixelScale, builder);

        // socket shapes
        if (pixelScale >= 0.3f)
        {
            for (const auto& socket : m_layout.sockets.values())
            {
                if (m_hoverSocketPreview == socket.ptr)
                    builder.fillColor(base::Color::YELLOW);
                else
                    builder.fillColor(socket.socketColor);

                BuildSocketShape(socket.socketShape, socket.socketOffset + drawArea.absolutePosition(), socket.socketSize, socket.socketPlacement, builder);
                //builder.rect(drawArea.absolutePosition().x - 20, drawArea.absolutePosition().x - 20, drawArea.size().x + 40, drawArea.size().y + 40);
            }
        }
    }

    extern void GenerateConnectionGeometry(const Position& sourcePos, const base::Vector2& sourceDir, 
		const Position& targetPos, const base::Vector2& targetDir, 
		base::Color color, float width, float pixelScale, bool startArrow, bool endArrow, base::canvas::GeometryBuilder& builder);

    void GraphEditorBlockNode::drawConnections(const ElementArea& outerArea, const ElementArea& outerClipArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
		base::canvas::Geometry geometry;

		const auto pixelWidth = cachedStyleParams().pixelScale;
        if (pixelWidth > 0.15f)
        {
            if (auto editor = base::rtti_cast<GraphEditor>(parent()))
            {
				base::canvas::GeometryBuilder builder(nullptr, geometry);

				for (const auto& socket : m_layout.sockets.values())
                {
                    for (const auto& con : socket.ptr->connections())
                    {
                        // draw only connections that originate from this block
                        // NOTE: connection object itself is SHARED between two sockes so we only need one socket to draw it
                        if (con->first() != socket.ptr)
                            continue;

                        // get source position
                        Position sourcePos(0, 0), sourceDir(0, 0);
                        if (editor->calcSocketLinkPlacement(con->first(), sourcePos, sourceDir))
                        {
                            Position targetPos(0, 0), targetDir(0, 0);
                            if (editor->calcSocketLinkPlacement(con->second(), targetPos, targetDir))
                            {
                                // bbox test
                                const auto minX = std::min<float>(sourcePos.x, targetPos.x);
                                const auto minY = std::min<float>(sourcePos.y, targetPos.y);
                                const auto maxX = std::max<float>(sourcePos.x, targetPos.x);
                                const auto maxY = std::max<float>(sourcePos.y, targetPos.y);
                                if (maxX >= outerClipArea.left() && minX <= outerClipArea.right() && maxY >= outerClipArea.top() && minY <= outerClipArea.bottom())
                                {
                                    GenerateConnectionGeometry(sourcePos, sourceDir, targetPos, targetDir, 
										con->first()->info().m_linkColor, 2.0f, pixelWidth, false, false, builder);
                                }
                            }
                        }
                    }
                }
            }
        }

		canvas.place(base::canvas::Placement(), geometry, mergedOpacity);
    }

    //--

} // ui
