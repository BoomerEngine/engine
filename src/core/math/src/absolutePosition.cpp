/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\absolute #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_STRUCT(AbsolutePosition);
    RTTI_BIND_NATIVE_COMPARE(AbsolutePosition);
    RTTI_PROPERTY(m_primary);
    RTTI_PROPERTY(m_error);
RTTI_END_TYPE();

//--

AbsolutePosition Snap(const AbsolutePosition& a, float grid)
{
    return AbsolutePosition(Snap(a.approximate(), grid));
}

AbsolutePosition Lerp(const AbsolutePosition& a, const AbsolutePosition& b, float frac)
{
    return AbsolutePosition(Lerp(a.approximate(), b.approximate(), frac), Lerp(a.error(), b.error(), frac));
}

AbsolutePosition Min(const AbsolutePosition &a, const AbsolutePosition &b)
{
    double ax, ay, az;
    a.expand(ax, ay, az);

    double bx, by, bz;
    b.expand(bx, by, bz);

    return AbsolutePosition(std::min(ax, bx), std::min(ay, by), std::min(az, bz));
}

AbsolutePosition Max(const AbsolutePosition &a, const AbsolutePosition &b)
{
    double ax, ay, az;
    a.expand(ax, ay, az);

    double bx, by, bz;
    b.expand(bx, by, bz);

    return AbsolutePosition(std::max(ax, bx), std::max(ay, by), std::max(az, bz));
}

AbsolutePosition Clamp(const AbsolutePosition &a, const AbsolutePosition &minV, const AbsolutePosition &maxV)
{
    double ax, ay, az;
    a.expand(ax, ay, az);

    double minX, minY, minZ;
    minV.expand(minX, minY, minZ);

    double maxX, maxY, maxZ;
    maxV.expand(maxX, maxY, maxZ);

    return AbsolutePosition(std::clamp(ax, minX, maxX), std::clamp(ay, minY, maxY), std::clamp(az, minZ, maxZ));
}

AbsolutePosition Clamp(const AbsolutePosition &a, double minF, double maxF)
{
    double ax, ay, az;
    a.expand(ax, ay, az);

    return AbsolutePosition(std::clamp(ax, minF, maxF), std::clamp(ay, minF, maxF), std::clamp(az, minF, maxF));
}

//--

static AbsolutePosition ROOT_ABSPOS(0.0f, 0.0f, 0.0f);

const AbsolutePosition& AbsolutePosition::ROOT()
{
    return ROOT_ABSPOS;
}

//--

END_BOOMER_NAMESPACE()
