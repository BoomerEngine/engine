/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\capsule #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//---

/// capsule
class BASE_MATH_API Capsule
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Capsule);

public:
    Vector4 positionAndRadius = Vector4::ZERO(); //< position (x,y,z), radius (w)
    Vector4 normalAndHeight = Vector4::ZERO(); //< orientation (x,y,z), height (w)

    //--

    INLINE Capsule() {};
    INLINE Capsule(const Capsule& other) = default;
    INLINE Capsule(Capsule&& other) = default;
    INLINE Capsule& operator=(const Capsule& other) = default;
    INLINE Capsule& operator=(Capsule&& other) = default;

    // create capsule between two positions with given radius
    Capsule(const Vector3& pos1, const Vector3& pos2, float radius);

    // Create capsule at given position, pointing along the normal vector with given radius and height
    // NOTE: radius is on top of the capsule height
    Capsule(const Vector3& pos, const Vector3& normal, float radius, float height);

    //--

    // get height of the Capsule
    INLINE float height() const { return normalAndHeight.w; }

    // get radius of the Capsule
    INLINE float radius() const { return positionAndRadius.w; }

    // get position at the bottom side of the Capsule
    INLINE const Vector3& position() const { return *(const Vector3*) &positionAndRadius; }

    // get the direction between bottom and top part of the Capsule
    INLINE const Vector3& normal() const { return *(const Vector3*) &normalAndHeight; }

    // get position at the bottom side of the Capsule
    Vector3 position2() const;

    // get center of mass for the Capsule
    Vector3 center() const;

    // compute the volume of the capsule (inner cylinder + sphere)
    float volume() const;

    // compute bounding box of the shape in the coordinates of the shape
    Box bounds() const;

    //--

    // check if this capsule contains a given point
    bool contains(const Vector3& point) const;

    // intersect this convex shape with ray, returns distance to point of entry
    bool intersect(const Vector3& origin, const Vector3& direction, float maxLength = VERY_LARGE_FLOAT, float* outEnterDistFromOrigin = nullptr, Vector3* outEntryPoint = nullptr, Vector3* outEntryNormal = nullptr) const;
};

//---

END_BOOMER_NAMESPACE(base)