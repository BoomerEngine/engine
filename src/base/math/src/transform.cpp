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
        RTTI_BIND_NATIVE_COPY(Transform);
        RTTI_BIND_NATIVE_COMPARE(Transform);
        RTTI_BIND_NATIVE_BINARY_SERIALIZATION(Transform);
    RTTI_END_TYPE();

    bool Transform::isIdentity() const
    {
        return (T == Translation::ZERO()) && (R == Rotation::IDENTITY()) && (S == Scale::ONE());
    }

    Transform Transform::invertedWithNoScale() const
    {
        auto invRotation  = R.inverted();
        auto invTranslation  = -invRotation.transformVector(T);
        return Transform(invTranslation, invRotation);
    }

    Transform Transform::inverted() const
    {
        return Transform(-R.inverted().transformVector(T) / S, R.inverted(), S.oneOver());
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

        return Transform(baseTransform.R.transformVector(baseTransform.S * T) + baseTransform.T,
                              baseTransform.R * R,
                              S * baseTransform.S);
    }

    static Transform IDENTITY_TRANSFORM;

    const Transform& Transform::IDENTITY()
    {
        return IDENTITY_TRANSFORM;
    }

    Matrix Transform::toMatrix() const
    {
        Matrix ret = R.toMatrix();
        ret.translation(T);
        ret.scaleColumns(S);
        return ret;
    }

    Matrix Transform::toInverseMatrix() const
    {
        return inverted().toMatrix();
    }

    EulerTransform Transform::toEulerTransform() const
    {
        EulerTransform ret;
        ret.T = T;
        ret.S = S;
        ret.R = R.toRotator();
        return ret;
    }

    ///----

    void Transform::writeBinary(base::stream::OpcodeWriter& stream) const
    {
        uint8_t flags = 0;

        if (T != Vector3::ZERO())
            flags |= 1;
        if (R != Quat::IDENTITY())
            flags |= 2;
        if (S != Vector3::ONE())
            flags |= 4;

        stream.writeTypedData(flags);

        if (flags & 1)
            stream.writeTypedData(T);

        if (flags & 2)
            stream.writeTypedData(R);

        if (flags & 4)
            stream.writeTypedData(S);
    }

    void Transform::readBinary(base::stream::OpcodeReader& stream)
    {
        uint8_t flags = 0;
        stream.readTypedData(flags);

        if (flags & 1)
            stream.readTypedData(T);
        else
            T = Vector3::ZERO();

        if (flags & 2)
            stream.readTypedData(R);
        else
            R = Quat::IDENTITY();

        if (flags & 4)
            stream.readTypedData(S);
        else
            S = Vector3::ONE();
    }

} // base