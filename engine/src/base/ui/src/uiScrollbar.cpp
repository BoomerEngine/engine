/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#include "build.h"
#include "uiScrollBar.h"
#include "uiInputAction.h"
#include "uiElement.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiStyleValue.h"
#include "uiTextLabel.h"

#include "base/input/include/inputStructures.h"
#include "base/canvas/include/canvasGeometry.h"

namespace ui
{

    //--

    RTTI_BEGIN_TYPE_CLASS(Scrollbar);
        RTTI_METADATA(ElementClassNameMetadata).name("ScrollBar");
    RTTI_END_TYPE();

    Scrollbar::Scrollbar(Direction direction /*= Direction::Vertical*/, bool createButtons /*= true*/)
        : m_direction(direction)
        , m_scrollPosition(0.0f)
        , m_scrollThumbSize(64.0f)
        , m_scrollAreaSize(256.0f)
        , m_smallStepSize(16.0f)
        , m_largeStepSize(64.0f)
        , m_cachedScrollAreaPixelDelta(1.0f)
        , m_hasButtons(createButtons)
    {
        // allow clicks
        hitTest(HitTestState::Enabled);

        // set sub-style based on direction
        addStyleClass(direction == Direction::Vertical ? "vertical"_id : "horizontal"_id);

        // auto expand
        enableAutoExpand(direction == Direction::Horizontal, direction == Direction::Vertical);

        // create the tracking thumb
        m_thumb = createChild<ScrollbarThumb>(this);

        // create the buttons
        if (createButtons)
        {
            if (direction == Direction::Vertical)
            {
                m_backwardButton = createNamedChild<Button>("up"_id, "");
                m_forwardButton = createNamedChild<Button>("down"_id, "");
            }
            else
            {
                m_backwardButton = createNamedChild<Button>("left"_id, "");
                m_forwardButton = createNamedChild<Button>("right"_id, "");
            }

            m_backwardButton->bind(EVENT_CLICKED, this) = [](Scrollbar* bar)
            {
                bar->scrollPosition(bar->scrollPosition() - bar->m_smallStepSize);
            };

            m_forwardButton->bind(EVENT_CLICKED, this) = [](Scrollbar* bar)
            {
                bar->scrollPosition(bar->scrollPosition() + bar->m_smallStepSize);
            };
        }
    }

    Scrollbar::~Scrollbar()
    {}

    void Scrollbar::scrollAreaSize(float size)
    {
        auto allowedSize = std::max<float>(0.0f, size);
        if (allowedSize != m_scrollAreaSize)
        {
            m_scrollAreaSize = allowedSize;
            invalidateGeometry();
        }
    }

    void Scrollbar::scrollThumbSize(float size)
    {
        auto allowedSize = std::clamp<float>(size, 0.0f, m_scrollAreaSize);
        if (allowedSize != m_scrollThumbSize)
        {
            m_scrollThumbSize = allowedSize;
            invalidateGeometry();
        }
    }

    void Scrollbar::scrollPosition(float position, bool callEvent)
    {
        auto maxPositionLeft = std::max<float>(0.0f, m_scrollAreaSize - m_scrollThumbSize);
        auto allowedPosition = std::clamp<float>(position, 0.0f, maxPositionLeft);
        if (allowedPosition != m_scrollPosition)
        {
            m_scrollPosition = allowedPosition;
            invalidateGeometry();

            call(EVENT_SCROLL, m_scrollPosition);
        }
    }

    void Scrollbar::smallStepSize(float smallStepSize)
    {
        m_smallStepSize = smallStepSize;
    }

    void Scrollbar::largeStepSize(float largeStepSize)
    {
        m_largeStepSize = largeStepSize;
    }

    void Scrollbar::computeSize(Size& outSize) const
    {
        TBaseClass::computeSize(outSize);

        // process the elements
        for (auto element = childrenList(); element; ++element)
        {
            if (element->visibility() == VisibilityState::Visible)
            {
                auto elementLayoutSize = element->cachedLayoutParams().calcTotalSize();
                if (m_direction == Direction::Vertical)
                {
                    outSize.x = std::max(outSize.x, elementLayoutSize.x);
                    outSize.y += elementLayoutSize.y;
                }
                else
                {
                    outSize.x += elementLayoutSize.x;
                    outSize.y = std::max(outSize.y, elementLayoutSize.y);
                }
            }
        }
    }

    InputActionPtr Scrollbar::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.leftClicked() && isEnabled())
        {
            // have we clicked before or after the thumb ?
            float clickPos = (float)((m_direction == Direction::Vertical) ? evt.absolutePosition().y : evt.absolutePosition().x);
            if (clickPos < m_cachedScrollThumbCenter)
                scrollPosition(scrollPosition() - m_largeStepSize);
            else
                scrollPosition(scrollPosition() + m_largeStepSize);
        }

        return nullptr;
    }

    void Scrollbar::handleEnableStateChange(bool isEnabled)
    {
        TBaseClass::handleEnableStateChange(isEnabled);

        if (m_backwardButton)
            m_backwardButton->enable(isEnabled);
        if (m_forwardButton)
            m_forwardButton->enable(isEnabled);
    }

    bool Scrollbar::handleMouseWheel(const base::input::MouseMovementEvent &evt, float delta)
    {
        if (isEnabled())
        {
            if (delta > 0.0f)
            {
                scrollPosition(scrollPosition() - m_smallStepSize * 3.0f);
                return true;
            }
            else if (delta < 0.0f)
            {
                scrollPosition(scrollPosition() + m_smallStepSize * 3.0f);
                return true;
            }
        }

        return false;
    }

    void Scrollbar::arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const
    {
        auto trackArea = innerArea;

        // eat the space for the back button
        if (m_backwardButton)
        {
            auto elementLayoutSize = m_backwardButton->cachedLayoutParams().calcTotalSize();
            auto buttonSize = (m_direction == Direction::Vertical) ? elementLayoutSize.y : elementLayoutSize.x;
            auto allocatedArea = trackArea.slice(m_direction, trackArea.min(m_direction), trackArea.min(m_direction) + buttonSize);
            auto area = ArrangeElementLayout(allocatedArea, m_backwardButton->cachedLayoutParams(), elementLayoutSize);
            outArrangedChildren.add(m_backwardButton, area, clipArea);

            // eat
            trackArea = trackArea.slice(m_direction, allocatedArea.max(m_direction), trackArea.max(m_direction));
        }

        // eat the space for the forward button
        if (m_forwardButton)
        {
            auto elementLayoutSize = m_forwardButton->cachedLayoutParams().calcTotalSize();
            auto buttonSize = (m_direction == Direction::Vertical) ? elementLayoutSize.y : elementLayoutSize.x;
            auto allocatedArea = trackArea.slice(m_direction, trackArea.max(m_direction) - buttonSize, trackArea.max(m_direction));
            auto area = ArrangeElementLayout(allocatedArea, m_backwardButton->cachedLayoutParams(), elementLayoutSize);
            outArrangedChildren.add(m_forwardButton, area, clipArea);

            // eat
            trackArea = trackArea.slice(m_direction, trackArea.min(m_direction), allocatedArea.min(m_direction));
        }

        // draw the thumb only if there's a scrollable area
        if (m_scrollAreaSize > 0.0f && m_scrollThumbSize > 0.0f && !trackArea.empty())
        {
            // compute fractional thumb position and size as the % of the visible area to the total area
            auto thumbSizeFrac = std::min<float>(m_scrollAreaSize, m_scrollThumbSize) / m_scrollAreaSize;
            auto thumbAbsPosFrac = std::clamp(m_scrollPosition / m_scrollAreaSize, 0.0f, 1.0f - thumbSizeFrac);

            // compute fractional position the thumb as the faction of the MAX position
            auto thumbRelPosFrac = (thumbSizeFrac < 1.0f) ? (thumbAbsPosFrac / (1.0f - thumbSizeFrac)) : 0.0f;

            // compute required thumb size
            auto trackAreaSizeBase = trackArea.size(m_direction);
            auto thumbMinSize = 10.0f;// m_thumb->cachedLayoutParams().calcMinSize();
            auto thumbSizeReal = std::min<float>(trackAreaSizeBase, std::max<float>(trackAreaSizeBase * thumbSizeFrac, thumbMinSize));

            // compute the size of the scrollable are, those are the pixels that remain after leaving the pixels for thumb
            auto trackAreaSizeReal = trackAreaSizeBase - thumbSizeReal;

            // cache the amount of pixels per scroll area 
            auto scrolableAreaSize = std::max<float>(0.0f, m_scrollAreaSize - m_scrollThumbSize);
            m_cachedScrollAreaPixelDelta = (trackAreaSizeReal > 0.0f) ? scrolableAreaSize / trackAreaSizeReal : 0.0f;

            // compute thumb placement
            auto thumbStart = trackArea.min(m_direction) + trackAreaSizeReal * thumbRelPosFrac;
            auto thumbEnd = thumbStart + thumbSizeReal;
            auto thumbArea = trackArea.slice(m_direction, thumbStart, thumbEnd);
            m_cachedScrollThumbCenter = (thumbStart + thumbEnd) * 0.5f;

            // place the thumb in the computed area
            outArrangedChildren.add(m_thumb, thumbArea, clipArea);
        }
        else
        {
            // no scrolling
            m_cachedScrollAreaPixelDelta = 0.0f;
            m_cachedScrollThumbCenter = 0.0f;
        }
    }

    //--

    class ScrollbarThumbInputAction : public MouseInputAction
    {
    public:
        ScrollbarThumbInputAction(ScrollbarThumb* thumb, const ElementArea& thumbArea, const Position& originalClickPosition)
            : MouseInputAction(thumb, base::input::KeyCode::KEY_MOUSE0)
            , m_thumb(thumb)
            , m_thumbInitialArea(thumbArea)
            , m_originalClickPosition(originalClickPosition)
        {
            m_originalScrollPosition = thumb->parentScrolbar()->scrollPosition();
        }

        virtual ~ScrollbarThumbInputAction()
        {
        }

        virtual void onCanceled() override
        {
            if (auto bar = m_thumb.lock())
            {
                bar->parentScrolbar()->scrollPosition(m_originalScrollPosition);
                bar->inputActionFinished();
            }
        }

        virtual void onFinished()  override
        {
            if (auto bar = m_thumb.lock())
                bar->inputActionFinished();
        }

        virtual bool allowsHoverTracking() const override
        {
            return false;
        }

        float computeTargetScrollPosition(const Position& pos) const
        {
            static const auto TRACK_AREA_MARGIN = 150.0f;

            if (auto bar = m_thumb.lock())
            {
                auto* scrollBar = bar->parentScrolbar();
                if (scrollBar->direction() == Direction::Vertical)
                {
                    // compute the allowed track area (extends up/down)
                    auto trackArea = m_thumbInitialArea.extend(TRACK_AREA_MARGIN, 10000.0f, TRACK_AREA_MARGIN, 10000.0f);
                    if (trackArea.contains(pos))
                    {
                        // compute the movement offset
                        auto deltaPos = pos - m_originalClickPosition;
                        auto deltaScroll = deltaPos.y * scrollBar->cachedScrollAreaPixelDelta();
                        return m_originalScrollPosition + deltaScroll;
                    }
                }
                else if (scrollBar->direction() == Direction::Horizontal)
                {
                    // compute the allowed track area (extends left/right)
                    auto trackArea = m_thumbInitialArea.extend(10000.0f, TRACK_AREA_MARGIN, 10000.0f, TRACK_AREA_MARGIN);
                    if (trackArea.contains(pos))
                    {
                        // compute the movement offset
                        auto deltaPos = pos - m_originalClickPosition;
                        auto deltaScroll = deltaPos.x * scrollBar->cachedScrollAreaPixelDelta();
                        return m_originalScrollPosition + deltaScroll;
                    }
                }
            }

            // we are outside the active scroll area, revert to default position
            return m_originalScrollPosition; 
        }

        virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override
        {
            if (auto bar = m_thumb.lock())
            {
                auto newScrollPosition = computeTargetScrollPosition(evt.absolutePosition().toVector());
                bar->parentScrolbar()->scrollPosition(newScrollPosition);
            }

            return InputActionResult();
        }

    private:
        base::RefWeakPtr<ScrollbarThumb> m_thumb;
        Position m_originalClickPosition;
        float m_originalScrollPosition;
        ElementArea m_thumbInitialArea;
    };

    //---

    RTTI_BEGIN_TYPE_CLASS(ScrollbarThumb);
        RTTI_METADATA(ElementClassNameMetadata).name("ScrollBarThumb");
    RTTI_END_TYPE();

    ScrollbarThumb::ScrollbarThumb(Scrollbar* parent)
        : m_scrollbar(parent)
        , m_dragging(false)
    {
        // allow clicks
        hitTest(HitTestState::Enabled);
    }

    void ScrollbarThumb::dragging(bool flag)
    {
        if (m_dragging != flag)
        {
            m_dragging = flag;

            if (flag)
                addStylePseudoClass("dragged"_id);
            else
                removeStylePseudoClass("dragged"_id);
        }
    }

    InputActionPtr ScrollbarThumb::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.leftClicked() && m_scrollbar->isEnabled())
        {
            dragging(true);
            return base::RefNew<ScrollbarThumbInputAction>(this, area, evt.absolutePosition().toVector());
        }

        return InputActionPtr();
    }

    void ScrollbarThumb::inputActionFinished()
    {
        dragging(false);

        m_scrollbar->call(EVENT_SCROLLED, m_scrollbar->scrollPosition());
    }

    //---

} // ui
