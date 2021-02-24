/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#include "build.h"

#include "uiScrollBar.h"
#include "uiScrollArea.h"
#include "uiInputAction.h"

#include "base/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

RTTI_BEGIN_TYPE_CLASS(ScrollArea);
    RTTI_METADATA(ElementClassNameMetadata).name("ScrollArea");
RTTI_END_TYPE();

ScrollArea::ScrollArea(ScrollMode vmode /*= ScrollMode::None*/, ScrollMode hmode /*= ScrollMode::None*/)
    : m_scrollOffset(0, 0)
{
    layoutVertical();

    if (vmode != ScrollMode::None)
        verticalScrollMode(vmode);

    if (hmode != ScrollMode::None)
        horizontalScrollMode(hmode);
}

bool ScrollArea::handleMouseWheel(const base::input::MouseMovementEvent& evt, float delta)
{
    /*if (evt.keyMask().isCtrlDown())
    {
        auto zoom = childrenScale();
        if (delta < 0.0f)
            zoom /= 1.1f;
        else if (delta > 0.0f)
            zoom *= 1.1f;

        if (std::fabs(zoom - 1.0f) <= 0.1f)
            zoom = 1.0f;

        childrenScale(zoom);
        return true;
    }
    else*/
    {
        if (m_verticalScrollMode != ScrollMode::None && m_verticalScrollBar)
            return ((IElement*)m_verticalScrollBar.get())->handleMouseWheel(evt, delta);

        else if (m_horizontalScrollMode != ScrollMode::None && m_horizontalScrollBar)
            return ((IElement*)m_horizontalScrollBar.get())->handleMouseWheel(evt, delta);
    }

    return TBaseClass::handleMouseWheel(evt, delta);
}

bool ScrollArea::handleTemplateProperty(base::StringView name, base::StringView value)
{
    if (name == "verticalScroll")
    {
        if (value == "none")
        {
            verticalScrollMode(ScrollMode::None);
            return true;
        }
        else if (value == "auto")
        {
            verticalScrollMode(ScrollMode::Auto);
            return true;
        }
        else if (value == "hidden")
        {
            verticalScrollMode(ScrollMode::Hidden);
            return true;
        }
        else if (value == "always")
        {
            verticalScrollMode(ScrollMode::Always);
            return true;
        }
        return false;
    }
    else if (name == "horizontalScroll")
    {
        if (value == "none")
        {
            horizontalScrollMode(ScrollMode::None);
            return true;
        }
        else if (value == "auto")
        {
            horizontalScrollMode(ScrollMode::Auto);
            return true;
        }
        else if (value == "hidden")
        {
            horizontalScrollMode(ScrollMode::Hidden);
            return true;
        }
        else if (value == "always")
        {
            horizontalScrollMode(ScrollMode::Always);
            return true;
        }
        return false;
    }

    return TBaseClass::handleTemplateProperty(name, value);
}

void ScrollArea::verticalScrollMode(ScrollMode mode)
{
    if (m_verticalScrollMode != mode)
    {
        m_verticalScrollMode = mode;

        if (!m_verticalScrollBar)
        {
            m_verticalScrollBar = base::RefNew<Scrollbar>(Direction::Vertical, true);
            m_verticalScrollBar->persistentElementFlag(true);
            m_verticalScrollBar->ignoredInAutomaticLayout(true);
            m_verticalScrollBar->bind(EVENT_SCROLL) = [this](float pos) { m_scrollOffset.y = pos; };

            attachChild(m_verticalScrollBar);
            hitTest(true);
        }
    }
}

void ScrollArea::horizontalScrollMode(ScrollMode mode)
{
    if (m_horizontalScrollMode != mode)
    {
        m_horizontalScrollMode = mode;

        if (!m_horizontalScrollBar)
        {
            m_horizontalScrollBar = base::RefNew<Scrollbar>(Direction::Horizontal, true);
            m_horizontalScrollBar->persistentElementFlag(true);
            m_horizontalScrollBar->ignoredInAutomaticLayout(true);
            m_horizontalScrollBar->bind(EVENT_SCROLL) = [this](float pos) { m_scrollOffset.x = pos; };
            attachChild(m_horizontalScrollBar);
        }
    }
}

void ScrollArea::scrollOffset(const Position& offset) const
{
    if (offset.x != m_scrollOffset.x)
    {
        m_scrollOffset.x = offset.x;

        if (m_horizontalScrollBar)
            m_horizontalScrollBar->scrollPosition(m_scrollOffset.x, false);
    }

    if (offset.y != m_scrollOffset.y)
    {
        m_scrollOffset.y = offset.y;

        if (m_verticalScrollBar)
            m_verticalScrollBar->scrollPosition(m_scrollOffset.y, false);
    }
}

void ScrollArea::scrollToMakeElementVisible(IElement* element)
{
    m_requestedScrollToElement = element;
}

void ScrollArea::scrollToMakeAreaVisible(const ElementArea& showArea) const
{
    if (!showArea.empty())
    {
        const auto& thisArea = cachedDrawArea();

        auto newPos = m_scrollOffset;

        if (m_horizontalScrollMode != ScrollMode::None)
        {
            if (showArea.left() < thisArea.left())
                newPos.x -= (thisArea.left() - showArea.left());
            else if (showArea.right() > thisArea.right())
                newPos.x += (showArea.right() - thisArea.right());
        }

        if (m_verticalScrollMode != ScrollMode::None)
        {
            if (showArea.top() < thisArea.top())
                newPos.y -= (thisArea.top() - showArea.top());
            else if (showArea.bottom() > thisArea.bottom())
                newPos.y += (showArea.bottom() - thisArea.bottom());
        }

        if (newPos != m_scrollOffset)
            scrollOffset(newPos);
    }
}

void ScrollArea::computeSize(Size& outSize) const
{
    TBaseClass::computeSize(outSize);

    /*auto hasVerticalScrollbar = m_scrollBarState & 2;
    if (m_horizontalScrollBar && hasVerticalScrollbar)
        outSize.y += m_horizontalScrollBar->cachedLayoutParams().calcTotalSize().y;*/
}

void ScrollArea::arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const
{
    // short-circuit for cases when no scrollbars are enabled
    if (m_verticalScrollMode == ScrollMode::None && m_horizontalScrollMode == ScrollMode::None)
    {
        TBaseClass::arrangeChildren(innerArea, clipArea, outArrangedChildren, dynamicSizing);
        return;
    }

    // element self reported size
    Size elementSize;
    computeSize(elementSize);

    // process the layout
    bool retry = false;
    auto usableInnerArea = innerArea;
    Size totalUsedSize = Size(0,0);
    uint8_t neededScrollBars = 0;
    do
    {
        usableInnerArea = innerArea;
        retry = false;

        // adjust size of inner area considering currently visible bars
        auto hasHorizontalScollbar = m_scrollBarState & 1;
        auto hasVerticalScrollbar = m_scrollBarState & 2;
        if (hasVerticalScrollbar)
            usableInnerArea = usableInnerArea.horizontalSice(usableInnerArea.left(), usableInnerArea.right() - m_verticalScrollBar->cachedLayoutParams().calcTotalSize().x);
        if (hasHorizontalScollbar)
            usableInnerArea = usableInnerArea.verticalSlice(usableInnerArea.top(), usableInnerArea.bottom() - m_horizontalScrollBar->cachedLayoutParams().calcTotalSize().y);

        // minimal size is the self reported size of the element
        totalUsedSize = elementSize;

        // do a first pass
        neededScrollBars = 0;
        outArrangedChildren.reset();
        auto usableClipArea = clipArea.clipTo(usableInnerArea);
        arrangeChildrenInner(usableInnerArea, usableClipArea, outArrangedChildren, dynamicSizing, totalUsedSize, neededScrollBars);

        // decide if we should show/hide the scrollbars
        auto horizontalScrollBarVisible = (m_horizontalScrollMode == ScrollMode::Always) || (m_horizontalScrollMode == ScrollMode::Auto && (neededScrollBars & 1));
        auto verticalScrollBarVisible = (m_verticalScrollMode == ScrollMode::Always) || (m_verticalScrollMode == ScrollMode::Auto && (neededScrollBars & 2));

        // show/hide the horizontal bar
        if (horizontalScrollBarVisible && !hasHorizontalScollbar)
        {
            m_scrollBarState |= 1;
            retry = true;
        }
        else if (!horizontalScrollBarVisible && hasHorizontalScollbar)
        {
            m_scrollBarState &= ~1;
            retry = true;
        }

        // show/hide the vertical bar
        if (verticalScrollBarVisible && !hasVerticalScrollbar)
        {
            m_scrollBarState |= 2;
            retry = true;
        }
        else if (!verticalScrollBarVisible && hasVerticalScrollbar)
        {
            m_scrollBarState &= ~2;
            retry = true;
        }
    } while (retry);

    // adjust the scroll positions so they are in range
    auto maxX = std::max<float>(0.0f, totalUsedSize.x - usableInnerArea.size().x);
    auto maxY = std::max<float>(0.0f, totalUsedSize.y - usableInnerArea.size().y);
    m_scrollOffset.x = std::clamp<float>(m_scrollOffset.x, 0.0f, maxX);
    m_scrollOffset.y = std::clamp<float>(m_scrollOffset.y, 0.0f, maxY);

    // if we requested to scroll to given element and it has the proper area computed finally then do the scrolling
    if (m_requestedScrollToElement)
    {
        if (auto element = m_requestedScrollToElement.lock())
        {
            const auto area = element->cachedDrawArea();
            if (!area.empty())
            {
                scrollToMakeAreaVisible(area);
            }
        }

        m_requestedScrollToElement.reset();
    }

    // ok, adjust the positions of the bars
    if (m_verticalScrollBar)
    {
        m_verticalScrollBar->scrollAreaSize(totalUsedSize.y);
        m_verticalScrollBar->scrollThumbSize(usableInnerArea.size().y);
        m_verticalScrollBar->scrollPosition(m_scrollOffset.y, false);

        if (m_scrollBarState & 2)
            outArrangedChildren.add(m_verticalScrollBar, usableInnerArea.horizontalSice(usableInnerArea.right(), usableInnerArea.right() + m_verticalScrollBar->cachedLayoutParams().calcTotalSize().x), clipArea);

    }

    if (m_horizontalScrollBar)
    {
        m_horizontalScrollBar->scrollAreaSize(totalUsedSize.x);
        m_horizontalScrollBar->scrollThumbSize(usableInnerArea.size().x);
        m_horizontalScrollBar->scrollPosition(m_scrollOffset.x, false);

        if (m_scrollBarState & 1)
            outArrangedChildren.add(m_horizontalScrollBar, usableInnerArea.verticalSlice(usableInnerArea.bottom(), usableInnerArea.bottom() + m_horizontalScrollBar->cachedLayoutParams().calcTotalSize().y), clipArea);
    }

    // move all arranged items
    if (m_scrollOffset.x != 0.0f || m_scrollOffset.y != 0.0f)
        outArrangedChildren.shift(-m_scrollOffset.x, -m_scrollOffset.y);
}

//--

END_BOOMER_NAMESPACE(ui)