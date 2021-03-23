/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\matrix #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_STRUCT(Matrix);
    RTTI_BIND_NATIVE_COMPARE(Matrix);
    RTTI_TYPE_TRAIT().noDestructor().fastCopyCompare(); // we must be constructed to identity

    RTTI_PROPERTY_VIRTUAL("x", Vector4, 0).editable();
    RTTI_PROPERTY_VIRTUAL("y", Vector4, 16).editable();
    RTTI_PROPERTY_VIRTUAL("z", Vector4, 32).editable();
    RTTI_PROPERTY_VIRTUAL("w", Vector4, 48).editable();
RTTI_END_TYPE();

//--

Vector3 Matrix::rowLengths() const
{
    return Vector3(
            std::hypot(m[0][0], m[0][1], m[0][2]),
            std::hypot(m[1][0], m[1][1], m[1][2]),
            std::hypot(m[2][0], m[2][1], m[2][2])
            );
}

Vector3 Matrix::columnLengths() const
{
    return Vector3(
            std::hypot(m[0][0], m[1][0], m[2][0]),
            std::hypot(m[0][1], m[1][1], m[2][1]),
            std::hypot(m[0][2], m[1][2], m[2][2])
    );
}

Matrix& Matrix::scaleColumns(const Vector3 &scale)
{
    m[0][0] *= scale.x;
    m[1][0] *= scale.x;
    m[2][0] *= scale.x;
    m[0][1] *= scale.y;
    m[1][1] *= scale.y;
    m[2][1] *= scale.y;
    m[0][2] *= scale.z;
    m[1][2] *= scale.z;
    m[2][2] *= scale.z;
    return *this;
}

Matrix& Matrix::scaleRows(const Vector3 &scale)
{
    m[0][0] *= scale.x;
    m[0][1] *= scale.x;
    m[0][2] *= scale.x;
    m[1][0] *= scale.y;
    m[1][1] *= scale.y;
    m[1][2] *= scale.y;
    m[2][0] *= scale.z;
    m[2][1] *= scale.z;
    m[2][2] *= scale.z;
    return *this;
}

Matrix& Matrix::scaleInner(float scale)
{
    m[0][0] *= scale;
    m[0][1] *= scale;
    m[0][2] *= scale;
    m[1][0] *= scale;
    m[1][1] *= scale;
    m[1][2] *= scale;
    m[2][0] *= scale;
    m[2][1] *= scale;
    m[2][2] *= scale;
    return *this;
}

Matrix& Matrix::normalizeRows()
{
    Vector3 scale = rowLengths();
    m[0][0] /= scale.x;
    m[0][1] /= scale.x;
    m[0][2] /= scale.x;
    m[1][0] /= scale.y;
    m[1][1] /= scale.y;
    m[1][2] /= scale.y;
    m[2][0] /= scale.z;
    m[2][1] /= scale.z;
    m[2][2] /= scale.z;
    return *this;
}

Matrix& Matrix::normalizeColumns()
{
    Vector3 scale = columnLengths();
    m[0][0] /= scale.x;
    m[1][0] /= scale.x;
    m[2][0] /= scale.x;
    m[0][1] /= scale.y;
    m[1][1] /= scale.y;
    m[2][1] /= scale.y;
    m[0][2] /= scale.z;
    m[1][2] /= scale.z;
    m[2][2] /= scale.z;
    return *this;
}

Matrix& Matrix::scaleTranslation(const Vector3 &scale)
{
    m[0][3] *= scale.x;
    m[1][3] *= scale.y;
    m[2][3] *= scale.z;
    return *this;
}

Matrix& Matrix::scaleTranslation(float scale)
{
    m[0][3] *= scale;
    m[1][3] *= scale;
    m[2][3] *= scale;
    return *this;
}

double Matrix::coFactor(int row, int col) const
{
#define M(i,j) (double)(m[(row+i)&3][(col+j)&3])
    double val = 0.0;
    val += M(1, 1) * M(2, 2) * M(3, 3);
    val += M(1, 2) * M(2, 3) * M(3, 1);
    val += M(1, 3) * M(2, 1) * M(3, 2);
    val -= M(3, 1) * M(2, 2) * M(1, 3);
    val -= M(3, 2) * M(2, 3) * M(1, 1);
    val -= M(3, 3) * M(2, 1) * M(1, 2);
    val *= ((row + col) & 1) ? -1.0f : 1.0f;
    return val;
#undef M
}

double Matrix::det() const
{
    return  m[0][0] * coFactor(0, 0) +
            m[0][1] * coFactor(0, 1) +
            m[0][2] * coFactor(0, 2) +
            m[0][3] * coFactor(0, 3);
}

double Matrix::det3() const
{
    double det = 0.0;
    det += m[0][0] * m[1][1] * m[2][2]; // 123
    det += m[0][1] * m[1][2] * m[2][0]; // 231
    det += m[0][2] * m[1][0] * m[2][1]; // 312
    det -= m[0][2] * m[1][1] * m[2][0]; // 321
    det -= m[0][1] * m[1][0] * m[2][2]; // 213
    det -= m[0][0] * m[1][2] * m[2][1]; // 132
    return det;
}

void Matrix::transpose()
{
    using std::swap;
    swap(m[1][0], m[0][1]);
    swap(m[2][0], m[0][2]);
    swap(m[3][0], m[0][3]);
    swap(m[3][1], m[1][3]);
    swap(m[3][2], m[2][3]);
    swap(m[1][2], m[2][1]);
}

Matrix Matrix::transposed() const
{
    Matrix out;
    out.m[0][0] = m[0][0];
    out.m[0][1] = m[1][0];
    out.m[0][2] = m[2][0];
    out.m[0][3] = m[3][0];
    out.m[1][0] = m[0][1];
    out.m[1][1] = m[1][1];
    out.m[1][2] = m[2][1];
    out.m[1][3] = m[3][1];
    out.m[2][0] = m[0][2];
    out.m[2][1] = m[1][2];
    out.m[2][2] = m[2][2];
    out.m[2][3] = m[3][2];
    out.m[3][0] = m[0][3];
    out.m[3][1] = m[1][3];
    out.m[3][2] = m[2][3];
    out.m[3][3] = m[3][3];
    return out;
}

void Matrix::invert()
{
    *this = inverted();
}

Matrix Matrix::inverted() const
{
    auto d = det();

    Matrix out;
    out.m[0][0] = (float)(coFactor(0, 0) / d);
    out.m[0][1] = (float)(coFactor(1, 0) / d);
    out.m[0][2] = (float)(coFactor(2, 0) / d);
    out.m[0][3] = (float)(coFactor(3, 0) / d);
    out.m[1][0] = (float)(coFactor(0, 1) / d);
    out.m[1][1] = (float)(coFactor(1, 1) / d);
    out.m[1][2] = (float)(coFactor(2, 1) / d);
    out.m[1][3] = (float)(coFactor(3, 1) / d);
    out.m[2][0] = (float)(coFactor(0, 2) / d);
    out.m[2][1] = (float)(coFactor(1, 2) / d);
    out.m[2][2] = (float)(coFactor(2, 2) / d);
    out.m[2][3] = (float)(coFactor(3, 2) / d);
    out.m[3][0] = (float)(coFactor(0, 3) / d);
    out.m[3][1] = (float)(coFactor(1, 3) / d);
    out.m[3][2] = (float)(coFactor(2, 3) / d);
    out.m[3][3] = (float)(coFactor(3, 3) / d);
    return out;
};

//--

Angles Matrix::toRotator() const
{
    Angles ret;

    float xy = std::hypot(m[0][0], m[1][0]);
    if (xy > 0.01f)
    {
        ret.yaw = RAD2DEG * std::atan2(m[1][0], m[0][0]);
        ret.pitch =  RAD2DEG * std::atan2(-m[2][0], xy);
        ret.roll =  RAD2DEG * std::atan2(m[2][1], m[2][2]);
    }
    else
    {
        ret.roll =  RAD2DEG * std::atan2(-m[1][2], m[1][1]);
        ret.pitch =  RAD2DEG * std::atan2(-m[2][0], xy);
        ret.yaw = 0.0f;
    }

    return ret;
}

Quat Matrix::toQuat() const
{
    Quat q;

    float trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0)
    {
        float s = 0.5f / std::sqrt(trace + 1.0f);
        q.w = 0.25f / s;
        q.x = (m[2][1] - m[1][2]) * s;
        q.y = (m[0][2] - m[2][0]) * s;
        q.z = (m[1][0] - m[0][1]) * s;
    }
    else
    {
        if (m[0][0] > m[1][1] && m[0][0] > m[2][2])
        {
            float s = 2.0f * std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]);
            q.w = (m[2][1] - m[1][2]) / s;
            q.x = 0.25f * s;
            q.y = (m[0][1] + m[1][0]) / s;
            q.z = (m[0][2] + m[2][0]) / s;
        }
        else if (m[1][1] > m[2][2])
        {
            float s = 2.0f * std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]);
            q.w = (m[0][2] - m[2][0]) / s;
            q.x = (m[0][1] + m[1][0]) / s;
            q.y = 0.25f * s;
            q.z = (m[1][2] + m[2][1]) / s;
        }
        else
        {
            float s = 2.0f * std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]);
            q.w = (m[1][0] - m[0][1]) / s;
            q.x = (m[0][2] + m[2][0]) / s;
            q.y = (m[1][2] + m[2][1]) / s;
            q.z = 0.25f * s;
        }
    }

    q.normalize();
    return q;
}

//--

Matrix Matrix::BuildScale(const Vector3 &scale)
{
    return Matrix(scale.x, 0.0f, 0.0f, 0.0f,
                    0.0f, scale.y, 0.0f, 0.0f,
                    0.0f, 0.0f, scale.z, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0);
}

Matrix Matrix::BuildScale(float scale)
{
    return Matrix(scale, 0.0f, 0.0f, 0.0f,
    0.0f, scale, 0.0f, 0.0f,
    0.0f, 0.0f, scale, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0);
}

Matrix Matrix::BuildTranslation(const Vector3 &trans)
{
    return Matrix(1.0f, 0.0f, 0.0f, trans.x,
                    0.0f, 1.0f, 0.0f, trans.y,
                    0.0f, 0.0f, 1.0f, trans.z,
                    0.0f, 0.0f, 0.0f, 1.0);
}

Matrix Matrix::BuildRotation(const Angles &rotation)
{
    return rotation.toMatrix();
}

Matrix Matrix::BuildRotationX(float degress)
{
    auto s = std::sin(DEG2RAD * degress);
    auto c = std::cos(DEG2RAD * degress);
    return Matrix(1,    0.0f, 0.0f, 0.0f,
                    0.0f, c,    s,    0.0f,
                    0.0f, -s,   c, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0);
}

Matrix Matrix::BuildRotationY(float degress)
{
    auto s = std::sin(DEG2RAD * degress);
    auto c = std::cos(DEG2RAD * degress);
    return Matrix(c,    0.0f, -s,   0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    s,    0.0f, c,    0.0f,
                    0.0f, 0.0f, 0.0f, 1.0);
}

Matrix Matrix::BuildRotationZ(float degress)
{
    auto s = std::sin(DEG2RAD * degress);
    auto c = std::cos(DEG2RAD * degress);
    return Matrix(c,    s,    0.0f, 0.0f,
                    -s,   c,    0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0);
}

Matrix Matrix::BuildRotation(float pitch, float yaw, float roll)
{
    return Angles(pitch, yaw, roll).toMatrix();
}

Matrix Matrix::BuildRotation(const Quat& rotation)
{
    return rotation.toMatrix();
}

Matrix Matrix::BuildReflection(const Plane &plane)
{
    Plane copy = plane;
    copy.normalize();

    Matrix ret;
    ret.m[0][0] = -2.0f * plane.n.x * plane.n.x + 1;
    ret.m[0][1] = -2.0f * plane.n.x * plane.n.y;
    ret.m[0][2] = -2.0f * plane.n.x * plane.n.z;
    ret.m[0][3] = -2.0f * plane.n.x * plane.d;
    ret.m[1][0] = -2.0f * plane.n.y * plane.n.x;
    ret.m[1][1] = -2.0f * plane.n.y * plane.n.y + 1;
    ret.m[1][2] = -2.0f * plane.n.y * plane.n.z;
    ret.m[1][3] = -2.0f * plane.n.y * plane.d;
    ret.m[2][0] = -2.0f * plane.n.z * plane.n.x;
    ret.m[2][1] = -2.0f * plane.n.z * plane.n.y;
    ret.m[2][2] = -2.0f * plane.n.z * plane.n.z + 1;
    ret.m[2][3] = -2.0f * plane.n.z * plane.d;
    ret.m[3][0] = 0.0f;
    ret.m[3][1] = 0.0f;
    ret.m[3][2] = 0.0f;
    ret.m[3][3] = 1.0f;
    return ret;
}

Matrix Matrix::BuildTRS(const Vector3& t, const Angles& r, const Vector3& s /*= Vector3::ONE()*/)
{
    auto ret = r.toMatrix();
    ret.scaleColumns(s);
    ret.translation(t);
    return ret;
}

Matrix Matrix::BuildTRS(const Vector3& t, const Quat& r, const Vector3& s /*= Vector3::ONE()*/)
{
    auto ret = r.toMatrix();
    ret.scaleColumns(s);
    ret.translation(t);
    return ret;
}

Matrix Matrix::BuildTRS(const Vector3& t, const Angles& r, float s)
{
    auto ret = r.toMatrix();
    ret.scaleColumns(Vector3(s,s,s));
    ret.translation(t);
    return ret;
}

Matrix Matrix::BuildTRS(const Vector3& t, const Quat& r, float s)
{
    auto ret = r.toMatrix();
    ret.scaleColumns(Vector3(s,s,s));
    ret.translation(t);
    return ret;
}

Matrix Matrix::BuildTRS(const Vector3& t, const Matrix33& rs)
{
    return Matrix(rs, t);
}

Matrix Matrix::BuildPerspective(float xScale, float yScale, float zNear, float zFar, float offsetX /*= 0.0f*/, float offsetY /*= 0.0f*/)
{
    auto a = zFar / (zFar - zNear);
    auto b = -zNear*zFar / (zFar - zNear);

    Matrix ret;
    ret.m[0][0] = xScale;
    ret.m[0][1] = 0.0f;
    ret.m[0][2] = offsetX; // note: get's divided by W to get the proper offset
    ret.m[0][3] = 0.0f;

    ret.m[1][0] = 0.0f;
    ret.m[1][1] = yScale;
    ret.m[1][2] = offsetY; // note: get's divided by W to get the proper offset
    ret.m[1][3] = 0.0f;

    ret.m[2][0] = 0.0f;
    ret.m[2][1] = 0.0f;
    ret.m[2][2] = a;
    ret.m[2][3] = b;

    ret.m[3][0] = 0.0f;
    ret.m[3][1] = 0.0f;
    ret.m[3][2] = 1.0f;
    ret.m[3][3] = 0.0f;
    return ret;
}

Matrix Matrix::BuildPerspectiveFOV(float fovDeg, float aspectWidthToHeight, float zNear, float zFar, float offsetX /*= 0.0f*/, float offsetY /*= 0.0f*/)
{
    float xScale = 1.0f / std::tan(DEG2RAD * (fovDeg*0.5f));
    float yScale = xScale * aspectWidthToHeight;
    return BuildPerspective(xScale, yScale, zNear, zFar, offsetX, offsetY);
}

Matrix Matrix::BuildOrtho(float width, float height, float zNear, float zFar)
{
    auto a = 1.0f / (zFar - zNear);
    auto b = zNear / (zNear - zFar);

    // at z = zNear
    // n / (f-n) + -n / (f-n) = n-n / (f-n) = 0

    // at z = zFar
    // f / (f-n) - n / (f-n) = f-n / f-n = 1

    Matrix ret;
    ret.m[0][0] = 2.0f / width;
    ret.m[0][1] = 0.0f;
    ret.m[0][2] = 0.0f;
    ret.m[0][3] = 0.0f;

    ret.m[1][0] = 0.0f;
    ret.m[1][1] = 2.0f / height;
    ret.m[1][2] = 0.0f;
    ret.m[1][3] = 0.0f;

    ret.m[2][0] = 0.0f;
    ret.m[2][1] = 0.0f;
    ret.m[2][2] = a;
    ret.m[2][3] = b;

    ret.m[3][0] = 0.0f;
    ret.m[3][1] = 0.0f;
    ret.m[3][2] = 0.0f;
    ret.m[3][3] = 1.0f;
    return ret;
}

Matrix Matrix::Concat(const Matrix &a, const Matrix &b)
{
    Matrix ret;
    ret.m[0][0] = b.m[0][0] * a.m[0][0] + b.m[0][1] * a.m[1][0] + b.m[0][2] * a.m[2][0] + b.m[0][3] * a.m[3][0];
    ret.m[0][1] = b.m[0][0] * a.m[0][1] + b.m[0][1] * a.m[1][1] + b.m[0][2] * a.m[2][1] + b.m[0][3] * a.m[3][1];
    ret.m[0][2] = b.m[0][0] * a.m[0][2] + b.m[0][1] * a.m[1][2] + b.m[0][2] * a.m[2][2] + b.m[0][3] * a.m[3][2];
    ret.m[0][3] = b.m[0][0] * a.m[0][3] + b.m[0][1] * a.m[1][3] + b.m[0][2] * a.m[2][3] + b.m[0][3] * a.m[3][3];
    ret.m[1][0] = b.m[1][0] * a.m[0][0] + b.m[1][1] * a.m[1][0] + b.m[1][2] * a.m[2][0] + b.m[1][3] * a.m[3][0];
    ret.m[1][1] = b.m[1][0] * a.m[0][1] + b.m[1][1] * a.m[1][1] + b.m[1][2] * a.m[2][1] + b.m[1][3] * a.m[3][1];
    ret.m[1][2] = b.m[1][0] * a.m[0][2] + b.m[1][1] * a.m[1][2] + b.m[1][2] * a.m[2][2] + b.m[1][3] * a.m[3][2];
    ret.m[1][3] = b.m[1][0] * a.m[0][3] + b.m[1][1] * a.m[1][3] + b.m[1][2] * a.m[2][3] + b.m[1][3] * a.m[3][3];
    ret.m[2][0] = b.m[2][0] * a.m[0][0] + b.m[2][1] * a.m[1][0] + b.m[2][2] * a.m[2][0] + b.m[2][3] * a.m[3][0];
    ret.m[2][1] = b.m[2][0] * a.m[0][1] + b.m[2][1] * a.m[1][1] + b.m[2][2] * a.m[2][1] + b.m[2][3] * a.m[3][1];
    ret.m[2][2] = b.m[2][0] * a.m[0][2] + b.m[2][1] * a.m[1][2] + b.m[2][2] * a.m[2][2] + b.m[2][3] * a.m[3][2];
    ret.m[2][3] = b.m[2][0] * a.m[0][3] + b.m[2][1] * a.m[1][3] + b.m[2][2] * a.m[2][3] + b.m[2][3] * a.m[3][3];
    ret.m[3][0] = b.m[3][0] * a.m[0][0] + b.m[3][1] * a.m[1][0] + b.m[3][2] * a.m[2][0] + b.m[3][3] * a.m[3][0];
    ret.m[3][1] = b.m[3][0] * a.m[0][1] + b.m[3][1] * a.m[1][1] + b.m[3][2] * a.m[2][1] + b.m[3][3] * a.m[3][1];
    ret.m[3][2] = b.m[3][0] * a.m[0][2] + b.m[3][1] * a.m[1][2] + b.m[3][2] * a.m[2][2] + b.m[3][3] * a.m[3][2];
    ret.m[3][3] = b.m[3][0] * a.m[0][3] + b.m[3][1] * a.m[1][3] + b.m[3][2] * a.m[2][3] + b.m[3][3] * a.m[3][3];
    return ret;
}

//--

Transform Matrix::toTransform() const
{
    return Transform(translation(), toQuat(), columnLengths());
}

void Matrix::toFloats(float* outData) const
{
    memcpy(outData, this, sizeof(Matrix));
}

void Matrix::toDoubles(double* outData) const
{
    auto* endData = outData + 16;
    const auto* readData = (const float*)this;
    while (outData < endData)
        *outData++ = *readData++;
}

void Matrix::toFloatsTransposed(float* outData) const
{
    *outData++ = m[0][0];
    *outData++ = m[1][0];
    *outData++ = m[2][0];
    *outData++ = m[3][0];
    *outData++ = m[0][1];
    *outData++ = m[1][1];
    *outData++ = m[2][1];
    *outData++ = m[3][1];
    *outData++ = m[0][2];
    *outData++ = m[1][2];
    *outData++ = m[2][2];
    *outData++ = m[3][2];
    *outData++ = m[0][3];
    *outData++ = m[1][3];
    *outData++ = m[2][3];
    *outData++ = m[3][3];
}

void Matrix::toDoublesTransposed(float* outData) const
{
    *outData++ = m[0][0];
    *outData++ = m[1][0];
    *outData++ = m[2][0];
    *outData++ = m[3][0];
    *outData++ = m[0][1];
    *outData++ = m[1][1];
    *outData++ = m[2][1];
    *outData++ = m[3][1];
    *outData++ = m[0][2];
    *outData++ = m[1][2];
    *outData++ = m[2][2];
    *outData++ = m[3][2];
    *outData++ = m[0][3];
    *outData++ = m[1][3];
    *outData++ = m[2][3];
    *outData++ = m[3][3];
}

//--

static Matrix IDENTITY_M(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
static Matrix ZERO_M(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0);

const Matrix& Matrix::IDENTITY()
{
    return IDENTITY_M;
}

const Matrix& Matrix::ZERO()
{
    return ZERO_M;
}

//--

void Matrix::print(IFormatStream& f) const
{
    f.appendf("X:[{},{},{}]  ", m[0][0], m[0][1], m[0][2]);
    f.appendf("Y:[{},{},{}]  ", m[1][0], m[1][1], m[1][2]);
    f.appendf("Z:[{},{},{}]  ", m[2][0], m[2][1], m[2][2]);
    f.appendf("T:[{},{},{}]  ", m[0][3], m[1][3], m[2][3]);

    f.appendf("SH:[{},{},{}]  ", row(0).xyz().length(), row(1).xyz().length(), row(2).xyz().length());
    f.appendf("SV:[{},{},{}]  ", column(0).xyz().length(), column(1).xyz().length(), column(2).xyz().length());

    const auto r = this->toRotator();
    f.appendf("R:[{},{},{}]  ", r.pitch, r.yaw, r.roll);

    f.appendf("Det:{}  Det33: {}", det(), det3());
}

//--

END_BOOMER_NAMESPACE()
