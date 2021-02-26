/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//----------------------------------------
//-- Random number generator interface
//-- NOTE: some applications require abstract class
    
struct CORE_MATH_API IRandom
{
    virtual ~IRandom();

    // reset seed
    virtual void seed(uint32_t seed) = 0;

    // get next random value, full 32bit of randomness
    virtual uint32_t next() = 0;

    //--

    // single random scalar [0,1) range
    ALWAYS_INLINE double unit();

    // 2D vector [0,1)
    ALWAYS_INLINE Vector2 unit2(); 

    // 3D vector [0,1)
    ALWAYS_INLINE Vector3 unit3();

    // 3D vector [0,1)
    ALWAYS_INLINE Vector4 unit4(); 

    // [min,max>
    ALWAYS_INLINE double range(double min, double max);

    // [0, max-1], returns 0 if max is 0
    ALWAYS_INLINE uint32_t range(uint32_t max);
};

//----------------------------------------
//-- Very simple random number generator

struct CORE_MATH_API FastRandState : public IRandom
{
    FastRandState(uint64_t seed = 0);

    virtual void seed(uint32_t val) override final;
    virtual uint32_t next() override final;

private:
    uint64_t state = 0;
};    

//----------------------------------------
//-- Mersenne-Twister random number generator

struct CORE_MATH_API MTRandState : public IRandom
{
    MTRandState(uint32_t seed = 0);

    virtual void seed(uint32_t val) override final;
    virtual uint32_t next() override final;
    
private:
    static const uint32_t SIZE = 624;

    uint32_t MT[SIZE];
    uint32_t MT_TEMPERED[SIZE];
    uint32_t index = SIZE;

    void generate();
};

//----------------------------------------
//-- Common stuff

// convert 2 uniform random variables to a random 2D point in a rectangle with uniform distribution
extern CORE_MATH_API Vector2 RandRectPoint(const Vector2& rand, const Vector2& min, const Vector2& max);

// convert 2 uniform random variables to a random 2D point in a circle with uniform distribution, NOTE: circle boundary is excluded
extern CORE_MATH_API Vector2 RandCirclePoint(const Vector2& rand, const Vector2& center, float radius);

// convert 3 uniform random variables to a random 3D point inside a box
extern CORE_MATH_API Vector3 RandBoxPoint(const Vector3& rand, const Vector3& min, const Vector3& max);

// convert 3 uniform random variables to a random 3D point inside a box
extern CORE_MATH_API Vector3 RandBoxPoint(const Vector3& rand, const Box& box);

// convert 3 uniform random variables to a random 3D point inside a box
extern CORE_MATH_API Vector3 RandBoxPoint(const Vector3& rand, const OBB& box);

// convert 3 uniform random variables to a random 3D point inside an sphere
extern CORE_MATH_API Vector3 RandSpherePoint(const Vector3& rand);

// convert 3 uniform random variables to a random 3D point inside a sphere
extern CORE_MATH_API Vector3 RandSpherePoint(const Vector3& rand, const Vector3& center, float radius);

// convert 3 uniform random variables to a random 3D point inside a sphere
extern CORE_MATH_API Vector3 RandSpherePoint(const Vector3& rand, const Sphere& sphere);

// convert 2 (TWO) uniform random variables to a random 3D point ON A SURFACE of the unit sphere
extern CORE_MATH_API Vector3 RandSphereSurfacePoint(const Vector2& rand);

// convert 2 (TWO) uniform random variables to a random 3D point ON A SURFACE of the sphere
extern CORE_MATH_API Vector3 RandSphereSurfacePoint(const Vector2& rand, const Vector3& center, float radius);

// convert 3 uniform random variables to a random 3D point inside a sphere
extern CORE_MATH_API Vector3 RandSphereSurfacePoint(const Vector2& rand, const Sphere& sphere);

// convert 2 (TWO) uniform random variables to a random 3D point on a unit hemi-sphere at 0,0,0 and with N=0,0,1 (standard tangent-space hemisphere)
extern CORE_MATH_API Vector3 RandHemiSphereSurfacePoint(Vector2 rand);

// convert 2 (TWO)
extern CORE_MATH_API Vector3 RandSphereSurfacePointFast(Vector2 rand);

// convert 2 (TWO) uniform random variables to a random 3D point on a unit hemi-sphere at 0,0,0 with specified N
extern CORE_MATH_API Vector3 RandHemiSphereSurfacePoint(const Vector2& rand, const Vector3& N);

// convert 2 (TWO) uniform random variables to a random 3D point on a hemi-sphere facing given direction
extern CORE_MATH_API Vector3 RandHemiSphereSurfacePoint(const Vector2& rand, const Vector3& center, float radius, const Vector3& normal);

// convert 2 (TWO) uniform random variables to a random point inside a unit triangle (basically we return the barycentric coordinates)
extern CORE_MATH_API Vector3 RandTrianglePoint(const Vector2& rand);

// convert 2 (TWO) uniform random variables to a random point inside a triangle
extern CORE_MATH_API Vector3 RandTrianglePoint(const Vector2& rand, const Vector3& a, const Vector3& b, const Vector3& c);

//---

// get random 2D point in a rectangle with uniform distribution
template< typename T >
INLINE Vector2 RandRectPoint(T& rand, const Vector2& min, const Vector2& max);

// get random 2D point in a circle with uniform distribution, NOTE: circle boundary is excluded
template< typename T >
INLINE Vector2 RandCirclePoint(T& rand, const Vector2& center, float radius);

// convert 3 uniform random variables to a random 3D point inside a box
template< typename T >
INLINE Vector3 RandBoxPoint(T& rand, const Vector3& min, const Vector3& max);

// convert 3 uniform random variables to a random 3D point inside a box
template< typename T >
INLINE Vector3 RandBoxPoint(T& rand, const Box& box);

// convert 3 uniform random variables to a random 3D point inside a box
template< typename T >
INLINE Vector3 RandBoxPoint(T& rand, const OBB& box);

// convert 3 uniform random variables to a random 3D point inside an sphere
template< typename T >
INLINE Vector3 RandSpherePoint(T& rand);

// convert 3 uniform random variables to a random 3D point inside a sphere
template< typename T >
INLINE Vector3 RandSpherePoint(T& rand, const Vector3& center, float radius);

// convert 3 uniform random variables to a random 3D point inside a sphere
template< typename T >
INLINE Vector3 RandSpherePoint(T& rand, const Sphere& sphere);

// convert 2 (TWO) uniform random variables to a random 3D point ON A SURFACE of the unit sphere
template< typename T >
INLINE Vector3 RandSphereSurfacePoint(T& rand);

// convert 2 (TWO) uniform random variables to a random 3D point ON A SURFACE of the sphere
template< typename T >
INLINE Vector3 RandSphereSurfacePoint(T& rand, const Vector3& center, float radius);

// convert 3 uniform random variables to a random 3D point inside a sphere
template< typename T >
INLINE Vector3 RandSphereSurfacePoint(T& rand, const Sphere& sphere);

// convert 2 (TWO) uniform random variables to a random 3D point on a unit hemi-sphere at 0,0,0 and with N=0,0,1 (standard tangent-space hemisphere)
template< typename T >
INLINE Vector3 RandHemiSphereSurfacePoint(T& rand);

// convert 2 (TWO) uniform random variables to a random 3D point on a unit hemi-sphere at 0,0,0 with specified N
template< typename T >
INLINE Vector3 RandHemiSphereSurfacePoint(T& rand, const Vector3& N);

// convert 2 (TWO) uniform random variables to a random 3D point on a hemi-sphere facing given direction
template< typename T >
INLINE Vector3 RandHemiSphereSurfacePoint(T& rand, const Vector3& center, float radius, const Vector3& normal);

// convert 2 (TWO) uniform random variables to a random point inside a unit triangle (basically we return the barycentric coordinates)
template< typename T >
INLINE Vector3 RandTrianglePoint(T& rand);

// convert 2 (TWO) uniform random variables to a random point inside a triangle
template< typename T >
INLINE Vector3 RandTrianglePoint(T& rand, const Vector3& a, const Vector3& b, const Vector3& c);

//---

END_BOOMER_NAMESPACE()
