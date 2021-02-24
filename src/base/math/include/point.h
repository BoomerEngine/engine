/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\point #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//---

/// 2D integer point
class BASE_MATH_API Point
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Point);

public:
    int x, y;

    //---

    INLINE Point();
    INLINE Point(int ax, int ay);
    INLINE Point(uint32_t ax, uint32_t ay);
    INLINE Point(float ax, float ay); // rounds to nearest
    INLINE Point(const Vector2& other); // rounds to nearest
    INLINE Point(const Vector3& other); // rounds to nearest
    INLINE Point(const Vector4& other); // rounds to nearest
    INLINE Point(const Point &other) = default;
    INLINE Point(Point&& other) = default;
    INLINE Point& operator=(const Point &other) = default;
    INLINE Point& operator=(Point&& other) = default;

    INLINE const int &operator()(int i) const;
    INLINE int &operator()(int i);

    INLINE bool operator==(const Point &other) const;
    INLINE bool operator!=(const Point &other) const;

    INLINE Point operator+=(const Point &other);
    INLINE Point operator-=(const Point &other);
    INLINE Point operator+(const Point &other) const;
    INLINE Point operator-(const Point &other) const;

    //---

    //! Get distance to other point
    INLINE float distanceTo(const Point& other) const;

    //! Get squared distance to other point
    INLINE float squaredDistanceTo(const Point& other) const;

    //---

    //! Convert to 2 component floating point vector
    Vector2 toVector() const;

    //---

    static const Point& ZERO();
};

//--

END_BOOMER_NAMESPACE(base)