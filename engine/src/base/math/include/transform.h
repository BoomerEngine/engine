/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\transform #]
***/

#pragma once

namespace base
{

    /// TRS (translation-rotation-scale) transform of a node in the node chain
    /// NOTE: this is preferred over the matrix because of easy and readable component separation
    /// NOTE: the transform concatenation is done in a way that always maps back to TRS space (so TRS * TRS -> TRS
    /// this means that not all possible actual 3D transformations can be encoded, especially we cannot encode anything with sheering
    /// NOTE: the translation is NOT a POSITION - the position may require much higher precision
    TYPE_ALIGN(16, class) BASE_MATH_API Transform
    {
    public:
        /// base types for the transform class
        typedef Vector3 Translation;
        typedef Vector3 Scale;
        typedef Quat Rotation;

        INLINE Transform();
        INLINE Transform(const Translation& pos);
        INLINE Transform(const Translation& pos, const Rotation& rot);
        INLINE Transform(const Translation& pos, const Rotation& rot, const Scale& scale);

        INLINE Transform(const Transform& other) = default;
        INLINE Transform(Transform&& other) = default;
        INLINE Transform& operator=(const Transform& other) = default;
        INLINE Transform& operator=(Transform&& other) = default;
        INLINE ~Transform() = default;

        INLINE bool operator==(const Transform& other) const;
        INLINE bool operator!=(const Transform& other) const;

        //--

        Rotation R;
        Translation T;
        Scale S;

        //---

        /// reset to identity
        INLINE Transform& identity();

        /// get transform created by shifting this transform in parent space
        INLINE Transform& shiftParent(const Translation& translationInParentSpace);

        /// get transform created by shifting this transform in local space (added shift is transformed by rotation and scale first)
        INLINE Transform& shiftLocal(const Translation& translationInLocalSpace);

        /// get transform created by applying a rotation in parent space
        INLINE Transform& rotateParent(float pitch, float yaw, float roll);

        /// get transform created by applying a rotation in parent space
        INLINE Transform& rotateParent(const base::Quat& quat);

        /// get transform created by applying a rotation in parent space
        INLINE Transform& rotateLocal(float pitch, float yaw, float roll);

        /// get transform created by applying a rotation in parent space
        INLINE Transform& rotateLocal(const base::Quat& quat);

        //---

        /// is transform and identity
        bool isIdentity() const;

        /// get an inversion of this transformation
        Transform inverted() const;

        /// get the simple TR only inverse, scale is not included
        Transform invertedWithNoScale() const;

        /// get transform created by applying this transform to the base transform
        /// NOTE: some combinations of scaling and rotation are not exactly representable back in the TRS, beware
        Transform applyTo(const Transform& baseTransform) const;

        /// get transform in space of given node
        Transform relativeTo(const Transform& baseTransform) const;

        //---

        /// transform vector by the transform
        /// NOTE: vectors are not affected by translations
        Vector3 transformVector(const Vector3& point) const;

        /// transform position by the transform
        Vector3 transformPoint(const Vector3& point) const;

        /// transform vector by the transform
        /// NOTE: vectors are not affected by translations
        Vector3 invTransformVector(const Vector3& point) const;

        /// transform position by the transform
        Vector3 invTransformPoint(const Vector3& point) const;

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
        void writeBinary(base::stream::OpcodeWriter& stream) const;
        void readBinary(base::stream::OpcodeReader& stream);

        //---        
    };

} // base
