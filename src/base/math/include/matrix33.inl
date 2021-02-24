/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\matrix33 #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//--

INLINE Matrix33::Matrix33()
{
    identity();
}

INLINE Matrix33::Matrix33(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
{
    m[0][0] = m00;
    m[0][1] = m01;
    m[0][2] = m02;
    m[1][0] = m10;
    m[1][1] = m11;
    m[1][2] = m12;
    m[2][0] = m20;
    m[2][1] = m21;
    m[2][2] = m22;
}

INLINE Matrix33::Matrix33(const Vector3& x, const Vector3& y, const Vector3& z)
{
    m[0][0] = x.x;
    m[0][1] = x.y;
    m[0][2] = x.z;
    m[1][0] = y.x;
    m[1][1] = y.y;
    m[1][2] = y.z;
    m[2][0] = z.x;
    m[2][1] = z.y;
    m[2][2] = z.z;
}

INLINE void Matrix33::zero()
{
    memzero(&m[0][0], sizeof(m));
}

INLINE void Matrix33::identity()
{
    zero();
    m[0][0] = 1.0f;
    m[1][1] = 1.0f;
    m[2][2] = 1.0f;
}

INLINE bool Matrix33::operator==(const Matrix33& other) const
{
    return 0 == memcmp(&m, &other.m, sizeof(m));
}

INLINE bool Matrix33::operator!=(const Matrix33& other) const
{
    return 0 != memcmp(&m, &other.m, sizeof(m));
}

//--

END_BOOMER_NAMESPACE(base)