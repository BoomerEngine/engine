/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\transform #]
***/

#include "build.h"
#include "base/containers/include/stringParser.h"
#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/streamTextWriter.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/streamTextReader.h"
#include "base/containers/include/stringBuilder.h"

namespace base
{
    RTTI_BEGIN_CUSTOM_TYPE(Transform);
        RTTI_TYPE_TRAIT().noDestructor().fastCopyCompare();
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

    bool Transform::writeBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryWriter& stream) const
    {
        stream.write(&m_trans, sizeof(m_trans));
        stream.write(&m_rot, sizeof(m_rot));
        stream.write(&m_scale, sizeof(m_scale));
        return true;
    }

    bool Transform::writeText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextWriter& stream) const
    {
        // TODO: remove string allocation
        stream.writeValue(TempString("{},{},{},{},{},{},{},{},{},{}", m_trans.x, m_trans.y, m_trans.z, m_rot.x, m_rot.y, m_rot.z, m_rot.w, m_scale.x, m_scale.y, m_scale.z));
        return true;
    }

    bool Transform::readBinary(const base::rtti::TypeSerializationContext& typeContext, base::stream::IBinaryReader& stream)
    {
        stream.read(&m_trans, sizeof(m_trans));
        stream.read(&m_rot, sizeof(m_rot));
        return true;
    }

    bool Transform::readText(const base::rtti::TypeSerializationContext& typeContext, base::stream::ITextReader& stream)
    {
        base::Vector3 pos;
        base::Quat rot;
        base::Vector3 scale(1,1,1);

        base::StringView<char> val;
        if (!stream.readValue(val))
            return false;

        base::StringParser parser(val);
        if (!parser.parseFloat(pos.x)) return false;
        if (!parser.parseKeyword(",")) return false;
        if (!parser.parseFloat(pos.y)) return false;
        if (!parser.parseKeyword(",")) return false;
        if (!parser.parseFloat(pos.z)) return false;
        if (!parser.parseKeyword(",")) return false;
        if (!parser.parseFloat(rot.x)) return false;
        if (!parser.parseKeyword(",")) return false;
        if (!parser.parseFloat(rot.y)) return false;
        if (!parser.parseKeyword(",")) return false;
        if (!parser.parseFloat(rot.z)) return false;
        if (!parser.parseKeyword(",")) return false;
        if (!parser.parseFloat(rot.w)) return false;
        if (!parser.parseKeyword(",")) return false;
        if (!parser.parseFloat(scale.x)) return false;
        if (!parser.parseKeyword(",")) return false;
        if (!parser.parseFloat(scale.y)) return false;
        if (!parser.parseKeyword(",")) return false;
        if (!parser.parseFloat(scale.z)) return false;

        m_trans = pos;
        m_rot = rot;
        m_scale = scale;
        return true;
    }

    void Transform::calcHash(CRC64& crc) const
    {
        crc << m_trans.x;
        crc << m_trans.y;
        crc << m_trans.z;
        crc << m_rot.x;
        crc << m_rot.y;
        crc << m_rot.z;
        crc << m_rot.w;
        crc << m_scale.x;
        crc << m_scale.y;
        crc << m_scale.z;
    }

} // base