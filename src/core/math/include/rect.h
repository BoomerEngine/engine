/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\rect #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

/// 2D integer rectangle
class CORE_MATH_API Rect
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Rect);

public:
    Point min, max;

    //---

    Rect();
    Rect(int left, int top, int right, int bottom);
    Rect(const Point &amin, const Point &amax);
    Rect(const Rect &other) = default;
    Rect(Rect&& other) = default;
    Rect& operator=(const Rect &other) = default;
    Rect& operator=(Rect&& other) = default;

    bool operator==(const Rect &other) const;
    bool operator!=(const Rect &other) const;

    Rect &operator+=(const Point &shift);
    Rect &operator-=(const Point &shift);

    Rect operator+(const Point &shift) const;
    Rect operator-(const Point &shift) const;

    Rect operator+(const Rect &other) const;
    Rect operator-(const Rect &other) const;

    //---

    //! get size of the rectangle
    Point size() const;

    //! get width of the rectangle
    int width() const;

    //! get height of the rectangle
    int height() const;

    //! Get left coordinate
    int left() const;

    //! Get right coordinate
    int right() const;

    //! Get top coordinate
    int top() const;

    //! Get bottom coordinate
    int bottom() const;

    //! Get horizontal center
    int centerX() const;

    //! Get vertical center
    int centerY() const;

    //! Get center of the rectangle
    Point center() const;

    //! Get top left point
    Point topLeft() const;

    //! Get top right point
    Point topRight() const;

    //! Get bottom left point
    Point bottomLeft() const;

    //! Get bottom right point
    Point bottomRight() const;

    //! Check if rect is empty
    bool empty() const;

    //! Check if point is inside rect
    bool contains(const Point &point) const;

    //! Check if point is inside rect
    bool contains(int x, int y) const;

    //! Check if rect is inside rect
    bool contains(const Rect &rect) const;

    //! Check if two rectangles are touching
    bool touches(const Rect &rect) const;

    //! get inner rect, returns empty rect if margin is to small
    Rect inner(const Point &margin) const;

    //! get inner rect, returns empty rect if margin is to small
    Rect inner(int margin) const;

    //! get rectangle inflated by given margin, returns empty rect if margin is to small
    Rect inflated(const Point &margin) const;

    //! get rectangle inflated by given margin, returns empty rect if margin is to small
    Rect inflated(int margin) const;

    //! get cliped rectangle (note: empty rectangle may be returned)
    Rect clipped(const Rect& clipArea) const;

    //! return rect centered on given point
    Rect centered(int x, int y) const;

    //! return rect centered on given point
    Rect centered(const Point& c) const;

    //! Merge two rectangles - extend rectangle to include given other rectangle
    Rect& merge(const Rect &rect);

    //! Extend rectangle to include given point
    Rect& merge(const Point& point);

    //! Extend rectangle to include given point
    Rect& merge(int x, int y);

    //---

    static const Rect& EMPTY(); // [inf,inf] [-inf,-inf]
    static const Rect& ZERO(); // [0,0] [0,0]
    static const Rect& UNIT(); // [0,0] [1,1]
};

END_BOOMER_NAMESPACE()
