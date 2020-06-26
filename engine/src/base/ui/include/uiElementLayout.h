/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#pragma once

#include "uiStyleLibrary.h"
#include "base/containers/include/inplaceArray.h"

namespace ui
{
    //------

    /// rendering space allocated for an element
    class BASE_UI_API ElementArea
    {
    public:
        ElementArea();
        ElementArea(const Position& absolutePosition, const Size& size);
        ElementArea(const base::Rect& pixelRect);
        ElementArea(float left, float top, float right, float bottom);
        INLINE ElementArea(const ElementArea& other) = default;
        INLINE ElementArea(ElementArea&& other) = default;
        INLINE ElementArea& operator=(const ElementArea& other) = default;
        INLINE ElementArea& operator=(ElementArea&& other) = default;

        INLINE bool empty() const
        {
            return m_size.x == 0 || m_size.y == 0;
        }

        INLINE const Position& absolutePosition() const
        {
            return m_absolutePosition;
        }

        INLINE const Size& size() const
        {
            return m_size;
        }

        INLINE Position center() const
        {
            return m_absolutePosition + m_size * 0.5f;
        }

        INLINE float top() const
        {
            return m_absolutePosition.y;
        }

        INLINE float left() const
        {
            return m_absolutePosition.x;
        }

        INLINE float bottom() const
        {
            return m_absolutePosition.y + m_size.y;
        }

        INLINE float right() const
        {
            return m_absolutePosition.x + m_size.x;
        }

        INLINE float size(Direction dir) const
        {
            return (dir == Direction::Vertical) ? m_size.y : m_size.x;
        }

        INLINE Position tl() const
        {
            return m_absolutePosition;
        }

        INLINE Position br() const
        {
            return m_absolutePosition + m_size;
        }

        INLINE float min(Direction dir) const
        {
            return (dir == Direction::Vertical) ? top() : left();
        }

        INLINE float max(Direction dir) const
        {
            return (dir == Direction::Vertical) ? bottom() : right();
        }

        INLINE base::Rect rect() const
        {
            return base::Rect((int)m_absolutePosition.x, (int)m_absolutePosition.y, (int)(m_absolutePosition.x + m_size.x), (int)(m_absolutePosition.y + m_size.y));
        }

        INLINE bool touches(const ElementArea& area) const
        {
            if (left() > area.right()) return false;
            if (top() > area.bottom()) return false;
            if (right() < area.left()) return false;
            if (bottom() < area.top()) return false;
            return true;
        }

        INLINE bool contains(const Position& pos) const
        {
            if (pos.x < m_absolutePosition.x || pos.y < m_absolutePosition.y)
                return false;
            if (pos.x > m_absolutePosition.x + m_size.x || pos.y > m_absolutePosition.y + m_size.y)
                return false;
            return true;
        }

        INLINE bool contains(const base::Point& pos) const
        {
            if ((float)pos.x < m_absolutePosition.x || (float)pos.y < m_absolutePosition.y)
                return false;
            if ((float)pos.x > m_absolutePosition.x + m_size.x || (float)pos.y > m_absolutePosition.y + m_size.y)
                return false;
            return true;
        }

        INLINE ElementArea resetOffset() const
        {
            return ElementArea(Position::ZERO(), m_size);
        }

        INLINE ElementArea offset(const Position& pos) const
        {
            return ElementArea(m_absolutePosition + pos, m_size);
        }

        INLINE ElementArea resize(const Size& size) const
        {
            return ElementArea(m_absolutePosition, size);
        }

        INLINE void shift(float dx, float dy)
        {
            m_absolutePosition.x += dx;
            m_absolutePosition.y += dy;
        }

        INLINE ElementArea shrink(float size) const
        {
            return ElementArea(m_absolutePosition + Position(size, size), m_size - Size(size*2.0f, size*2.0f));
        }

        INLINE ElementArea verticalSlice(float from, float to) const
        {
            return ElementArea(Position(m_absolutePosition.x, from), Size(m_size.x, std::max(0.0f, to - from)));
        }

        INLINE ElementArea horizontalSice(float from, float to) const
        {
            return ElementArea(Position(from, m_absolutePosition.y), Size(std::max(0.0f, to - from), m_size.y));
        }

        INLINE ElementArea slice(Direction dir, float from, float to) const
        {
            if (dir == Direction::Vertical)
                return verticalSlice(from, to);
            else
                return horizontalSice(from, to);
        }

        INLINE ElementArea extend(float left, float top, float right, float bottom) const
        {
            ElementArea ret(*this);
            ret.m_absolutePosition.x -= left;
            ret.m_absolutePosition.y -= top;
            ret.m_size.x += left + right;
            ret.m_size.y += top + bottom;
            return ret;
        }

        INLINE float distance(const Position& pos) const
        {
            auto leftDist = std::max(0.0f, left() - pos.x);
            auto topDist = std::max(0.0f, top() - pos.y);
            auto rightDist = std::max(0.0f, pos.x - right());
            auto bottomDist = std::max(0.0f, pos.y - bottom());
            return std::max(std::max(leftDist, rightDist), std::max(topDist, bottomDist));
        }

        INLINE ElementArea clipTo(const ElementArea& clipArea) const
        {
            auto left = std::max(this->left(), clipArea.left());
            auto top = std::max(this->top(), clipArea.top());
            auto right = std::min(this->right(), clipArea.right());
            auto bottom = std::min(this->bottom(), clipArea.bottom());
            return ElementArea(left, top, right, bottom);
        }

    private:
        Position m_absolutePosition;
        Size m_size;
    };

    //------

    /// generalized padding/border/margin settings
    struct BASE_UI_API Offsets
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(Offsets);

    public:
        Offsets(); // zero
        Offsets(float val); // all the same
        Offsets(float left, float top, float right, float bottom);
        Offsets(const Offsets& other) = default;

        //--

        bool operator==(const Offsets& other) const;
        bool operator!=(const Offsets& other) const;

        Offsets operator+(const Offsets& other) const;
        Offsets operator*(float scale) const;

        Offsets& operator*=(float scale);

        //--

        /// get left offset, if the offset is set to auto it's clamped to 0
        INLINE float left() const { return m_left; }

        /// get top offset, if the offset is set to auto it's clamped to 0
        INLINE float top() const { return m_top; }

        /// get right offset, if the offset is set to auto it's clamped to 0
        INLINE float right() const { return m_right; }

        /// get bottom offset, if the offset is set to auto it's clamped to 0
        INLINE float bottom() const { return m_bottom; }

    protected:
        float m_left;
        float m_top;
        float m_right;
        float m_bottom;
    };

    //------

    // vertical alignment mode for element
    enum class ElementVerticalLayout : uint8_t
    {
        Top=0,
        Middle=1,
        Bottom=2,
        Expand=3,
    };

    // horizontal alignment mode for element
    enum class ElementHorizontalLayout : uint8_t
    {
        Left=0,
        Center=1,
        Right=2,
        Expand=3,
    };

    //------

    // basic layout informations for ui element
    // TODO: pack this better ?
    class BASE_UI_API ElementLayout
    {
    public:
        ElementVerticalLayout m_verticalAlignment = ElementVerticalLayout::Expand;
        ElementHorizontalLayout m_horizontalAlignment = ElementHorizontalLayout::Expand;
        ElementVerticalLayout m_internalVerticalAlignment = ElementVerticalLayout::Top;
        ElementHorizontalLayout m_internalHorizontalAlignment = ElementHorizontalLayout::Left;

        Offsets m_padding;
        Offsets m_margin;
        Size m_innerSize; // inner part (no padding/no margin/no border)        
        Position m_relative; // relative position for drawing
        float m_proportion; // proportion for vertical/horizontal sizer

        ElementLayout();
        ElementLayout(const Size& innerSize);

        // calculate size required for element including it's margins, padding and border
        Size calcTotalSize() const;

        // calculate draw area element given the size of the outside area (-margins - border/2)
        ElementArea calcDrawAreaFromOuterArea(const ElementArea& outerArea) const;

        // calculate inner area of element given the size of the drawing area (-padding)
        ElementArea calcInnerAreaFromDrawArea(const ElementArea& drawArea) const;

        // calculate outer area of element given the size of the drawing area (+margins)
        ElementArea calcTotalAreaFromDrawArea(const ElementArea& drawArea) const;
    };

    //------

    // list of arranged child elements
    // NOTE: invisible elements are usually not arranged
    class BASE_UI_API ArrangedChildren
    {
    public:
        ArrangedChildren();

        struct Entry
        {
            IElement* m_element;
            ElementArea m_drawArea;
            ElementArea m_clipArea;

            INLINE Entry(IElement* ptr, const ElementArea& drawArea, const ElementArea& clipArea)
                : m_element(ptr)
                , m_drawArea(drawArea)
                , m_clipArea(clipArea)
            {}
        };

        INLINE void add(const IElement* ptr, const ElementArea& drawArea, const ElementArea& clipArea)
        {
            m_elements.emplaceBack(const_cast<IElement*>(ptr), drawArea, clipArea); // TODO: clenaup the const here
        }

        INLINE base::ConstArrayIterator<Entry> begin() const
        {
            return m_elements.begin();
        }

        INLINE base::ConstArrayIterator<Entry> end() const
        {
            return m_elements.end();
        }

        INLINE void reset()
        {
            m_elements.reset();
        }

        INLINE void clear()
        {
            m_elements.clear();
        }

        INLINE bool empty() const
        {
            return m_elements.empty();
        }

        //--

        void shift(float dx, float dy);

    private:
        typedef base::InplaceArray<Entry, 20> TElements;
        TElements m_elements;
    };

    //-----

    // align on vertical axis
    extern BASE_UI_API void ArrangeAxis(float requiredSize, float allowedSize, float minSize, float maxSize, ElementVerticalLayout mode, float& outSize, float& outOffset);

    // align on horizontal axis
    extern BASE_UI_API void ArrangeAxis(float requiredSize, float allowedSize, float minSize, float maxSize, ElementHorizontalLayout mode, float& outSize, float& outOffset);

    // arrange element into provided area using the element style
    extern BASE_UI_API ElementArea ArrangeElementLayout(const ElementArea& incomingInnerArea, const ElementLayout& layout, const Size& totalSize);

    // arrange element into provided area using the element style
    extern BASE_UI_API ElementArea ArrangeAreaLayout(const ElementArea& incomingInnerArea, const Size& size, ElementVerticalLayout verticalAlign, ElementHorizontalLayout horizontalAlign);

    //-----

} // ui