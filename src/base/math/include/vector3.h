/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\vector3 #]
***/

#pragma once

namespace base
{
    //-----------------------------------------------------------------------------

    /// A point or vector in 3D space
    class BASE_MATH_API Vector3
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(Vector3);

    public:
        float x, y ,z;

        //---

        INLINE Vector3();
        INLINE Vector3(const Vector3& other) = default;
        INLINE Vector3(float inX, float inY, float inZ);
        INLINE Vector3(Vector3&& other) = default;
        INLINE Vector3& operator=(const Vector3& other) = default;
        INLINE Vector3& operator=(Vector3&& other) = default;

        INLINE bool operator==(const Vector3 &other) const;
        INLINE bool operator!=(const Vector3 &other) const;

        //--

        //! returns minimal from 3 vector components
        INLINE float minValue() const;

        //! returns maximal from 3 vector components
        INLINE float maxValue() const;

        //! get sum of all components
        INLINE float sum() const;

        //! get product of all components
        INLINE float trace() const;

        //! get vector with absolute components
        INLINE Vector3 abs() const;

        //! round to nearest integer
        INLINE Vector3 round() const;

        //! get fractional part
        INLINE Vector3 frac() const;

        //! truncate fractional part
        INLINE Vector3 trunc() const;

        //! round to lower integer
        INLINE Vector3 floor() const;

        //! round to higher integer
        INLINE Vector3 ceil() const;

        //! vector square length
        INLINE float squareLength() const;

        //! vector length
        INLINE float length() const;

        //! 1/vector length, safe
        INLINE float invLength() const;

        //! check if vector is exactly equal to zero vector
        INLINE bool isZero() const;

        //! check if vector is close to zero vector
        INLINE bool isNearZero(float epsilon = SMALL_EPSILON)  const;

        //! check if vector is near enough to another one
        INLINE bool isSimilar(const Vector3 &other, float epsilon = SMALL_EPSILON) const;

        //! normalize vector, does nothing if vector is zero, returns vector length
        INLINE float normalize();

        //! normalize vector, no checks, returns length
        INLINE float normalizeFast();

        //! return normalized vector, returns the same vector if it can't be normalized
        INLINE Vector3 normalized() const;

        //! return fast normalized vector, no checks
        INLINE Vector3 normalizedFast() const;

        //! get transformed vector by a vector space
        INLINE Vector3 transform(const Vector3& x, const Vector3& y, const Vector3& z = Vector3::ZERO()) const;

        //! get the 1/x 1/y 1/z vector
        //! NOTE: if any of the components is zero the inverse will be returned as zero as well
        INLINE Vector3 oneOver() const;

        //! get the 1/x 1/y 1/z vector
        //! NOTE: if any of the components is zero this will cause problems, use the oneOverSafe when there's risc of that
        INLINE Vector3 oneOverFast() const;

        //! calculate distance to other point
        INLINE float distance(const Vector3 &other) const;

        //! calculate square distance to other point
        INLINE float squareDistance(const Vector3 &other) const;

        //! get index of largest axis
        INLINE int largestAxis() const;

        //! get index of smallest axis
        INLINE int smallestAxis() const;

        //---

        // extract sub vectors
        INLINE Vector2 xx() const;
        INLINE Vector2 yy() const;
        INLINE Vector2 zz() const;
        INLINE Vector2 yx() const;
        INLINE Vector2 xz() const;
        INLINE Vector2 zx() const;
        INLINE Vector2 zy() const;

        INLINE Vector3 xxx() const;
        INLINE Vector3 xxy() const;
        INLINE Vector3 xxz() const;
        INLINE Vector3 xyx() const;
        INLINE Vector3 xyy() const;
        INLINE Vector3 xzx() const;
        INLINE Vector3 xzy() const;
        INLINE Vector3 xzz() const;
        INLINE Vector3 yxx() const;
        INLINE Vector3 yxy() const;
        INLINE Vector3 yxz() const;
        INLINE Vector3 yyx() const;
        INLINE Vector3 yyy() const;
        INLINE Vector3 yyz() const;
        INLINE Vector3 yzx() const;
        INLINE Vector3 yzy() const;
        INLINE Vector3 yzz() const;
        INLINE Vector3 zxx() const;
        INLINE Vector3 zxy() const;
        INLINE Vector3 zxz() const;
        INLINE Vector3 zyx() const;
        INLINE Vector3 zyy() const;
        INLINE Vector3 zyz() const;
        INLINE Vector3 zzx() const;
        INLINE Vector3 zzy() const;
        INLINE Vector3 zzz() const;

        INLINE Vector4 xyzw(float w = 1.0f) const;

        INLINE Vector2& xy();
        INLINE Vector2& yz();
        INLINE Vector3& xyz();

        INLINE const Vector2& xy() const;
        INLINE const Vector2& yz() const;
        INLINE const Vector3& xyz() const;

        //---

        // math operators
        INLINE Vector3 operator-() const;
        INLINE Vector3 operator+(const Vector3 &other) const;
        INLINE Vector3 operator+(float value) const;
        INLINE Vector3 operator-(const Vector3 &other) const;
        INLINE Vector3 operator-(float value) const;
        INLINE Vector3 operator*(const Vector3 &other) const;
        INLINE Vector3 operator*(float value) const;
        friend Vector3 operator*(float value, const Vector3 &other);
        INLINE Vector3 operator/(const Vector3 &other) const;
        INLINE Vector3 operator/(float value) const;
        INLINE Vector3 &operator+=(const Vector3 &other);
        INLINE Vector3 &operator+=(float value);
        INLINE Vector3 &operator-=(const Vector3 &other);
        INLINE Vector3 &operator-=(float value);
        INLINE Vector3 &operator*=(const Vector3 &other);
        INLINE Vector3 &operator*=(float value);
        INLINE Vector3 &operator/=(const Vector3 &other);
        INLINE Vector3 &operator/=(float value);
        INLINE float operator|(const Vector3 &other) const;
        INLINE Vector3 operator^(const Vector3 &other) const;
        INLINE float operator[](int index) const;
        INLINE float& operator[](int index);

        //---

        //! convert to rotation that will rotate from EX to this vector
        //! NOTE: some orientations have multiple rotators (like straight up)
        Angles toRotator() const;

        //---

        static const Vector3& ZERO();
        static const Vector3& ONE();
        static const Vector3& EX();
        static const Vector3& EY();
        static const Vector3& EZ();
        static const Vector3& INF();

        //---

        //! Position based interface for polygons
        INLINE Vector3& position() { return *this; }
        INLINE const Vector3& position() const { return *this; }

        //--

        void print(IFormatStream& f) const;

    private:
        INLINE Vector2 _xy() const;
        INLINE Vector2 _yz() const;
        INLINE Vector3 _xyz() const;
    };

    //--

} // base