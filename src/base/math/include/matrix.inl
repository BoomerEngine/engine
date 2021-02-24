/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\matrix #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//-

INLINE Matrix::Matrix()
{
    identity();
}

INLINE Matrix::Matrix(const Matrix33 &other, const Vector3& t /*= Vector3::ZERO()*/)
{
    m[0][0] = other.m[0][0];
    m[0][1] = other.m[0][1];
    m[0][2] = other.m[0][2];
    m[0][3] = t.x;
    m[1][0] = other.m[1][0];
    m[1][1] = other.m[1][1];
    m[1][2] = other.m[1][2];
    m[1][3] = t.y;
    m[2][0] = other.m[2][0];
    m[2][1] = other.m[2][1];
    m[2][2] = other.m[2][2];
    m[2][3] = t.z;
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

INLINE Matrix::Matrix(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
{
    m[0][0] = m00;
    m[0][1] = m01;
    m[0][2] = m02;
    m[0][3] = 0.0f;
    m[1][0] = m10;
    m[1][1] = m11;
    m[1][2] = m12;
    m[1][3] = 0.0f;
    m[2][0] = m20;
    m[2][1] = m21;
    m[2][2] = m22;
    m[2][3] = 0.0f;
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

INLINE Matrix::Matrix(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33)
{
    m[0][0] = m00;
    m[0][1] = m01;
    m[0][2] = m02;
    m[0][3] = m03;
    m[1][0] = m10;
    m[1][1] = m11;
    m[1][2] = m12;
    m[1][3] = m13;
    m[2][0] = m20;
    m[2][1] = m21;
    m[2][2] = m22;
    m[2][3] = m23;
    m[3][0] = m30;
    m[3][1] = m31;
    m[3][2] = m32;
    m[3][3] = m33;
}

INLINE Matrix::Matrix(const Vector3& x, const Vector3& y, const Vector3& z)
{
    m[0][0] = x.x;
    m[0][1] = x.y;
    m[0][2] = x.z;
    m[0][3] = 0.0f;
    m[1][0] = y.x;
    m[1][1] = y.y;
    m[1][2] = y.z;
    m[1][3] = 0.0f;
    m[2][0] = z.x;
    m[2][1] = z.y;
    m[2][2] = z.z;
    m[2][3] = 0.0f;
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

INLINE Matrix::Matrix(const Vector4& x, const Vector4& y, const Vector4& z, const Vector4& w)
{
    m[0][0] = x.x;
    m[0][1] = x.y;
    m[0][2] = x.z;
    m[0][3] = x.w;
    m[1][0] = y.x;
    m[1][1] = y.y;
    m[1][2] = y.z;
    m[1][3] = y.w;
    m[2][0] = z.x;
    m[2][1] = z.y;
    m[2][2] = z.z;
    m[2][3] = z.w;
    m[3][0] = w.x;
    m[3][1] = w.y;
    m[3][2] = w.z;
    m[3][3] = w.w;
}

INLINE void Matrix::zero()
{
    memzero(&m[0][0], sizeof(m));
}

INLINE void Matrix::identity()
{
    zero();
    m[0][0] = 1.0f;
    m[1][1] = 1.0f;
    m[2][2] = 1.0f;
    m[3][3] = 1.0f;
}

INLINE Vector3 Matrix::translation() const
{
    return Vector3(m[0][3], m[1][3], m[2][3]);
}

INLINE void Matrix::translation(const Vector3 &trans)
{
    m[0][3] = trans.x;
    m[1][3] = trans.y;
    m[2][3] = trans.z;
}

INLINE void Matrix::translation(float x, float y, float z)
{
    m[0][3] = x;
    m[1][3] = y;
    m[2][3] = z;
}

INLINE Vector4 Matrix::column(int i) const
{
    return Vector4(m[0][i], m[1][i], m[2][i], m[3][i]);
}

INLINE Vector4& Matrix::row(int i)
{
    return ((Vector4*)this)[i];
}

INLINE const Vector4& Matrix::row(int i) const
{
    return ((const Vector4*)this)[i];
}

INLINE Matrix Matrix::operator~() const
{
    return inverted();
}

INLINE Matrix Matrix::operator*(const Matrix &other) const
{
    return Concat(*this, other);
}

INLINE Matrix& Matrix::operator*=(const Matrix &other)
{
    *this = Concat(*this, other);
    return *this;
}

INLINE bool Matrix::operator==(const Matrix& other) const
{
    return 0 == memcmp(&m, &other.m, sizeof(m));
}

INLINE bool Matrix::operator!=(const Matrix& other) const
{
    return 0 != memcmp(&m, &other.m, sizeof(m));
}

//---

END_BOOMER_NAMESPACE(base)