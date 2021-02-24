/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\point #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE(base)


//--

RTTI_BEGIN_TYPE_STRUCT(Point);
    RTTI_BIND_NATIVE_COMPARE(Point);
    RTTI_TYPE_TRAIT().zeroInitializationValid().noConstructor().noDestructor().fastCopyCompare();
    RTTI_PROPERTY(x).editable();
    RTTI_PROPERTY(y).editable();
RTTI_END_TYPE();

//--

Vector2 Point::toVector() const
{
    return Vector2((float)x, (float)y);
}

//--

static Point ZERO_P(0,0);

const Point& Point::ZERO()
{
    return ZERO_P;
}

//--

Point Lerp(const Point& a, const Point& b, float frac)
{
    return Lerp(a.toVector(), b.toVector(), frac);
}

Point Min(const Point& a, const Point& b)
{
    return Point(std::min(a.x, b.x), std::min(a.y, b.y));
}

Point Max(const Point& a, const Point& b)
{
    return Point(std::max(a.x, b.x), std::max(a.y, b.y));
}

Point Clamp(const Point &a, const Point &minV, const Point &maxV)
{
    return Point(std::clamp(a.x, minV.x, maxV.x), std::clamp(a.y, minV.y, maxV.y));
}

Point Clamp(const Point &a, int minF, int maxF)
{
    return Point(std::clamp(a.x, minF, maxF), std::clamp(a.y, minF, maxF));
}

//--

END_BOOMER_NAMESPACE(base)