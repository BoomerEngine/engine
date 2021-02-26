/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

// translate plane by offset
extern CORE_MATH_API void TranslatePlane(Plane& plane, const Vector3& offset);

// transform plane by matrix
extern CORE_MATH_API void TransformPlane(Plane& plane, const Matrix& matrix);

// transform list of planes by matrix, flips the planes if the det was negative
extern CORE_MATH_API void TransformPlanes(Plane* planes, uint32_t numPlanes, const Matrix& matrix);

// compute the corners of box
extern CORE_MATH_API void CalcBoxCorners(const Vector3& pos, const Vector3& edgeX, const Vector3& edgeY, const Vector3& edgeZ, Vector3* outVertices);

// compute the corners of axis aligned box
extern CORE_MATH_API void CalcAlignedBoxCorners(const Vector3& boxMin, const Vector3& boxMax, Vector3* outVertices);

// compute circle points on a plane with given normal
// NOTE: numPoints+1 vertices are output
extern CORE_MATH_API void CalcCircleVertices(const Vector3& origin, const Vector3& normal, float radius, uint32_t numPoints, Vector3* outVertices);

// compute circle vertices
// NOTE: numPoints+1 vertices are output
extern CORE_MATH_API void CalcCircleVertices(const Vector3& origin, const Vector3& u, const Vector3& v, uint32_t numPoints, Vector3* outVertices);

// solve (find roots) of a quadratic equation (ax^2 + bx + c = 0)
extern CORE_MATH_API bool SolveQuadraticEquation(float a, float b, float c, float& x1, float& x2);

// check if this convex hull contains a given point
extern CORE_MATH_API bool PointInPlaneList(const Plane* planes, uint32_t numPlanes, const Vector3& point);

// intersect this convex shape with ray, returns distance to point of entry
extern CORE_MATH_API bool IntersectPlaneList(const Plane* planes, uint32_t numPlanes, const Vector3& origin, const Vector3& direction, float maxLength = VERY_LARGE_FLOAT, float* outEnterDistFromOrigin = nullptr, Vector3* outEntryPoint = nullptr, Vector3* outEntryNormal = nullptr);

// intersect a triangle with a ray
extern CORE_MATH_API bool IntersectTriangleRay(const Vector3& origin, const Vector3& dir, const Vector3& v0, const Vector3& v1, const Vector3& v2, float maxDist = VERY_LARGE_FLOAT, float* outDist = nullptr, float* outU = nullptr, float* outV = nullptr, bool cull = true, float additionalEpsilon = 0.0f);

// test if a ray intersects a box
extern CORE_MATH_API bool IntersectBoxRay(const Vector3& origin, const Vector3& dir, const Vector3& invDir, float maxLength, const Vector3& boxMin, const Vector3& boxMax, float* outDist = nullptr);

// having a 2 points compute a circle that passes though all of them (basically the A-B segments becomes the diameter of the circle)
extern CORE_MATH_API void CalcCircle(const Vector2& a, const Vector2& b, Vector2& outCenter, float& outRadius);

// having a 3 points compute a circle that passes though all of them
extern CORE_MATH_API bool CalcCircle(const Vector2& a, const Vector2& b, const Vector2& c, Vector2& outCenter, float& outRadius);
    
//--

END_BOOMER_NAMESPACE()
