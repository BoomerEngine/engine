/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\transform #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//--

Vector3 BaseTransformation::transformPoint(const Vector3& point) const
{
    float x = point.x * matrix.m[0][0] + point.y * matrix.m[0][1] + point.z * matrix.m[0][2] + matrix.m[0][3];
    float y = point.x * matrix.m[1][0] + point.y * matrix.m[1][1] + point.z * matrix.m[1][2] + matrix.m[1][3];
    float z = point.x * matrix.m[2][0] + point.y * matrix.m[2][1] + point.z * matrix.m[2][2] + matrix.m[2][3];
    return Vector3(x, y, z);

}

Vector4 BaseTransformation::transformPointWithW(const Vector3& point) const
{
    float x = point.x * matrix.m[0][0] + point.y * matrix.m[0][1] + point.z * matrix.m[0][2] + matrix.m[0][3];
    float y = point.x * matrix.m[1][0] + point.y * matrix.m[1][1] + point.z * matrix.m[1][2] + matrix.m[1][3];
    float z = point.x * matrix.m[2][0] + point.y * matrix.m[2][1] + point.z * matrix.m[2][2] + matrix.m[2][3];
    float w = point.x * matrix.m[3][0] + point.y * matrix.m[3][1] + point.z * matrix.m[3][2] + matrix.m[3][3];
    return Vector4(x, y, z, w);

}

Vector3 BaseTransformation::transformInvPoint(const Vector3& point) const
{
    float px = point.x - matrix.m[0][3];
    float py = point.y - matrix.m[1][3];
    float pz = point.z - matrix.m[2][3];
    float x = px * matrix.m[0][0] + py * matrix.m[1][0] + pz * matrix.m[2][0];
    float y = px * matrix.m[0][1] + py * matrix.m[1][1] + pz * matrix.m[2][1];
    float z = px * matrix.m[0][2] + py * matrix.m[1][2] + pz * matrix.m[2][2];
    return Vector3(x, y, z);
}

Vector3 BaseTransformation::transformVector(const Vector3& point) const
{
    float x = point.x * matrix.m[0][0] + point.y * matrix.m[0][1] + point.z * matrix.m[0][2];
    float y = point.x * matrix.m[1][0] + point.y * matrix.m[1][1] + point.z * matrix.m[1][2];
    float z = point.x * matrix.m[2][0] + point.y * matrix.m[2][1] + point.z * matrix.m[2][2];
    return Vector3(x, y, z);
}

Vector3 BaseTransformation::transformInvVector(const Vector3& point) const
{
    float x = point.x * matrix.m[0][0] + point.y * matrix.m[1][0] + point.z * matrix.m[2][0];
    float y = point.x * matrix.m[0][1] + point.y * matrix.m[1][1] + point.z * matrix.m[2][1];
    float z = point.x * matrix.m[0][2] + point.y * matrix.m[1][2] + point.z * matrix.m[2][2];
    return Vector3(x, y, z);
}

ExactPosition BaseTransformation::transformPosition(const ExactPosition& point) const
{
    double x = point.x * matrix.m[0][0] + point.y * matrix.m[0][1] + point.z * matrix.m[0][2] + matrix.m[0][3];
    double y = point.x * matrix.m[1][0] + point.y * matrix.m[1][1] + point.z * matrix.m[1][2] + matrix.m[1][3];
    double z = point.x * matrix.m[2][0] + point.y * matrix.m[2][1] + point.z * matrix.m[2][2] + matrix.m[2][3];
    return ExactPosition(x, y, z);
}

ExactPosition BaseTransformation::transformInvPosition(const ExactPosition& point) const
{
    double px = point.x - matrix.m[0][3];
    double py = point.y - matrix.m[1][3];
    double pz = point.z - matrix.m[2][3];
    double x = px * matrix.m[0][0] + py * matrix.m[1][0] + pz * matrix.m[2][0];
    double y = px * matrix.m[0][1] + py * matrix.m[1][1] + pz * matrix.m[2][1];
    double z = px * matrix.m[0][2] + py * matrix.m[1][2] + pz * matrix.m[2][2];
    return ExactPosition(x, y, z);
}

Vector4 BaseTransformation::transformVector4(const Vector4& point) const
{
    float x = point.x * matrix.m[0][0] + point.y * matrix.m[0][1] + point.z * matrix.m[0][2] + point.w * matrix.m[0][3];
    float y = point.x * matrix.m[1][0] + point.y * matrix.m[1][1] + point.z * matrix.m[1][2] + point.w * matrix.m[1][3];
    float z = point.x * matrix.m[2][0] + point.y * matrix.m[2][1] + point.z * matrix.m[2][2] + point.w * matrix.m[2][3];
    float w = point.x * matrix.m[3][0] + point.y * matrix.m[3][1] + point.z * matrix.m[3][2] + point.w * matrix.m[3][3];
    return Vector4(x, y, z, w);
}


Plane BaseTransformation::transformPlane(const Plane& plane) const
{
    auto n = transformVector(plane.n);
    auto p = transformPoint(plane.n * -plane.d);
    n.normalize();
    return Plane(n, -(n| p));
}

Box BaseTransformation::transformBox(const Box& box) const
{
    Box out;

    if (!box.empty())
    {
        out.min = out.max = transformPoint(box.corner(0));
        // Transform ret of the corners
        out.merge(transformPoint(box.corner(1)));
        out.merge(transformPoint(box.corner(2)));
        out.merge(transformPoint(box.corner(3)));
        out.merge(transformPoint(box.corner(4)));
        out.merge(transformPoint(box.corner(5)));
        out.merge(transformPoint(box.corner(6)));
        out.merge(transformPoint(box.corner(7)));
    }

    return out;
}

//--

Transformation::Transformation()
    : BaseTransformation(localMatrix)
{}

Transformation::Transformation(const Matrix& m)
    : BaseTransformation(m)
{}

Transformation::Transformation(const Matrix33& m)
    : BaseTransformation(localMatrix)
    , localMatrix(m.toMatrix())
{
}

Transformation::Transformation(const Matrix33& m, const Vector3& pos)
    : BaseTransformation(localMatrix)
    , localMatrix(m.toMatrix())
{}

Transformation::Transformation(const Vector3& pos)
    : BaseTransformation(localMatrix)
{
    localMatrix.identity();
    localMatrix.translation(pos);
}

Transformation::Transformation(const Vector3& pos, const Angles& rot)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
    localMatrix.translation(pos);
}

Transformation::Transformation(const Vector3& pos, const Angles& rot, float scale)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
    localMatrix.scaleColumns(Vector3(scale, scale, scale));
    localMatrix.translation(pos);
}

Transformation::Transformation(const Vector3& pos, const Angles& rot, const Vector3& scale)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
    localMatrix.scaleColumns(scale);
    localMatrix.translation(pos);
}

Transformation::Transformation(const Angles& rot)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
}

Transformation::Transformation(const Angles& rot, float scale)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
    localMatrix.scaleColumns(Vector3(scale, scale, scale));
}

Transformation::Transformation(const Angles& rot, const Vector3& scale)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
    localMatrix.scaleColumns(scale);
}

Transformation::Transformation(const Vector3& pos, float scale)
    : BaseTransformation(localMatrix)
{
    localMatrix = Matrix::BuildScale(scale);
    localMatrix.translation(pos);
}

Transformation::Transformation(const Vector3& pos, const Vector3& scale)
    : BaseTransformation(localMatrix)
{
    localMatrix = Matrix::BuildScale(scale);
    localMatrix.translation(pos);
}

Transformation::Transformation(const Vector3& pos, const Quat& rot)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
    localMatrix.translation(pos);
}

Transformation::Transformation(const Vector3& pos, const Quat& rot, float scale)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
    localMatrix.scaleColumns(Vector3(scale, scale, scale));
    localMatrix.translation(pos);
}

Transformation::Transformation(const Vector3& pos, const Quat& rot, const Vector3& scale)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
    localMatrix.scaleColumns(scale);
    localMatrix.translation(pos);
}

Transformation::Transformation(const Quat& rot)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
}

Transformation::Transformation(const Quat& rot, float scale)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
    localMatrix.scaleColumns(Vector3(scale, scale, scale));
}

Transformation::Transformation(const Quat& rot, const Vector3& scale)
    : BaseTransformation(localMatrix)
{
    localMatrix = rot.toMatrix();
    localMatrix.scaleColumns(scale);
}

Transformation::Transformation(const EulerTransform& t)
    : BaseTransformation(localMatrix)
    , localMatrix(t.toMatrix())
{}

Transformation::Transformation(const Transform& t)
    : BaseTransformation(localMatrix)
    , localMatrix(t.toMatrix())
{}

//--

END_BOOMER_NAMESPACE()
