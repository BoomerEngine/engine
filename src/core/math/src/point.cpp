/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\point #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()


//--

RTTI_BEGIN_TYPE_STRUCT(Point);
    RTTI_BIND_NATIVE_COMPARE(Point);
    RTTI_TYPE_TRAIT().zeroInitializationValid().noConstructor().noDestructor().fastCopyCompare();
    RTTI_PROPERTY(x).editable();
    RTTI_PROPERTY(y).editable();
RTTI_END_TYPE();

//--

static Point ZERO_P(0,0);

const Point& Point::ZERO()
{
    return ZERO_P;
}

//--

END_BOOMER_NAMESPACE()
