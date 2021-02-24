/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\range #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE(base)

//--

RTTI_BEGIN_TYPE_STRUCT(Range);
    RTTI_BIND_NATIVE_COMPARE(Range);
    RTTI_TYPE_TRAIT().zeroInitializationValid().noConstructor().noDestructor().fastCopyCompare();
    RTTI_PROPERTY(min).editable();
    RTTI_PROPERTY(max).editable();
RTTI_END_TYPE();

//--

static Range ZERO_R(0.0f, 0.0f);
static Range UNIT_R(0.0f, 1.0f);
static Range PLUS_MINUS_ONE_R(-1.0f, 1.0f);
static Range PLUS_MINUS_HALF_R(-0.5f, 0.5f);
static Range EMPTY_R(INFINITY, -INFINITY);;

const Range& Range::ZERO() { return ZERO_R; }
const Range& Range::UNIT() { return UNIT_R; }
const Range& Range::PLUS_MINUS_ONE() { return PLUS_MINUS_ONE_R; }
const Range& Range::PLUS_MINUS_HALF() { return PLUS_MINUS_HALF_R; }
const Range& Range::EMPTY() { return EMPTY_R; }

//--

Range Snap(const Range &a, float grid)
{
    return Range(Snap(a.min, grid), Snap(a.max, grid));
}

Range Lerp(const Range &a, const Range &b, float frac)
{
    return Range(Lerp(a.min, b.min, frac), Lerp(a.max, b.max, frac));
}

Range Min(const Range &a, const Range &b)
{
    return Range(std::min(a.min, b.min), std::min(a.max, b.max));
}

Range Max(const Range &a, const Range &b)
{
    return Range(std::max(a.min, b.min), std::max(a.max, b.max));
}

Range Clamp(const Range &a, const Range &minV, const Range &maxV);
Range Clamp(const Range &a, float minF, float maxF);

//--

END_BOOMER_NAMESPACE(base)
