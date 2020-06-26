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

    /// Mersenne Twister pseudo-random number generator
    class BASE_MATH_API MTGenerator
    {
    public:
        static const uint32_t MAX_VALUE = 0xFFFFFFFF;

        MTGenerator(uint32_t seed = 0);

        // set to specific seed
        void seed(uint32_t seed);

        // get 32-bit random number
        INLINE uint32_t nextUint32();

        // get 64-bit random number
        INLINE uint64_t nextUint64();

        // get next value as float in 0-1 range (without the 1)
        INLINE float nextFloat();

    private:
        static const uint32_t SIZE = 624;
        static const uint32_t PERIOD = 397;
        static const uint32_t DIFF = SIZE - PERIOD;
        static const uint32_t MAGIC = 0x9908b0df;

        uint32_t MT[SIZE];
        uint32_t MT_TEMPERED[SIZE];
        uint32_t index = SIZE;

        void generateNumbers();
    };

    //--

} // base

#include "randomMersenne.inl"