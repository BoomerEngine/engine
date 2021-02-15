/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity #]
*
***/

#include "build.h"
#include "worldEntityBehavior.h"

namespace base
{
    namespace world
    {

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IEntityBehavior);
        RTTI_END_TYPE();

        //--

        IEntityBehavior::IEntityBehavior()
        {}

        IEntityBehavior::~IEntityBehavior()
        {}

        //--

    } // world
} // base