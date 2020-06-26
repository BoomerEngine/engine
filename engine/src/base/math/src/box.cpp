/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\box #]
***/

#include "build.h"

namespace base
{
    //--

    RTTI_BEGIN_TYPE_STRUCT(Box);
        RTTI_BIND_NATIVE_COMPARE(Box);
        RTTI_PROPERTY(min).editable();
        RTTI_PROPERTY(max).editable();
    RTTI_END_TYPE();

    //--

    static Box ZERO_B(Vector3(0,0,0), Vector3(0,0,0));
    static Box EMPTY_B(Vector3(VERY_LARGE_FLOAT, VERY_LARGE_FLOAT, VERY_LARGE_FLOAT), -Vector3(-VERY_LARGE_FLOAT,-VERY_LARGE_FLOAT,-VERY_LARGE_FLOAT));
    static Box UNIT_B(Vector3(0,0,0), Vector3(1,1,1));

    const Box& Box::ZERO()
    {
        return ZERO_B;
    }

    const Box& Box::EMPTY()
    {
        return EMPTY_B;
    }

    const Box& Box::UNIT()
    {
        return UNIT_B;
    }

    //--

    Vector3 Box::rand() const
    {
        return Vector3(RandRange(min.x, max.x), RandRange(min.y, max.y), RandRange(min.z, max.z));
    }

    //--

} // base