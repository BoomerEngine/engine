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

RTTI_BEGIN_TYPE_STRUCT(ExactPosition);
    RTTI_BIND_NATIVE_COMPARE(ExactPosition);
    RTTI_PROPERTY(x).editable();
    RTTI_PROPERTY(y).editable();
    RTTI_PROPERTY(z).editable();
RTTI_END_TYPE();

//--

static ExactPosition ROOT_ABSPOS(0.0f, 0.0f, 0.0f);

void ExactPosition::print(IFormatStream& f) const
{
    f.appendf("{x={}, y={}, z={}}", Prec(x, 3), Prec(y, 3), Prec(z, 3));
}

const ExactPosition& ExactPosition::ZERO()
{
    return ROOT_ABSPOS;
}

//--

END_BOOMER_NAMESPACE()
