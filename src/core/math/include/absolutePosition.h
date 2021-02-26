/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\absolute #]
***/

#pragma once

#include "core/reflection/include/reflectionMacros.h"

BEGIN_BOOMER_NAMESPACE()

//--

// Absolute (world space) position
// Used to represent XYZ placement of objects in the world with enough precission for even big worlds
// NOTE: it's implemented as 2 normal vectors because there are many places where this is more sensible than doubles
class CORE_MATH_API AbsolutePosition
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(AbsolutePosition);

public:
    INLINE AbsolutePosition();
    INLINE AbsolutePosition(double x, double y, double z);
    INLINE AbsolutePosition(const Vector3& primary, const Vector3& error = Vector3::ZERO());

    INLINE AbsolutePosition(const AbsolutePosition& other) = default;
    INLINE AbsolutePosition(AbsolutePosition&& other) = default;
    INLINE AbsolutePosition& operator=(const AbsolutePosition& other) = default;
    INLINE AbsolutePosition& operator=(AbsolutePosition&& other) = default;
    INLINE ~AbsolutePosition() = default;

    ///--

    INLINE AbsolutePosition operator+(const Vector3& offset) const;
    INLINE AbsolutePosition& operator+=(const Vector3& offset);
    INLINE AbsolutePosition operator-(const Vector3& offset) const;
    INLINE AbsolutePosition& operator-=(const Vector3& offset);

    INLINE bool operator==(const AbsolutePosition& other) const;
    INLINE bool operator!=(const AbsolutePosition& other) const;

    INLINE Vector3 operator-(const AbsolutePosition& base) const;

    //--

    //! expand into doubles
    INLINE void expand(double& x, double& y, double& z) const;

    //! calculate the approximate distance between position
    INLINE float distance(const AbsolutePosition& base) const;

    //! calculate the exact distance between position
    INLINE double exactDistance(const AbsolutePosition& base) const;

    //! get approximated position, NOTE: zero cost, no cast
    INLINE const Vector3& approximate() const;

    //! get the error term for position approximation, NOTE: zero cost, no cast
    INLINE const Vector3& error() const;

    //--

    //! get the root position of the world (0,0,0)
    static const AbsolutePosition& ROOT();

private:
    Vector3 m_primary;
    Vector3 m_error;

    void normalize();
};

END_BOOMER_NAMESPACE()
