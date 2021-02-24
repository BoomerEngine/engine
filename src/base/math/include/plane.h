/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\plane #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//--

/// a simple plane in 3D space defined as normal vector and distance from origin along it
TYPE_ALIGN(16, class) BASE_MATH_API Plane
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Plane);

public:
    Vector3 n;
    float d;

    //--

    INLINE Plane();
    INLINE Plane(float nx, float ny, float nz, float dist);
    INLINE Plane(const Vector3 &normal, float dist);
    INLINE Plane(const Vector3 &normal, const Vector3 &point);
    INLINE Plane(const Vector3 &a, const Vector3 &b, const Vector3 &c);
    INLINE Plane(const Plane &other) = default;
    INLINE Plane(Plane&& other) = default;
    INLINE Plane& operator=(const Plane &other) = default;
    INLINE Plane& operator=(Plane&& other) = default;

    INLINE bool operator==(const Plane& other) const;
    INLINE bool operator!=(const Plane& other) const;

    INLINE Plane operator-() const;

    //--

    //! calculate distance to point
    INLINE float distance(const Vector3 &point) const;

    //! project point on plane
    INLINE Vector3 project(const Vector3& point) const;

    //! normalize plane (renormalize the normal vector and reduce the D)
    INLINE void normalize();

    //! flip the plane (changes the positive and negative half-space)
    INLINE void flip();

    //! get the flipped plane
    INLINE Plane fliped() const;

    //--

    //! Test point position
    PlaneSide testPoint(const Vector3 &point, float epsilon = PLANE_EPSILON) const;

    //! Test bounding box position
    PlaneSide testBox(const Box &box, float epsilon = PLANE_EPSILON) const;

    //--

    //! test if plane contains given point, returns true if given point belongs to plane
    bool contains(const Vector3 &point, float epsilon = PLANE_EPSILON) const;

    //! Intersect ray with plane
    //! NOTE: this is a single sided intersection, only rays coming from the front side of the plane will be intersected
    bool intersect(const Vector3& origin, const Vector3& direction, float maxLength = VERY_LARGE_FLOAT, float* outEnterDistFromOrigin = nullptr, Vector3* outEntryPoint = nullptr, Vector3* outEntryNormal = nullptr) const;

    //--

    static const Plane& EX();
    static const Plane& EY();
    static const Plane& EZ();
};

//--

END_BOOMER_NAMESPACE(base)