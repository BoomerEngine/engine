/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\absolute #]
***/

#pragma once

#include "core/reflection/include/reflectionMacros.h"

BEGIN_BOOMER_NAMESPACE()

/// Global transform
TYPE_ALIGN(16, class) CORE_MATH_API AbsoluteTransform
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(AbsoluteTransform);

public:
    INLINE AbsoluteTransform();
    INLINE AbsoluteTransform(const AbsolutePosition& pos, const Quat& rotation = Quat::IDENTITY(), Vector3 scale = Vector3::ONE());
    INLINE AbsoluteTransform(const AbsoluteTransform& other) = default;
    INLINE AbsoluteTransform(AbsoluteTransform&& other) = default;
    INLINE AbsoluteTransform& operator=(const AbsoluteTransform& other) = default;
    INLINE AbsoluteTransform& operator=(AbsoluteTransform&& other) = default;
    INLINE ~AbsoluteTransform() = default;

    INLINE bool operator==(const AbsoluteTransform& other) const;
    INLINE bool operator!=(const AbsoluteTransform& other) const;

    INLINE AbsoluteTransform operator*(const Transform& relativeTransform) const;
    INLINE AbsoluteTransform& operator*=(const Transform& relativeTransform);

    INLINE AbsoluteTransform operator*(const Quat& rot) const;
    INLINE AbsoluteTransform& operator*=(const Quat& rot);

    INLINE AbsoluteTransform operator+(const Vector3& delta) const;
    INLINE AbsoluteTransform& operator+=(const Vector3& delta);
    INLINE AbsoluteTransform operator-(const Vector3& delta) const;
    INLINE AbsoluteTransform& operator-=(const Vector3& delta);

    Matrix operator/(const AbsolutePosition& basePosition) const; // compute transform relative to base
    Transform operator/(const AbsoluteTransform& baseTransform) const;  // compute transform relative to base

    //--

    /// get the absolute position
    INLINE const AbsolutePosition& position() const;

    /// set absolute position in the transform
    INLINE AbsoluteTransform& position(const AbsolutePosition& pos);

    /// set absolute position in the transform
    INLINE AbsoluteTransform& position(const Vector3& pos);

    /// set absolute position in the transform
    INLINE AbsoluteTransform& position(double x, double y, double z);

    /// get the rotation
    INLINE const Quat& rotation() const;

    /// set rotation from quaternion
    INLINE AbsoluteTransform& rotation(const Quat& rot);

    /// set rotation directly from angles
    INLINE AbsoluteTransform& rotation(const Angles& angles);

    /// set rotation directly from angle tripplet
    INLINE AbsoluteTransform& rotation(float pitch, float yaw, float roll = 0.0f);

    /// get the scale
    INLINE const Vector3& scale() const;

    /// set the scale from a single scalar
    INLINE AbsoluteTransform& scale(float scale);

    /// set the per axis scale
    INLINE AbsoluteTransform& scale(const Vector3& scale);

    /// set the per axis scale from components
    INLINE AbsoluteTransform& scale(float x, float y, float z);

    //--

    // get approximate matrix that does the same transform
    Matrix approximate() const;

    // transform position from local space to absolute world space using this transform
    AbsolutePosition transformPointFromSpace(const Vector3& localPoint) const;

    // transform direction from local space to absolute world space using this transform
    Vector3 transformDirectionFromSpace(const Vector3& localDir) const;

    // transform absolute position to local space using this transform
    Vector3 transformPointToSpace(const AbsolutePosition& absolutePosition) const;

    // transform absolute world space direction to local space using this transform
    Vector3 transformDirectionToSpace(const Vector3& worldDir) const;

    // transform bounding box from local space to absolute world space using this transform
    Box transformBox(const Box& localBox) const;

    //--

    // root transform (no rotation at root position)
    static const AbsoluteTransform& ROOT();

private:
    Quat m_rotation;
    AbsolutePosition m_position;
    Vector3 m_scale;
};

END_BOOMER_NAMESPACE()
