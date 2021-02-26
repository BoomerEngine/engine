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
class AbsolutePosition;
class AbsoluteTransform;
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

//! Get normal component of vector
extern CORE_MATH_API Vector2 NormalPart(const Vector2 &a, const Vector2 &normal);

//! Get tangent component of vector
extern CORE_MATH_API Vector2 TangentPart(const Vector2 &a, const Vector2 &normal);

//! Get normal component of vector
extern CORE_MATH_API Vector3 NormalPart(const Vector3 &a, const Vector3 &normal);

//! Get tangent component of vector
extern CORE_MATH_API Vector3 TangentPart(const Vector3 &a, const Vector3 &normal);

//! Get normal component of vector
extern CORE_MATH_API Vector4 NormalPart(const Vector4 &a, const Vector4 &normal);

//! Get tangent component of vector
extern CORE_MATH_API Vector4 TangentPart(const Vector4 &a, const Vector4 &normal);

//! Limit length of the vector
extern CORE_MATH_API Vector2 ClampLength(const Vector2& a, float maxLength);

//! Limit length of the vector
extern CORE_MATH_API Vector3 ClampLength(const Vector3& a, float maxLength);

//! Limit length of the vector
extern CORE_MATH_API Vector4 ClampLength(const Vector4& a, float maxLength);

//! Set length of the vector
extern CORE_MATH_API Vector2 SetLength(const Vector2& a, float maxLength);

//! Set length of the vector
extern CORE_MATH_API Vector3 SetLength(const Vector3& a, float maxLength);

//! Set length of the vector
extern CORE_MATH_API Vector4 SetLength(const Vector4& a, float maxLength);

//--

//! snap to grid of given size, slow
extern CORE_MATH_API float Snap(float val, float grid);

//! snap 2D position to grid of given size, slow
extern CORE_MATH_API Vector2 Snap(const Vector2& val, float grid);

//! snap 3D position to grid of given size, slow
extern CORE_MATH_API Vector3 Snap(const Vector3& val, float grid);

//! snap rotation to grid of given size, slow
extern CORE_MATH_API Angles Snap(const Angles& val, float grid);

//! snap point to a grid
extern CORE_MATH_API Point Snap(const Point& val, int grid);
    
//! snap absolute position to a grid
extern CORE_MATH_API AbsolutePosition Snap(const AbsolutePosition& a, float grid);

//--

//! component-wise min vector
extern CORE_MATH_API Vector2 Min(const Vector2& a, const Vector2& b);

//! component-wise min vector
extern CORE_MATH_API Vector3 Min(const Vector3& a, const Vector3& b);

//! component-wise min vector
extern CORE_MATH_API Vector4 Min(const Vector4& a, const Vector4& b);

//! component-wise min point
extern CORE_MATH_API Point Min(const Point& a, const Point& b);

//! component-wise min of two rectangles
extern CORE_MATH_API Rect Min(const Rect& a, const Rect& b);

//! component-wise min of two colors
extern CORE_MATH_API Color Min(const Color& a, const Color& b);

//! component-wise min of position components
extern CORE_MATH_API AbsolutePosition Min(const AbsolutePosition& a, const AbsolutePosition& b);

//! component-wise min of two angles
extern CORE_MATH_API Angles Min(const Angles& a, const Angles& b);

//--

//! component-wise max vector
extern CORE_MATH_API Vector2 Max(const Vector2& a, const Vector2& b);

//! component-wise max vector
extern CORE_MATH_API Vector3 Max(const Vector3& a, const Vector3& b);

//! component-wise max vector
extern CORE_MATH_API Vector4 Max(const Vector4& a, const Vector4& b);

//! component-wise max point
extern CORE_MATH_API Point Max(const Point& a, const Point& b);

//! component-wise max of two rectangles
extern CORE_MATH_API Rect Max(const Rect& a, const Rect& b);

//! component-wise max of two colors
extern CORE_MATH_API Color Max(const Color& a, const Color& b);

//! component-wise max of position components
extern CORE_MATH_API AbsolutePosition Max(const AbsolutePosition& a, const AbsolutePosition& b);

//! component-wise max of two angles
extern CORE_MATH_API Angles Max(const Angles& a, const Angles& b);

//--

//! clamp components to range
extern CORE_MATH_API Vector2 Clamp(const Vector2& a, const Vector2& minV, const Vector2& maxV);

//! clamp components to range
extern CORE_MATH_API Vector3 Clamp(const Vector3& a, const Vector3& minV, const Vector3& maxV);

//! clamp components to range
extern CORE_MATH_API Vector4 Clamp(const Vector4& a, const Vector4& minV, const Vector4& maxV);

//! clamp components to range
extern CORE_MATH_API Point Clamp(const Point& a, const Point& minV, const Point& maxV);

//! clamp rectangle coordinates to fit inside given one
extern CORE_MATH_API Rect Clamp(const Rect& a, const Rect& limit);

//! Clamp color to range
extern CORE_MATH_API Color Clamp(const Color& a, const Color& minV, const Color& maxV);

//! Limit position component to range
extern CORE_MATH_API AbsolutePosition Clamp(const AbsolutePosition& a, const AbsolutePosition& minV, const AbsolutePosition& maxV);

//! Limit angles to range
extern CORE_MATH_API Angles Clamp(const Angles& a, const Angles& minV, const Angles& maxV);

//--

//! clamp components to range (all to same range)
extern CORE_MATH_API Vector2 Clamp(const Vector2& a, float minF=0.0f, float maxF=1.0f);

//! clamp components to range (all to same range)
extern CORE_MATH_API Vector3 Clamp(const Vector3& a, float minF=0.0f, float maxF=1.0f);

//! clamp components to range (all to same range)
extern CORE_MATH_API Vector4 Clamp(const Vector4& a, float minF = 0.0f, float maxF = 1.0f);

//! clamp components to range
extern CORE_MATH_API Point Clamp(const Point& a, int minF, int maxF);

//! clamp rectangle coordinates to fit inside given range
extern CORE_MATH_API Rect Clamp(const Rect& a, int minF, int maxF);

//! clamp color to range
extern CORE_MATH_API Color Clamp(const Color& a, uint8_t minF, uint8_t maxF);

//! limit position value to range
extern CORE_MATH_API AbsolutePosition Clamp(const AbsolutePosition& a, double minF, double maxF);

//! limit angles to range
extern CORE_MATH_API Angles Clamp(const Angles& a, float minF, float maxF);

//--

//! 2D dot product
extern CORE_MATH_API float Dot(const Vector2& a, const Vector2& b);

//! 3D dot product
extern CORE_MATH_API float Dot(const Vector3& a, const Vector3& b);

//! 4D dot product
extern CORE_MATH_API float Dot(const Vector4& a, const Vector4& b);

//! Quaternion dot product
extern CORE_MATH_API float Dot(const Quat& a, const Quat& b);

//! Rotation dot product - the dot product of their respective "forward" vectors
extern CORE_MATH_API float Dot(const Angles& a, const Angles& b);

//--

//! 3D vector cross product
extern CORE_MATH_API Vector3 Cross(const Vector3& a, const Vector3& b);

//--

//! map 0-1 float value to 0-255 ubyte (mostly for LDR colors)
extern CORE_MATH_API uint8_t FloatTo255(float col);

//! map 0-255 ubyte to 0-1 float
INLINE float FloatFrom255(uint8_t col) { return (float)col / 255.0f; }

//! check if value is power of two
INLINE bool IsPow2(uint64_t x) { return ( x & ( x-1 )) == 0; }

//! get next power of two value
extern CORE_MATH_API uint32_t NextPow2(uint32_t v);

//! get next power of two value
extern CORE_MATH_API uint64_t NextPow2(uint64_t v);

//! get the "log2" (rounding down) of the given value
extern CORE_MATH_API uint8_t FloorLog2(uint64_t v);

//--

//! linear interpolation of scalar value
INLINE float Lerp(float valA, float valB, float fraction) { return valA + fraction * ( valB - valA ); }

//! linear interpolation of scalar value
INLINE double Lerp(double valA, double valB, float fraction) { return valA + fraction * ( valB - valA ); }

//! interpolate 2D vector
extern CORE_MATH_API Vector2 Lerp(const Vector2& a, const Vector2& b, float frac);

//! interpolate 3D vector
extern CORE_MATH_API Vector3 Lerp(const Vector3& a, const Vector3& b, float frac);

//! interpolate 4D vector
extern CORE_MATH_API Vector4 Lerp(const Vector4& a, const Vector4& b, float frac);

//! linear lerp quaternions (like vectors) - must be aligned
extern CORE_MATH_API Quat LinearLerp(const Quat& a, const Quat& b, float fraction);

//! interpolate quaternions (spherical lerp)
extern CORE_MATH_API Quat Lerp(const Quat& a, const Quat& b, float fraction);

//! interpolate two Euler angles
extern CORE_MATH_API Angles Lerp(const Angles& a, const Angles& b, float frac);

//! interpolate two Euler angles choosing the shortest route
extern CORE_MATH_API Angles LerpNormalized(const Angles& a, const Angles& b, float frac);

//! lerp color (in general the fraction should not go outside 0-1 range)
extern CORE_MATH_API Color Lerp(const Color& a, const Color& b, float frac);

//! lerp color with a fraction that goes from 0 to 256 (yes, 256!)
//! NOTE: does not interpolate the alpha, sets it to 255
extern CORE_MATH_API Color Lerp256(const Color& a, const Color& b, uint32_t frac0to256);

//! interpolate absolute position
extern CORE_MATH_API AbsolutePosition Lerp(const AbsolutePosition& a, const AbsolutePosition& b, float frac);

//! interpolate absolute transformation
extern CORE_MATH_API AbsoluteTransform Lerp(const AbsoluteTransform& a, const AbsoluteTransform& b, float frac);

//--

//! concatenate quaterions - L to R multiplication order
extern CORE_MATH_API Quat Concat(const Quat& a, const Quat& b);

//! concatenate transforms - L to R multiplication order
extern CORE_MATH_API Transform Concat(const Transform& a, const Transform& b);

//! concatenate 2D transforms - L to R multiplication order
extern CORE_MATH_API XForm2D Concat(const XForm2D& a, const XForm2D& b);

//! concatenate matrices - L to R multiplication order
extern CORE_MATH_API Matrix Concat(const Matrix& a, const Matrix& b);

//! concatenate 3x3 matrices - L to R multiplication order
extern CORE_MATH_API Matrix33 Concat(const Matrix33& a, const Matrix33& b);

//--

END_BOOMER_NAMESPACE()
