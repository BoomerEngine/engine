/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\transform #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

/// TRS (translation-rotation-scale) transform, usually used for entity system
/// NOTE: this is preferred over the matrix because of easy and readable component separation and accuracy (especially position)
/// BEWARE!: the transform concatenation is done in a way that always maps back to TRS space (so TRS * TRS -> TRS
/// this means that not all possible actual 3D transformations can be encoded, especially we cannot encode anything with sheering :( :( BEWARE!
TYPE_ALIGN(16, class) CORE_MATH_API Transform
{
public:
    INLINE Transform();
    INLINE Transform(const ExactPosition& pos);
    INLINE Transform(const ExactPosition & pos, const Quat& rot);
    INLINE Transform(const ExactPosition & pos, const Quat& rot, const Vector3& scale);

    INLINE Transform(const Transform& other) = default;
    INLINE Transform(Transform&& other) = default;
    INLINE Transform& operator=(const Transform& other) = default;
    INLINE Transform& operator=(Transform&& other) = default;
    INLINE ~Transform() = default;

    INLINE bool operator==(const Transform& other) const;
    INLINE bool operator!=(const Transform& other) const;

    //--

    Quat R; // 16
    ExactPosition T; // 24
    Vector3 S; // 12

    //---

    /// reset to identity
    INLINE void reset();

    //---

    /// is transform and identity
    bool checkIdentity() const;

    /// get an inversion of this transformation
    Transform inverted() const;

    /// get the simple TR only inverse, scale is not included
    Transform invertedWithNoScale() const;

    ///--

    /// get transform created by applying this transform to the base transform
    /// NOTE: some combinations of scaling and rotation are not exactly representable back in the TRS, beware!
    Transform operator*(const Transform& baseTransform) const;

    /// express this transform in space of given other transform
    Transform operator/(const Transform& baseTransform) const;

    //---

    /// get identity transform
    static const Transform& IDENTITY();

    //---

    /// get a matrix representation of the transform
    Matrix toMatrix() const;

    /// get a matrix representation of the transform without the scaling
    Matrix toMatrixNoScale() const;

    /// get a matrix representation of the inverse transform
    Matrix toInverseMatrix() const;

    /// get a matrix representation of the inverse transform without the scaling
    Matrix toInverseMatrixNoScale() const;

    /// convert to Euler transform (NOTE: rotation conversion may have singularities)
    EulerTransform toEulerTransform() const;

    //---

    // Custom type implementation requirements
    void writeBinary(SerializationWriter& stream) const;
    void readBinary(SerializationReader& stream);

    //---
};

END_BOOMER_NAMESPACE()
