/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\quat #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//-----------------------------------------------------------------------------

/// Quaternion
TYPE_ALIGN(16, class) CORE_MATH_API Quat
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Quat);

public:
    float x, y, z, w;

    //--

    INLINE Quat();
    INLINE Quat(float inX, float inY, float inZ, float inW);
    INLINE Quat(const Vector4 &other);
    INLINE Quat(const Quat &other) = default;
    INLINE Quat(Quat&& other) = default;
    Quat(const Vector3& axis, float angleDeg);

    INLINE Quat& operator=(const Quat &other) = default;
    INLINE Quat& operator=(Quat&& other) = default;

    INLINE bool operator==(const Quat &other) const;
    INLINE bool operator!=(const Quat &other) const;

    INLINE Quat operator-() const;
    INLINE Quat &operator*=(const Quat &other);
    INLINE Quat operator*(const Quat &other) const;
    INLINE float operator|(const Quat & other) const;

    //--

    // reset to identity
    INLINE void identity();

    //! invert (conjugate) quaternion
    INLINE void invert();

    //! get inverted quaternion
    INLINE Quat inverted() const;

    //! normalize to length = 1
    //! NOTE: most operations leave the quat normalized so this is not necessary
    INLINE void normalize();

    //! align this quaternion with another
    INLINE void align(const Quat& other);

    //--

    //! compute distance (in radians) between quaternions
    //! NOTE: slow and uses acos!
    float calcAngle(const Quat & other) const;

    //--

    //! convert to matrix, assumes unit quaternion, fastest
    Matrix toMatrix() const;

    //! convert to matrix, assumes unit quaternion, fastest
    Matrix33 toMatrix33() const;

    //! convert to euler angles
    Angles toRotator() const;

    //--

    //! zero quaternion
    static const Quat& ZERO();

    //! identity quaternion
    static const Quat& IDENTITY();
};

//--

END_BOOMER_NAMESPACE()
