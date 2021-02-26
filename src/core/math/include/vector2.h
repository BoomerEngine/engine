/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\vector2 #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

/// A point or vector in 2D space (texcoord mostly)
class CORE_MATH_API Vector2
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Vector2);

public:
    float x;        //!< X vector component
    float y;        //!< Y vector component

    //--

    INLINE Vector2();
    INLINE Vector2(float inX, float inY);
    INLINE Vector2(const Vector2 &other) = default;
    INLINE Vector2(Vector2 &&other) = default;
    INLINE Vector2& operator=(const Vector2 &other) = default;
    INLINE Vector2& operator=(Vector2 &&other) = default;

    INLINE bool operator==(const Vector2 &other) const;
    INLINE bool operator!=(const Vector2 &other) const;

    INLINE Vector2 operator-() const;
    INLINE Vector2 operator+(const Vector2 &other) const;
    INLINE Vector2 operator+(float value) const;
    INLINE Vector2 operator-(const Vector2 &other) const;
    INLINE Vector2 operator-(float value) const;
    INLINE Vector2 operator*(const Vector2 &other) const;
    INLINE Vector2 operator*(float value) const;
    friend Vector2 operator*(float value, const Vector2 &other);
    INLINE Vector2 operator/(const Vector2 &other) const;
    INLINE Vector2 operator/(float value) const;
    INLINE Vector2 &operator+=(const Vector2 &other);
    INLINE Vector2 &operator+=(float value);
    INLINE Vector2 &operator-=(const Vector2 &other);
    INLINE Vector2 &operator-=(float value);
    INLINE Vector2 &operator*=(const Vector2 &other);
    INLINE Vector2 &operator*=(float value);
    INLINE Vector2 &operator/=(const Vector2 &other);
    INLINE Vector2 &operator/=(float value);
    INLINE float operator|(const Vector2 &other) const;
    INLINE Vector2 operator~() const;

    //--

    //! returns minimal from 2 vector components
    INLINE float minValue() const;

    //! returns maximal from 2 vector components
    INLINE float maxValue() const;

    //! get sum of the components
    INLINE float sum() const;

    //! get product of the components
    INLINE float trace() const;

    //! get vector with absolute components
    INLINE Vector2 abs() const;

    //! round to nearest integer
    INLINE Vector2 round() const;

    //! get fractional part
    INLINE Vector2 frac() const;

    //! truncate fractional part
    INLINE Vector2 trunc() const;

    //! round to lower integer
    INLINE Vector2 floor() const;

    //! round to higher integer
    INLINE Vector2 ceil() const;

    //! square length of vector
    INLINE float squareLength() const;

    //! length of vector
    INLINE float length() const;

    //! 1/length, safe
    INLINE float invLength() const;

    //! check if vector is exactly equal to zero vector
    INLINE bool isZero() const;

    //! check if vector is close to zero vector
    INLINE bool isNearZero(float epsilon = SMALL_EPSILON) const;

    //! check if vector is near enough to another one
    INLINE bool isSimilar(const Vector2 &other, float epsilon = SMALL_EPSILON) const;

    //! normalize vector, does nothing if zero, returns length of the vector before normalization
    INLINE float normalize();

    //! Return normalized vector
    INLINE Vector2 normalized() const;

    //! normalize vector, no checks, returns length
    INLINE float normalizeFast();

    //! Return fast normalized vector, no checks
    INLINE Vector2 normalizedFast() const;

    //! Return perpendicular vector (-y,x)
    INLINE Vector2 prep() const;

    //! Calculate distance to other point
    INLINE float distance(const Vector2 &other) const;

    //! Calculate square distance to other point
    INLINE float squareDistance(const Vector2 &other) const;

    //! get index of largest axis
    INLINE int largestAxis() const;

    //! get index of smallest axis
    INLINE int smallestAxis() const;

    //! get the 1/x 1/y vector, safe
    INLINE Vector2 oneOver() const;

    //! get the 1/x 1/y vector, unsafe
    INLINE Vector2 oneOverFast() const;

    //--

    INLINE Vector2 xx() const;
    INLINE Vector2 yy() const;
    INLINE Vector2 yx() const;
    INLINE Vector3 xyz(float z = 0.0f) const;
    INLINE Vector4 xyzw(float z = 0.0f, float w = 1.0f) const;

    INLINE Vector2& xy();
    INLINE const Vector2& xy() const;

    //--

    static const Vector2& ZERO();
    static const Vector2& ONE();
    static const Vector2& EX();
    static const Vector2& EY();
    static const Vector2& INF();

    //--

    void print(IFormatStream& f) const;

private:
    INLINE Vector2 _xy() const;
};

//--

END_BOOMER_NAMESPACE()
