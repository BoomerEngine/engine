/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\cylinder #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//---

/// cylider - constant radius between two points
class BASE_MATH_API Cylinder
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Cylinder);

public:
    Vector4 positionAndRadius = Vector4::ZERO(); //< position (x,y,z), radius (w)
    Vector4 normalAndHeight = Vector4::ZERO();   //< orientation (x,y,z), height (w)

    //--

    INLINE Cylinder() {};
    INLINE Cylinder(const Cylinder& other) = default;
    INLINE Cylinder(Cylinder&& other) = default;
    INLINE Cylinder& operator=(const Cylinder& other) = default;
    INLINE Cylinder& operator=(Cylinder&& other) = default;

    // create cylinder between two positions with given radius
    Cylinder(const Vector3& pos1, const Vector3& pos2, float radius);

    // Create cylinder at given position, pointing along the normal vector with given radius and height
    Cylinder(const Vector3& pos, const Vector3& normal, float radius, float height );

    //--

    // get height of the cylinder
    INLINE float height() const { return normalAndHeight.w; }

    // get radius of the cylinder
    INLINE float radius() const { return positionAndRadius.w; }

    // get position at the bottom side of the cylinder
    INLINE const Vector3& position() const { return positionAndRadius.xyz(); }

    // get the direction between bottom and top part of the cylinder
    INLINE const Vector3& normal() const { return normalAndHeight.xyz(); }

    // calculate the U cylinder vector (vectors in the base plane perpendicular to the normal)
    void baseVectors(Vector3& outU, Vector3& outV) const;

    // get position at the bottom side of the cylinder
    Vector3 position2() const;

    // get center of mass for the cylinder
    Vector3 center() const;

    // Compute volume of the cylinder
    float volume() const;

    // compute bounding box of the shape in the coordinates of the shape
    Box bounds() const;

    //--

    // check given point is inside the cylinder
    bool contains(const Vector3& point) const;

    // intersect this convex shape with ray, returns distance to point of entry
    bool intersect(const Vector3& origin, const Vector3& direction, float maxLength = VERY_LARGE_FLOAT, float* outEnterDistFromOrigin = nullptr, Vector3* outEntryPoint = nullptr, Vector3* outEntryNormal = nullptr) const;

    //--
};

//---

END_BOOMER_NAMESPACE(base)