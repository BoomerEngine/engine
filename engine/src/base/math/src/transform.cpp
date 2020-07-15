/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\transform #]
***/

#include "build.h"
#include "base/containers/include/stringParser.h"
#include "base/containers/include/stringBuilder.h"
#include "base/object/include/streamOpcodeWriter.h"
#include "base/object/include/streamOpcodeReader.h"

namespace base
{
    RTTI_BEGIN_CUSTOM_TYPE(Transform);
        RTTI_TYPE_TRAIT().noDestructor().fastCopyCompare();
        RTTI_BIND_NATIVE_BINARY_SERIALIZATION(Transform);
    RTTI_END_TYPE();

    Transform Transform::invertedWithNoScale() const
    {
        auto invRotation  = m_rot.inverted();
        auto invTranslation  = -invRotation.transformVector(m_trans);
        return Transform::TR(invTranslation, invRotation);
    }

    Transform Transform::inverted() const
    {
        return Transform::TRS(-m_rot.inverted().transformVector(m_trans) / m_scale, m_rot.inverted(), m_scale.oneOver());
    }

    Transform Transform::relativeTo(const Transform& baseTransform) const
    {
        return applyTo(baseTransform.inverted());
    }

    Transform Transform::applyTo(const Transform& baseTransform) const
    {
        // x2 = R1*x1 + T1
        // x3 = R2*x2 + T2 = R2(R1*x1 + T1) + T2
        // = R2*R1*x1 + T2 + T1*R2

        return Transform::TRS(baseTransform.m_rot.transformVector(baseTransform.m_scale * m_trans) + baseTransform.m_trans,
                              baseTransform.m_rot * m_rot,
                              m_scale * baseTransform.m_scale);
    }

    static Transform IDENTITY_TRANSFORM;

    const Transform& Transform::IDENTITY()
    {
        return IDENTITY_TRANSFORM;
    }

    base::Matrix Transform::toMatrix() const
    {
        base::Matrix ret = m_rot.toMatrix();
        ret.translation(m_trans);
        ret.scaleColumns(m_scale);
        return ret;
    }

    base::Matrix Transform::toInverseMatrix() const
    {
        return inverted().toMatrix();
    }

    ///----

    void Transform::writeBinary(base::stream::OpcodeWriter& stream) const
    {
        uint8_t flags = 0;

        if (m_trans != Vector3::ZERO())
            flags |= 1;
        if (m_rot != Quat::IDENTITY())
            flags |= 2;
        if (m_scale != Vector3::ONE())
            flags |= 4;

        stream.writeTypedData(flags);

        if (flags & 1)
            stream.writeTypedData(m_trans);

        if (flags & 2)
            stream.writeTypedData(m_rot);

        if (flags & 4)
            stream.writeTypedData(m_scale);
    }

    void Transform::readBinary(base::stream::OpcodeReader& stream)
    {
        uint8_t flags = 0;
        stream.readTypedData(flags);

        if (flags & 1)
            stream.readTypedData(m_trans);
        else
            m_trans = Vector3::ZERO();

        if (flags & 2)
            stream.readTypedData(m_rot);
        else
            m_rot = Quat::IDENTITY();

        if (flags & 4)
            stream.readTypedData(m_scale);
        else
            m_scale = Vector3::ONE();
    }

} // base