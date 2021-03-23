/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

DECLARE_UI_EVENT(EVENT_SCROLL, float)
DECLARE_UI_EVENT(EVENT_SCROLLED, float)

//--

class Button;
class Scrollbar;
class ScrollbarThumb;
class ScrollbarThumbInputAction;

/// scrolling event
typedef std::function<void(const RefPtr<Scrollbar>& scrollbar)> TScrollEvent;

/// a simple scrollbar, consists of scroll thumb, buttons and few other shitty pieces
class ENGINE_UI_API Scrollbar : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(Scrollbar, IElement);

public:
    Scrollbar(Direction direction = Direction::Vertical, bool createButtons = true);
    virtual ~Scrollbar();

    // get orientation of the scrollbar
    INLINE Direction direction() const { return m_direction; }

    // get size of the total scrolled area allowed
    INLINE float scrollAreaSize() const { return m_scrollAreaSize; }

    // get the scroll area pixel delta (amount of scroll area per pixel of the scrollbar track area)
    INLINE float cachedScrollAreaPixelDelta() const { return m_cachedScrollAreaPixelDelta; }

    // get current scrollbar position
    INLINE float scrollPosition() const { return m_scrollPosition; }

    // get current scrollbar thumb size 
    INLINE float scrollThumbSize() const { return m_scrollThumbSize; }

    // get small step size (button clicks)
    INLINE float smallStepSize() const { return m_smallStepSize; }

    // get large step size (track area clicks)
    INLINE float largeStepSize() const { return m_largeStepSize; }

    // set total scrollable area
    // NOTE: this should be set first
    void scrollAreaSize(float areaSize);

    // set the thumb size 
    // NOTE: the thumb size is limited by the total area size, area size should be set first
    void scrollThumbSize(float thumbSize);

    // set scrollbar position
    // NOTE: position is limited by scroll area size and thumb size, this must be set after the area and thumb size is changed
    void scrollPosition(float position, bool callEvent = true);

    // set the small step size (button press), usually a single line
    void smallStepSize(float smallStepSize);

    // set the large step size (track area press), usually multiple lines
    void largeStepSize(float largeStepSize);

    //--

    //TScrollEvent OnScroll;

    //--

private:
    // IElement
    virtual void computeSize(Size& outSize) const override;
    virtual void arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const override;
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt) override final;
    virtual bool handleMouseWheel(const InputMouseMovementEvent &evt, float delta) override;
    virtual void handleEnableStateChange(bool isEnabled) override;

    float m_scrollPosition; // position in the scrolled area
    float m_scrollAreaSize; // total size of the scrolled area
    float m_scrollThumbSize; // size of the scroll area represented by the thumb
    Direction m_direction; // scrollbar direction
    bool m_hasButtons; // do we have scroll buttons ?

    float m_smallStepSize; // amount of position to advance when a scroll button is pressed (and or arrow keys when the scrollbar is in focus)
    float m_largeStepSize; // amount of position to advance when the track area (area being the thumb) is cliked

    mutable float m_cachedScrollAreaPixelDelta; // amount of scroll area covered by one pixel of the scrollbar
    mutable float m_cachedScrollThumbCenter;

    RefPtr<ScrollbarThumb> m_thumb;
    RefPtr<Button> m_backwardButton;
    RefPtr<Button> m_forwardButton;
};

/// tracking element of the scrollbar
class ENGINE_UI_API ScrollbarThumb : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ScrollbarThumb, IElement);

public:
    ScrollbarThumb(Scrollbar* parent = nullptr);

    /// get the parent scrollbar
    INLINE Scrollbar* parentScrolbar() const { return m_scrollbar; }

    //--

    // toggle the dragging flag
    void dragging(bool flag);

private:
    // IElement
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt) override;

    //--

    Scrollbar* m_scrollbar;
    bool m_dragging : 1;

    void inputActionFinished();

    friend class ScrollbarThumbInputAction;
};

END_BOOMER_NAMESPACE_EX(ui)

