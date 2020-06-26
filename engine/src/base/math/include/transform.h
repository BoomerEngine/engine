/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\transform #]
***/

#pragma once

#include "base/reflection/include/reflectionMacros.h"
#include "vector3.h"
#include "quat.h"

namespace base
{

    /// TRS (translation-rotation-scale) transform of a node in the node chain
    /// NOTE: this is prefered over the matrix because of easy and readable component separation
    /// NOTE: the transform concatenation is done in a way that always maps back to TRS space (so TRS * TRS -> TRS
    /// this means that not all possible actual 3D transformations are encodable here, especially we cannot encode anything with sheering
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

        /// get raw translation element
        INLINE const Translation& translation() const { return m_trans; }

        /// set translation
        INLINE Transform& translation(const Translation& t);

        /// set translation
        INLINE Transform& translation(float x, float y, float z);

        /// get raw rotation element
        INLINE const Rotation& rotation() const { return m_rot; }

        /// set rotation
        INLINE Transform& rotation(const Rotation& rotation);

        /// set rotation
        INLINE Transform& rotation(float pitch, float yaw, float roll);

        /// get raw scale element
        INLINE const Scale& scale() const { return m_scale; }

        /// set scale
        INLINE Transform& scale(const Scale& s);

        /// set scale
        INLINE Transform& scale(float s);

        //---

        /// build a translation only transform
        INLINE static Transform T(const Translation& t);

        /// build a rotation only transform
        INLINE static Transform R(const Rotation& r);

        /// build a translation and rotation only transform
        INLINE static Transform TR(const Translation& t, const Rotation& r);

        /// build a scale only transform
        INLINE static Transform S(const Scale& s);

        /// build a scale only transform (uniform scaling version)
        INLINE static Transform S(float s);

        /// build a full TRS transform
        INLINE static Transform TRS(const Translation& t, const Rotation& r, const Scale& s);

        /// build a full TRS transform (uniform scale version)
        INLINE static Transform TRS(const Translation& t, const Rotation& r, float s);

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

        //---

        // Custom type implementation requirements
        bool writeBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryWriter& stream) const;
        bool writeText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextWriter& stream) const;
        bool readBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryReader& stream);
        bool readText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextReader& stream);

        // Calculate hash of the data
        void calcHash(CRC64& crc) const;

        //---

    private:
        Rotation m_rot;
        Translation m_trans;
        Scale m_scale;
    };

} // scene
