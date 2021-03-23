/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\range #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

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

END_BOOMER_NAMESPACE()
