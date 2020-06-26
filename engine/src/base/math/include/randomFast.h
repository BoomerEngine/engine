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
    /// simple but fast random number generator
    /// NOTE: not for any serious use
    class BASE_MATH_API FastGenerator
    {
    public:
        // set to specific seed
        void seed(uint32_t seed);

        // get 32-bit random number
        INLINE uint32_t nextUint32();

        // get 64-bit random number
        INLINE uint64_t nextUint64();

        // get next value as float in 0-1 range (without the 1)
        INLINE float nextFloat();

    private:
        uint64_t m_state = 0;
    };

} // base

#include "randomFast.inl"