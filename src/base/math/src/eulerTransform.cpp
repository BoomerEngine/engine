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
#include "base/system/include/format.h"

BEGIN_BOOMER_NAMESPACE(base)

RTTI_BEGIN_TYPE_CLASS(EulerTransform);
    RTTI_TYPE_TRAIT().noDestructor().fastCopyCompare();
    RTTI_PROPERTY(T).editable();
    RTTI_PROPERTY(R).editable();
    RTTI_PROPERTY(S).editable();
RTTI_END_TYPE();

bool EulerTransform::isIdentity() const
{
    return (T == Translation::ZERO()) && (R == Rotation::ZERO()) && (S == Scale::ONE());
}

Transform EulerTransform::toTransform() const
{
    const auto quat = R.toQuat();
    return Transform(T, quat, S);
}

static EulerTransform IDENTITY_TRANSFORM;

const EulerTransform& EulerTransform::IDENTITY()
{
    return IDENTITY_TRANSFORM;
}

base::Matrix EulerTransform::toMatrix() const
{
    base::Matrix ret = R.toMatrix();
    ret.translation(T);
    ret.scaleColumns(S);
    return ret;
}

Matrix EulerTransform::toMatrixNoScale() const
{
    base::Matrix ret = R.toMatrix();
    ret.translation(T);
    return ret;
}

///----

void EulerTransform::print(base::IFormatStream& f) const
{
    bool hasSomething = false;

    f << "{";

    if (T != Translation::ZERO())
    {
        f.appendf("T={}", T);
        hasSomething = true;
    }

    if (R != Rotation::ZERO())
    {
        if (hasSomething)
            f << ", ";
        f.appendf("R={}", R);
        hasSomething = true;
    }

    if (S != Scale::ZERO())
    {
        if (hasSomething)
            f << ", ";
        f.appendf("S={}", S);
        hasSomething = true;
    }

    if (!hasSomething)
        f << "Identity";

    f << "}";
}

///----

/*void EulerTransform::writeBinary(base::stream::OpcodeWriter& stream) const
{
    uint8_t flags = 0;

    if (T != Translation::ZERO())
        flags |= 1;
    if (R != Rotation::ZERO())
        flags |= 2;
    if (S != Scale::ONE())
        flags |= 4;

    stream.writeTypedData(flags);

    if (flags & 1)
        stream.writeTypedData(T);

    if (flags & 2)
        stream.writeTypedData(R);

    if (flags & 4)
        stream.writeTypedData(S);
}

void EulerTransform::readBinary(base::stream::OpcodeReader& stream)
{
    uint8_t flags = 0;
    stream.readTypedData(flags);

    if (flags & 1)
        stream.readTypedData(T);
    else
        T = Translation::ZERO();

    if (flags & 2)
        stream.readTypedData(R);
    else
        R = Rotation::ZERO();

    if (flags & 4)
        stream.readTypedData(S);
    else
        S = Scale::ONE();
}*/

//--

END_BOOMER_NAMESPACE(base)