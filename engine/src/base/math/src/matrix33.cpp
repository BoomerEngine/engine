/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\matrix #]
***/

#include "build.h"

namespace base
{

    //--

    RTTI_BEGIN_TYPE_STRUCT(Matrix33);
        RTTI_BIND_NATIVE_COMPARE(Matrix33);
        RTTI_TYPE_TRAIT().noDestructor().fastCopyCompare(); // we must be constructed to identity

        RTTI_PROPERTY_VIRTUAL("x", Vector3, 0).editable();
        RTTI_PROPERTY_VIRTUAL("y", Vector3, 12).editable();
        RTTI_PROPERTY_VIRTUAL("z", Vector3, 24).editable();
    RTTI_END_TYPE();

    //--

    Vector3 Matrix33::rowLengths() const
    {
        return Vector3(
                std::hypot(m[0][0], m[0][1], m[0][2]),
                std::hypot(m[1][0], m[1][1], m[1][2]),
                std::hypot(m[2][0], m[2][1], m[2][2])
        );
    }

    Vector3 Matrix33::columnLengths() const
    {
        return Vector3(
                std::hypot(m[0][0], m[1][0], m[2][0]),
                std::hypot(m[0][1], m[1][1], m[2][1]),
                std::hypot(m[0][2], m[1][2], m[2][2])
        );
    }

    void Matrix33::scaleColumns(const Vector3 &scale)
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
    }

    void Matrix33::scaleRows(const Vector3 &scale)
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
    }

    void Matrix33::scaleInner(float scale)
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
    }

    void Matrix33::normalizeRows()
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
    }

    void Matrix33::normalizeColumns()
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
    }

    double Matrix33::coFactor(int row, int col) const
    {
#define M(i,j) (double)(m[(row+i)%3][(col+j)%3])
        double val = 0.0;
        val += M(1, 1) * M(2, 2);
        val -= M(2, 1) * M(1, 2);
        val *= ((row + col) & 1) ? -1.0f : 1.0f;
        return val;
#undef M
    }

    double Matrix33::det() const
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

    void Matrix33::transpose()
    {
        using std::swap;
        swap(m[1][0], m[0][1]);
        swap(m[2][0], m[0][2]);
        swap(m[1][2], m[2][1]);
    }

    Matrix33 Matrix33::transposed() const
    {
        Matrix33 out;
        out.m[0][0] = m[0][0];
        out.m[0][1] = m[1][0];
        out.m[0][2] = m[2][0];
        out.m[1][0] = m[0][1];
        out.m[1][1] = m[1][1];
        out.m[1][2] = m[2][1];
        out.m[2][0] = m[0][2];
        out.m[2][1] = m[1][2];
        out.m[2][2] = m[2][2];
        return out;
    }

    void Matrix33::invert()
    {
        *this = inverted();
    }

    Matrix33 Matrix33::inverted() const
    {
        auto d = det();

        Matrix33 out;
        out.m[0][0] = (float)(coFactor(0, 0) / d);
        out.m[0][1] = (float)(coFactor(1, 0) / d);
        out.m[0][2] = (float)(coFactor(2, 0) / d);
        out.m[1][0] = (float)(coFactor(0, 1) / d);
        out.m[1][1] = (float)(coFactor(1, 1) / d);
        out.m[1][2] = (float)(coFactor(2, 1) / d);
        out.m[2][0] = (float)(coFactor(0, 2) / d);
        out.m[2][1] = (float)(coFactor(1, 2) / d);
        out.m[2][2] = (float)(coFactor(2, 2) / d);
        return out;
    };

    Vector3 Matrix33::column(int i) const
    {
        return Vector3(m[i][0], m[i][1], m[i][2]);
    }

    Vector3 Matrix33::row(int i) const
    {
        return Vector3(m[0][i], m[1][i], m[2][i]);
    }

    Vector3 Matrix33::transformVector(const Vector3& v) const
    {
        return Vector3(v.x*m[0][0] + v.y*m[0][1] + v.z*m[0][2],
                       v.x*m[1][0] + v.y*m[1][1] + v.z*m[1][2],
                       v.x*m[2][0] + v.y*m[2][1] + v.z*m[2][2]);
    }

    Vector3 Matrix33::transformInvVector(const Vector3& v) const
    {
        return Vector3(v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0],
        v.x*m[0][1] + v.y*m[1][1] + v.z*m[2][1],
        v.x*m[0][2] + v.y*m[1][2] + v.z*m[2][2]);
    }

    Matrix33 Matrix33::operator*(const Matrix33 &other) const
    {
        return Concat(*this, other);
    }

    Matrix33& Matrix33::operator*=(const Matrix33 &other)
    {
        *this = Concat(*this, other);
        return *this;
    }

    Matrix33 Matrix33::Concat(const Matrix33 &a, const Matrix33 &b)
    {
        Matrix33 ret;
        ret.m[0][0] = b.m[0][0] * a.m[0][0] + b.m[0][1] * a.m[1][0] + b.m[0][2] * a.m[2][0];
        ret.m[0][1] = b.m[0][0] * a.m[0][1] + b.m[0][1] * a.m[1][1] + b.m[0][2] * a.m[2][1];
        ret.m[0][2] = b.m[0][0] * a.m[0][2] + b.m[0][1] * a.m[1][2] + b.m[0][2] * a.m[2][2];
        ret.m[1][0] = b.m[1][0] * a.m[0][0] + b.m[1][1] * a.m[1][0] + b.m[1][2] * a.m[2][0];
        ret.m[1][1] = b.m[1][0] * a.m[0][1] + b.m[1][1] * a.m[1][1] + b.m[1][2] * a.m[2][1];
        ret.m[1][2] = b.m[1][0] * a.m[0][2] + b.m[1][1] * a.m[1][2] + b.m[1][2] * a.m[2][2];
        ret.m[2][0] = b.m[2][0] * a.m[0][0] + b.m[2][1] * a.m[1][0] + b.m[2][2] * a.m[2][0];
        ret.m[2][1] = b.m[2][0] * a.m[0][1] + b.m[2][1] * a.m[1][1] + b.m[2][2] * a.m[2][1];
        ret.m[2][2] = b.m[2][0] * a.m[0][2] + b.m[2][1] * a.m[1][2] + b.m[2][2] * a.m[2][2];
        return ret;
    }

    Matrix Matrix33::toMatrix() const
    {
        return Matrix(*this);
    }

    Vector3 Matrix33::trace() const
    {
        return Vector3(m[0][0], m[1][1], m[2][2]);
    }

    //-----------------------------------------------------------------------------

    Matrix33 Matrix33::BuildScale(const Vector3 &scale)
    {
        return Matrix33(scale.x, 0.0f, 0.0f, 0.0f, scale.y, 0.0f, 0.0f, 0.0f, scale.z);
    }

    Matrix33 Matrix33::BuildScale(float scale)
    {
        return Matrix33(scale, 0.0f, 0.0f, 0.0f, scale, 0.0f, 0.0f, 0.0f, scale);
    }

    Matrix33 Matrix33::BuildRotation(const Angles& rotation)
    {
        return rotation.toMatrix33();
    }

    Matrix33 Matrix33::BuildRotation(float pitch, float yaw, float roll)
    {
        return Angles(pitch, yaw, roll).toMatrix33();
    }

    Matrix33 Matrix33::BuildRotation(const Quat& quat)
    {
        return quat.toMatrix33();
    }

    Matrix33 Matrix33::BuildRotationScale(const Angles& rotation, const Vector3& scale)
    {
        auto ret = rotation.toMatrix33();
        ret.scaleColumns(scale);
        return ret;
    }

    Matrix33 Matrix33::BuildRotationScale(const Quat& quat, const Vector3& scale)
    {
        auto ret = quat.toMatrix33();
        ret.scaleColumns(scale);
        return ret;
    }

    Matrix33 Matrix33::BuildRotationScale(const Angles& rotation, float scale)
    {
        auto ret = rotation.toMatrix33();
        ret.scaleInner(scale);
        return ret;
    }

    Matrix33 Matrix33::BuildRotationScale(const Quat& quat, float scale)
    {
        auto ret = quat.toMatrix33();
        ret.scaleInner(scale);
        return ret;
    }

    //--

    static const  Matrix33 IDENTITY_M33(1,0,0, 0,1,0, 0,0,1);
    static const  Matrix33 ZERO_M33(0,0,0, 0,0,0, 0,0,0);

    const Matrix33& Matrix33::IDENTITY()
    {
        return IDENTITY_M33;
    }

    const Matrix33& Matrix33::ZERO()
    {
        return ZERO_M33;
    }

    //--

    Angles Matrix33::toRotator() const
    {
        Angles ret;

        float xy = std::hypot(m[0][0], m[1][0]);
        if (xy > 0.01f)
        {
            ret.yaw =  RAD2DEG * std::atan2(m[1][0], m[0][0]);
            ret.pitch =  RAD2DEG * std::atan2(-m[2][0], xy);
            ret.roll =  RAD2DEG * std::atan2(m[2][1], m[2][2]);
        }
        else
        {
            ret.yaw =  RAD2DEG * std::atan2(-m[0][1], m[1][1]);
            ret.pitch =  RAD2DEG * std::atan2(-m[2][0], xy);
            ret.roll = 0.0f;
        }

        return ret;
    }

    Quat Matrix33::toQuat() const
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

    //-----------------------------------------------------------------------------

    void Matrix33::diagonalise(Matrix33& outRotation, double limit /*= 0.01f*/, uint32_t maxIterations /*= 64*/)
    {
        double md[3][3];
        md[0][0] = m[0][0];
        md[0][1] = m[0][1];
        md[0][2] = m[0][2];
        md[1][0] = m[1][0];
        md[1][1] = m[1][1];
        md[1][2] = m[1][2];
        md[2][0] = m[2][0];
        md[2][1] = m[2][1];
        md[2][2] = m[2][2];

        double rot[3][3];
        memset(&rot, 0, sizeof(rot));
        rot[0][0] = 1.0;
        rot[1][1] = 1.0;
        rot[2][2] = 1.0;

        for (uint32_t step = maxIterations; step > 0; step--)
        {
            // find off-diagonal element [p][q] with largest magnitude
            int p = 0;
            int q = 1;
            int r = 2;
            double max = abs(md[0][1]);
            double v = abs(md[0][2]);
            if (v > max)
            {
                q = 2;
                r = 1;
                max = v;
            }
            v = abs(md[1][2]);
            if (v > max)
            {
                p = 1;
                q = 2;
                r = 0;
                max = v;
            }

            double t = limit * (abs(md[0][0]) + abs(md[1][1]) + abs(md[2][2]));
            if (max <= t)
            {
                if (max <= 0.0000000000001 * t)
                {
                    return;
                }
                step = 1;
            }

            // compute Jacobi rotation J which leads to a zero for element [p][q]
            double mpq = md[p][q];
            double theta = (md[q][q] - md[p][p]) / (2 * mpq);
            double theta2 = theta * theta;
            double cos;
            double sin;
            if (theta2 * theta2 < double(10 / 0.0000000000001))
            {
                t = (theta >= 0) ? 1 / (theta + sqrt(1 + theta2))
                                 : 1 / (theta - sqrt(1 + theta2));
                cos = 1 / sqrt(1 + t * t);
                sin = cos * t;
            }
            else
            {
                // approximation for large theta-value, i.e., a nearly diagonal matrix
                t = 1 / (theta * (2 + double(0.5) / theta2));
                cos = 1 - double(0.5) * t * t;
                sin = cos * t;
            }

            // apply rotation to matrix (this = J^T * this * J)
            md[p][q] = md[q][p] = 0;
            md[p][p] -= t * mpq;
            md[q][q] += t * mpq;
            double mrp = md[r][p];
            double mrq = md[r][q];
            md[r][p] = md[p][r] = cos * mrp - sin * mrq;
            md[r][q] = md[q][r] = cos * mrq + sin * mrp;

            // apply rotation to rot (rot = rot * J)
            for (int i = 0; i < 3; i++)
            {
                mrp = rot[i][p];
                mrq = rot[i][q];
                rot[i][p] = (float)(cos * mrp - sin * mrq);
                rot[i][q] = (float)(cos * mrq + sin * mrp);
            }
        }

        outRotation.m[0][0] = (float)rot[0][0];
        outRotation.m[0][1] = (float)rot[0][1];
        outRotation.m[0][2] = (float)rot[0][2];
        outRotation.m[1][0] = (float)rot[1][0];
        outRotation.m[1][1] = (float)rot[1][1];
        outRotation.m[1][2] = (float)rot[1][2];
        outRotation.m[2][0] = (float)rot[2][0];
        outRotation.m[2][1] = (float)rot[2][1];
        outRotation.m[2][2] = (float)rot[2][2];
    }

} // base