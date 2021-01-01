/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\layout #]
***/

#pragma once

#include "uiElement.h"
#include "uiScrollBar.h"

namespace ui
{

    //--

    /// element that can contain more content than normally allowed and show scrollbars
    class BASE_UI_API ScrollArea : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ScrollArea, IElement);

    public:
        ScrollArea(ScrollMode vmode = ScrollMode::None, ScrollMode hscroll = ScrollMode::None);

        // get vertical scroll mode
        INLINE ScrollMode verticalScrollMode() const { return m_verticalScrollMode; }

        // get horizontal scroll mode
        INLINE ScrollMode horizontalScrollMode() const { return m_horizontalScrollMode; }

        // get vertical scroll offset
        INLINE float verticalScrollOffset() const { return m_scrollOffset.y; }

        // get horizontal scroll offset
        INLINE float horizontalScrollOffset() const { return m_scrollOffset.x; }

        // set new vertical scroll mode
        void verticalScrollMode(ScrollMode mode);

        // set new horizontal scroll mode
        void horizontalScrollMode(ScrollMode mode);

        // set new scroll offset
        void scrollOffset(const Position& offset) const;

        // ensure given area is visible
        void scrollToMakeAreaVisible(const ElementArea& area) const;

        // ensure area of given element will be visible
        void scrollToMakeElementVisible(IElement* element);


    protected:
        ScrollMode m_verticalScrollMode = ScrollMode::None;
        ScrollMode m_horizontalScrollMode = ScrollMode::None;

        mutable uint8_t m_scrollBarState = 0;
        mutable Position m_scrollOffset;
        mutable base::RefPtr<Scrollbar> m_verticalScrollBar;
        mutable base::RefPtr<Scrollbar> m_horizontalScrollBar;

        mutable ElementWeakPtr m_requestedScrollToElement;

        ///

        virtual bool handleMouseWheel(const base::input::MouseMovementEvent& evt, float delta) override;
        virtual bool handleTemplateProperty(base::StringView name, base::StringView value) override;
        virtual void arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const override;
        virtual void computeSize(Size& outSize) const override;
    };

    //--

} // ui