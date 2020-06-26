/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: random #]
**/

#include "build.h"
#include "randomFast.h"

namespace base
{

    void FastGenerator::seed(uint32_t seed)
    {
        m_state = 0;
    }

} // base
