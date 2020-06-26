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

    //---

    uint32_t MTGenerator::nextUint32()
    {
        if (index == SIZE)
            generateNumbers();

        return MT_TEMPERED[index++];
    }

    uint64_t MTGenerator::nextUint64()
    {
        return (uint64_t)nextUint32() | ((uint64_t)nextUint32() << 32);
    }

    float MTGenerator::nextFloat()
    {
        auto val = (double)nextUint32();
        return (float)(val / (double)0xFFFFFFFF);
    }

    //---

} // base
