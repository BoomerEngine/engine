/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\quat #]
***/

#pragma once

namespace base
{
    //-----------------------------------------------------------------------------

    /// Quaternion
    TYPE_ALIGN(16, class) BASE_MATH_API Quat
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

        //! transform vector by this quaternion
        Vector3 transformVector(const Vector3& v) const;

        //! transform vector by this quaternion
        void transformVector(const Vector3& v, Vector3& ret) const;

        //! transform vector by inverse of this quaternion
        Vector3 transformInvVector(const Vector3& v) const;

        //! transform vector by inverse of this quaternion
        void transformInvVector(const Vector3& v, Vector3& ret) const;

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

    extern BASE_MATH_API float Dot(const Quat &a, const Quat &b);
    extern BASE_MATH_API Quat LinearLerp(const Quat &a, const Quat &b, float fraction);
    extern BASE_MATH_API Quat Lerp(const Quat &a, const Quat &b, float fraction);
    extern BASE_MATH_API Quat Concat(const Quat &a, const Quat &b);

    //--

} // base