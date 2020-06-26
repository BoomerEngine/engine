/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements #]
***/

#include "build.h"
#include "uiElement.h"
#include "uiElementLayout.h"

namespace ui
{

    //---

    RTTI_BEGIN_TYPE_ENUM(Direction);
        RTTI_ENUM_OPTION(Vertical);
        RTTI_ENUM_OPTION(Horizontal);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(ElementVerticalLayout);
        RTTI_ENUM_OPTION(Top);
        RTTI_ENUM_OPTION(Middle);
        RTTI_ENUM_OPTION(Bottom);
        RTTI_ENUM_OPTION(Expand);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(ElementHorizontalLayout);
        RTTI_ENUM_OPTION(Left);
        RTTI_ENUM_OPTION(Center);
        RTTI_ENUM_OPTION(Right);
        RTTI_ENUM_OPTION(Expand)
    RTTI_END_TYPE();

    //---

    ElementArea::ElementArea()
        : m_size(0, 0)
        , m_absolutePosition(0, 0)
    {}

    ElementArea::ElementArea(const Position& absolutePosition, const Size& size)
        : m_absolutePosition(absolutePosition)
        , m_size(std::max(0.0f, size.x), std::max(0.0f, size.y))
    {}

    ElementArea::ElementArea(const base::Rect& pixelRect)
        : m_absolutePosition((float)pixelRect.min.x, (float)pixelRect.min.y)
        , m_size(std::max(0.0f, (float)pixelRect.width()), std::max(0.0f, (float)pixelRect.height()))
    {}

    ElementArea::ElementArea(float left, float top, float right, float bottom)
        : m_absolutePosition(left, top)
        , m_size(std::max(0.0f, right-left), std::max(0.0f, bottom-top))
    {}

    ///---

    ElementLayout::ElementLayout()
        : m_innerSize(0, 0)
        , m_verticalAlignment(ElementVerticalLayout::Top)
        , m_horizontalAlignment(ElementHorizontalLayout::Left)
        , m_relative(0.0f, 0.0f)
        , m_proportion(0.0f)
    {}

    ElementLayout::ElementLayout(const Size& size)
        : m_innerSize(size)
        , m_verticalAlignment(ElementVerticalLayout::Top)
        , m_horizontalAlignment(ElementHorizontalLayout::Left)
        , m_relative(0.0f, 0.0f)
        , m_proportion(0.0f)
    {}

    Size ElementLayout::calcTotalSize() const
    {
        Size ret;
        ret.x = m_innerSize.x + m_padding.left() + m_padding.right();
        ret.y = m_innerSize.y + m_padding.top() + m_padding.bottom();
        ret.x += m_margin.left() + m_margin.right();
        ret.y += m_margin.top() + m_margin.bottom();
        return ret;
    }

    ElementArea ElementLayout::calcInnerAreaFromDrawArea(const ElementArea& outerArea) const
    {
        auto outerSize = outerArea.size();

        Size size;
        size.x = std::max(0.0f, outerSize.x - m_padding.left() - m_padding.right());
        size.y = std::max(0.0f, outerSize.y - m_padding.top() - m_padding.bottom());
        Position pos;
        pos.x = m_padding.left();
        pos.y = m_padding.top();
        return ElementArea(pos + outerArea.absolutePosition(), size);
    }

    ElementArea ElementLayout::calcTotalAreaFromDrawArea(const ElementArea& drawArea) const
    {
        auto outerSize = drawArea.size();

        Size size;
        size.x = std::max(0.0f, outerSize.x + m_margin.left() + m_margin.right());
        size.y = std::max(0.0f, outerSize.y + m_margin.top() + m_margin.bottom());
        Position pos;
        pos.x -= m_margin.left();
        pos.y -= m_margin.top();
        return ElementArea(pos + drawArea.absolutePosition(), size);
    }

    ElementArea ElementLayout::calcDrawAreaFromOuterArea(const ElementArea& outerArea) const
    {
        auto outerSize = outerArea.size();

        Size size;
        if (m_horizontalAlignment == ElementHorizontalLayout::Expand)
            size.x = std::max(0.0f, outerSize.x - m_margin.left() - m_margin.right());
        else
            size.x = m_innerSize.x + m_padding.left() + m_padding.right();

        if (m_verticalAlignment == ElementVerticalLayout::Expand)
            size.y = std::max(0.0f, outerSize.y - m_margin.top() - m_margin.bottom());
        else
            size.y = m_innerSize.y + m_padding.top() + m_padding.bottom();

        Position pos;
        pos.x = m_margin.left();
        pos.y = m_margin.top();
        pos.x += m_relative.x;
        pos.y += m_relative.y;
        return ElementArea(pos + outerArea.absolutePosition(), size);
    }

    ///---

    RTTI_BEGIN_TYPE_STRUCT(Offsets);
        RTTI_PROPERTY(m_left).editable();
        RTTI_PROPERTY(m_top).editable();
        RTTI_PROPERTY(m_right).editable();
        RTTI_PROPERTY(m_bottom).editable();
    RTTI_END_TYPE();

    Offsets::Offsets()
        : m_left(0)
        , m_right(0)
        , m_top(0)
        , m_bottom(0)
    {}

    Offsets::Offsets(float val)
        : m_left(val)
        , m_right(val)
        , m_top(val)
        , m_bottom(val)
    {}

    Offsets::Offsets(float left, float top, float right, float bottom)
        : m_left(left)
        , m_top(top)
        , m_right(right)
        , m_bottom(bottom)
    {}

    Offsets Offsets::operator+(const Offsets& other) const
    {
        return Offsets(m_left + other.m_left, m_top + other.m_top, m_right + other.m_right, m_bottom + other.m_bottom);
    }

    Offsets Offsets::operator*(float scale) const
    {
        return Offsets(m_left * scale, m_top * scale, m_right * scale, m_bottom * scale);
    }

    Offsets& Offsets::operator*=(float scale)
    {
        m_left *= scale;
        m_top *= scale;
        m_right *= scale;
        m_bottom *= scale;
        return *this;
    }

    bool Offsets::operator==(const Offsets& other) const
    {
        return (m_left == other.m_left) && (m_right == other.m_right) && (m_top == other.m_top) && (m_bottom == other.m_bottom);
    }

    bool Offsets::operator!=(const Offsets& other) const
    {
        return !operator==(other);
    }

    //---

    ArrangedChildren::ArrangedChildren()
    {}

    void ArrangedChildren::shift(float dx, float dy)
    {
        for (auto& elem : m_elements)
            if (!elem.m_element->isIgnoredInAutomaticLayout())
                elem.m_drawArea.shift(dx, dy);
    }

    //---

    ElementPtr Layout(LayoutMode mode, const std::initializer_list<ElementPtr>& elements)
    {
        auto ret = base::CreateSharedPtr<IElement>();
        ret->layoutMode(mode);

        for (const auto& child : elements)
            ret->attachChild(child);

        return ret;
    }

    ElementPtr LayoutVertically(const std::initializer_list<ElementPtr>& elements)
    {
        return Layout(LayoutMode::Vertical, elements);
    }

    ElementPtr LayoutHorizontally(const std::initializer_list<ElementPtr>& elements)
    {
        return Layout(LayoutMode::Horizontal, elements);
    }

    //---

} // ui
