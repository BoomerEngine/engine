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

/// Plane sides - a special bit mask
enum PlaneSide
{
    SIDE_OnPlane = 0,
    SIDE_Positive = 1,
    SIDE_Negative = 2,
    SIDE_Split = 3,
};

//--

class Vector2;
class Vector3;
class Vector4;
class Matrix;
class Matrix33;
class Box;
class Plane;
class Quat;
class Transform;
class Angles;
class XForm2D;
class Color;
class Point;
class Rect;
class OBB;
class Sphere;
class Convex;
class TriMesh;
class Cylinder;
class Capsule;
class ExactPosition;
class EulerTransform;

//--

// normalize angel to -360 to 360 range
INLINE float AngleNormalize(float angle) { return angle - (std::trunc(angle / 360.0f) * 360.0f); }

// get distance between two angles, it's always in range from -180 to 180 deg
extern CORE_MATH_API float AngleDistance(float srcAngle, float srcTarget);

// move given amount of degrees from starting angle towards the target angle
extern CORE_MATH_API float AngleReach(float srcCurrent, float srcTarget, float move);

//--

//! Choose a plane that is the most perpendicular to given directions
extern CORE_MATH_API bool GetMostPerpendicularPlane(const Vector3 &forward, const Vector3 &axis, const Vector3 &point, Plane &outPlane);

//! Calculate intersection of ray with plane
extern CORE_MATH_API bool CalcPlaneRayIntersection(const Vector3& planeNormal, float planeDistance, const Vector3& rayOrigin, const Vector3& rayDir, float rayLength = VERY_LARGE_FLOAT, float* outDistance = nullptr, Vector3* outPosition = nullptr);

//! Calculate intersection of ray with plane
extern CORE_MATH_API bool CalcPlaneRayIntersection(const Vector3& planeNormal, const Vector3& planePoint, const Vector3& rayOrigin, const Vector3& rayDir, float rayLength = VERY_LARGE_FLOAT, float* outDistance = nullptr, Vector3* outPosition = nullptr);

//! Calculate distance to an edge given by two endpoints, can also return closest point
extern CORE_MATH_API float CalcDistanceToEdge(const Vector3& point, const Vector3& a, const Vector3 &b, Vector3* outClosestPoint);

//! Calculate two perpendicular vectors (UV)
extern CORE_MATH_API void CalcPerpendicularVectors(const Vector3& dir, Vector3& outU, Vector3& outV);

//! Calculate normal of triangle from given vertices, if triangle is degenerated it returns zero vector
extern CORE_MATH_API Vector3 TriangleNormal(const Vector3 &a, const Vector3 &b, const Vector3 &c);

//! Calculate normal if the given points create a triangle
extern CORE_MATH_API bool SafeTriangleNormal(const Vector3 &a, const Vector3 &b, const Vector3 &c, Vector3& outN);

//--

//! snap value to a grid
ALWAYS_INLINE float Snap(float val, float grid);

//! snap value to a grid
ALWAYS_INLINE double Snap(double val, double grid);

//! map 0-1 float value to 0-255 ubyte (mostly for LDR colors)
ALWAYS_INLINE uint8_t FloatTo255(float col);

//! map 0-255 ubyte to 0-1 float
ALWAYS_INLINE float FloatFrom255(uint8_t col) { return (float)col / 255.0f; }

//! check if value is power of two
ALWAYS_INLINE bool IsPow2(uint32_t x) { return (x & (x - 1)) == 0; }

//! check if value is power of two
ALWAYS_INLINE bool IsPow2(uint64_t x) { return ( x & ( x-1 )) == 0; }

//! get next power of two value
extern CORE_MATH_API uint32_t NextPow2(uint32_t v);

//! get next power of two value
extern CORE_MATH_API uint64_t NextPow2(uint64_t v);

//! get the "log2" (rounding down) of the given value
extern CORE_MATH_API uint8_t FloorLog2(uint64_t v);

//--

END_BOOMER_NAMESPACE()
