/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\rect #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//--

INLINE Rect::Rect()
{}

INLINE Rect::Rect(int left, int top, int right, int bottom)
    : min(left, top)
    , max(right, bottom)
{}

INLINE Rect::Rect(const Point &amin, const Point &amax)
{
    min.x = std::min<int>(amin.x, amax.x);
    min.y = std::min<int>(amin.y, amax.y);
    max.x = std::max<int>(amin.x, amax.x);
    max.y = std::max<int>(amin.y, amax.y);
}

INLINE Point Rect::size() const
{
    return Point(max.x - min.x, max.y - min.y);
}

INLINE int Rect::width() const
{
    return max.x - min.x;
}

INLINE int Rect::height() const
{
    return max.y - min.y;
}

INLINE Rect Rect::inner(const Point &margin) const
{
    return Rect(min + margin, max - margin);
}

INLINE Rect Rect::inner(int margin) const
{
    return inner(Point(margin, margin));
}

INLINE Rect Rect::inflated(const Point &margin) const
{
    return Rect(min - margin, max + margin);
}

INLINE Rect Rect::inflated(int margin) const
{
    return inflated(Point(margin, margin));
}

INLINE Rect Rect::clipped(const Rect& clipArea) const
{
    return Rect(std::max<int>(min.x, clipArea.min.x), std::max<int>(min.y, clipArea.min.y), std::min<int>(max.x, clipArea.max.x), std::min<int>(max.y, clipArea.max.y));
}

INLINE int Rect::left() const
{
    return min.x;
}

INLINE int Rect::right() const
{
    return max.x;
}

INLINE int Rect::top() const
{
    return min.y;
}

INLINE int Rect::bottom() const
{
    return max.y;
}

INLINE int Rect::centerX() const
{
    return (min.x + max.x) / 2;
}

INLINE int Rect::centerY() const
{
    return (min.y + max.y) / 2;
}

INLINE Point Rect::center() const
{
    return Point(centerX(), centerY());
}

INLINE Point Rect::topLeft() const
{
    return Point(min.x, min.y);
}

INLINE Point Rect::topRight() const
{
    return Point(max.x, min.y);
}

INLINE Point Rect::bottomLeft() const
{
    return Point(min.x, max.y);
}

INLINE Point Rect::bottomRight() const
{
    return Point(max.x, max.y);
}

INLINE bool Rect::empty() const
{
    return (min.x >= max.x) || (min.y >= max.y);
}

INLINE bool Rect::contains(const Point &point) const
{
    return (point.x < max.x) && (point.x >= min.x) && (point.y < max.y) && (point.y >= min.y);
}

INLINE bool Rect::contains(int x, int y) const
{
    return (x < max.x) && (x >= min.x) && (y < max.y) && (y >= min.y);
}

INLINE bool Rect::contains(const Rect &rect) const
{
    return (rect.max.x <= max.x) && (rect.min.x >= min.x) && (rect.max.y <= max.y) && (rect.min.y >= min.y);
}

INLINE bool Rect::touches(const Rect &rect) const
{
    return (rect.min.x <= max.x) && (rect.max.x >= min.x) && (rect.min.y <= max.y) && (rect.max.y >= min.y);
}

INLINE bool Rect::operator==(const Rect &other) const
{
    return (min == other.min) && (max == other.max);
}

INLINE bool Rect::operator!=(const Rect &other) const
{
    return (min != other.min) || (max != other.max);
}

INLINE Rect& Rect::operator+=(const Point &shift)
{
    min += shift;
    max += shift;
    return *this;
}

INLINE Rect& Rect::operator-=(const Point &shift)
{
    min -= shift;
    max -= shift;
    return *this;
}

INLINE Rect Rect::operator+(const Point &shift) const
{
    return Rect(min + shift, max + shift);
}

INLINE Rect Rect::operator-(const Point &shift) const
{
    return Rect(min - shift, max - shift);
}

INLINE Rect Rect::operator+(const Rect &other) const
{
    return Rect(min + other.min, max + other.max);
}

INLINE Rect Rect::operator-(const Rect &other) const
{
    return Rect(min - other.min, max - other.max);
}

//--

END_BOOMER_NAMESPACE(base)