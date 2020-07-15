/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\obb #]
***/

#pragma once

namespace base
{
    //---

    /// oriented bounding box
    class BASE_MATH_API OBB
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(OBB);

    public:
        //--

        Vector4 positionAndLength = Vector4::ZERO(); //! position (x,y,z), edge3 length (w)
        Vector4 edge1AndLength = Vector4::ZERO(); //! foward, edge1 - normalized, length (w)
        Vector4 edge2AndLength = Vector4::ZERO(); //! right, edge2 - normalized, length (w)

        //--


        INLINE OBB() {};
        INLINE OBB(const OBB& other) = default;
        INLINE OBB(OBB&& other) = default;
        INLINE OBB& operator=(const OBB& other) = default;
        INLINE OBB& operator=(OBB&& other) = default;

        // create OBB at given position
        OBB(const Vector3& pos, const Vector3& edge1, const Vector3& edge2, const Vector3& edge3);

        // create OBB from box at given orientation
        OBB(const Box& box, const Matrix& transform);

        //--

        // get position of the OOB corner
        INLINE const Vector3& position() const { return positionAndLength.xyz(); }

        // get the first edge vector
        INLINE const Vector3& edgeA() const { return edge1AndLength.xyz(); }

        // get the second edge vector
        INLINE const Vector3& edgeB() const { return edge2AndLength.xyz(); }

        // calculate center of mass
        Vector3 center() const;

        // calculate the 3rd edge
        Vector3 edgeC() const;

        // calculate corners of the box
        void corners(Vector3* outCorners) const;

        //--

        // Compute volume of the OBB
        float volume() const;

        // compute bounding box of the shape in the coordinates of the shape
        Box bounds() const;

        //--
        
        // check if this convex hull contains a given point
        bool contains(const Vector3& point) const;

        // intersect this convex shape with ray, returns distance to point of entry
        bool intersect(const Vector3& origin, const Vector3& direction, float maxLength = VERY_LARGE_FLOAT, float* outEnterDistFromOrigin = nullptr, Vector3* outEntryPoint = nullptr, Vector3* outEntryNormal = nullptr) const;

        //--
    };

    //---

} // base