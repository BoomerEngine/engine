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
class CORE_MATH_API ExactPosition
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ExactPosition);

public:
    //--

    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    //--

    INLINE ExactPosition();
    INLINE ExactPosition(double x_, double y_, double z_);

    INLINE ExactPosition(const ExactPosition& other) = default;
    INLINE ExactPosition(ExactPosition&& other) = default;
    INLINE ExactPosition& operator=(const ExactPosition& other) = default;
    INLINE ExactPosition& operator=(ExactPosition&& other) = default;
    INLINE ~ExactPosition() = default;

    INLINE ExactPosition(const Vector3& pos);
    INLINE ExactPosition& operator=(const Vector3& other);

    ///--

    INLINE ExactPosition operator+(const Vector3& offset) const;
    INLINE ExactPosition& operator+=(const Vector3& offset);
    INLINE ExactPosition operator-(const Vector3& offset) const;
    INLINE ExactPosition& operator-=(const Vector3& offset);

    INLINE ExactPosition operator+(const ExactPosition& offset) const;
    INLINE ExactPosition& operator+=(const ExactPosition& offset);
    INLINE ExactPosition operator-(const ExactPosition& offset) const;
    INLINE ExactPosition& operator-=(const ExactPosition& offset);

    INLINE ExactPosition operator*(float scale) const;
    INLINE ExactPosition& operator*=(float scale);
    INLINE ExactPosition operator*(const Vector3& scale) const;
    INLINE ExactPosition& operator*=(const Vector3& scale);

    INLINE bool operator==(const ExactPosition& other) const;
    INLINE bool operator!=(const ExactPosition& other) const;

    INLINE bool operator==(const Vector3& other) const;
    INLINE bool operator!=(const Vector3& other) const;

    INLINE operator Vector3() const; // auto cast

    INLINE ExactPosition operator-() const;

    //--

    //! calculate the approximate distance between positions
    INLINE float distance(const ExactPosition& base) const;
    INLINE float distance(const Vector3& base) const; // even less accurate

    //! calculate the exact distance between position
    INLINE double exactDistance(const ExactPosition& base) const;

    //! square distance
    INLINE float squareDistance(const ExactPosition& base) const;
    INLINE float squareDistance(const Vector3& base) const; // even less accurate

    //! exact square distance
    INLINE double exactSquareDistance(const ExactPosition& base) const;

    //--

    //! get the root position of the world (0,0,0)
    static const ExactPosition& ZERO();

    //--

    // debug print
    void print(IFormatStream& f) const;

    //--

    // snap to grid
    INLINE void snap(double grid);

    // snap to grid
    INLINE ExactPosition snapped(double grid) const;
    
    //--
};

END_BOOMER_NAMESPACE()
