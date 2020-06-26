/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: random #]
**/

#pragma once

namespace base
{

    //--

    uint32_t FastGenerator::nextUint32()
    {
        return (uint32_t)nextUint64();
    }

    uint64_t FastGenerator::nextUint64()
    {
        return StatelessNextUint64(m_state);
    }

    float FastGenerator::nextFloat()
    {
        auto val  = (double)nextUint32();
        return (float)(val / (double)0xFFFFFFFF);
    }

    //--

} // base
