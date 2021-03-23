/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\convex #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

/// general transformation "engine"
class CORE_MATH_API BaseTransformation : public NoCopy
{
public:
    INLINE BaseTransformation(const Matrix& matrix_);

    Vector3 transformPoint(const Vector3& point) const;

    Vector4 transformPointWithW(const Vector3& point) const;

    Vector3 transformInvPoint(const Vector3& point) const;

    Vector3 transformVector(const Vector3& point) const;

    Vector3 transformInvVector(const Vector3& point) const;

    ExactPosition transformPosition(const ExactPosition& point) const;

    ExactPosition transformInvPosition(const ExactPosition& point) const;

    Vector4 transformVector4(const Vector4& point) const;

    Plane transformPlane(const Plane& plane) const;

    Box transformBox(const Box& box) const;

private:
    const Matrix& matrix;
};

//---

/// general transformation "engine"
class CORE_MATH_API Transformation : public BaseTransformation
{
public:
    Transformation(); // identity

    Transformation(const Matrix& m);
    Transformation(const Matrix33& m);
    Transformation(const Matrix33& m, const Vector3& pos);

    Transformation(const Vector3& pos);
    Transformation(const Vector3& pos, float scale);
    Transformation(const Vector3& pos, const Vector3& scale);

    Transformation(const Vector3& pos, const Angles& rot);
    Transformation(const Vector3& pos, const Angles& rot, float scale);
    Transformation(const Vector3& pos, const Angles& rot, const Vector3& scale);
    Transformation(const Angles& rot);
    Transformation(const Angles& rot, float scale);
    Transformation(const Angles& rot, const Vector3& scale);

    Transformation(const Vector3& pos, const Quat& rot);
    Transformation(const Vector3& pos, const Quat& rot, float scale);
    Transformation(const Vector3& pos, const Quat& rot, const Vector3& scale);
    Transformation(const Quat& rot);
    Transformation(const Quat& rot, float scale);
    Transformation(const Quat& rot, const Vector3& scale);

    Transformation(const EulerTransform& t);
    Transformation(const Transform& t);

    //--

private:
    Matrix localMatrix;
};

//---

END_BOOMER_NAMESPACE()
