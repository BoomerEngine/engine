/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: random #]
**/

#include "build.h"
#include "randomMersenne.h"

namespace base
{
    //--

    MTGenerator::MTGenerator(uint32_t initialSeed)
    {
        seed(initialSeed);
    }

    void MTGenerator::seed(uint32_t seed)
    {
        MT[0] = seed;
        index = SIZE;

        for (uint32_t i=1; i<SIZE; ++i)
            MT[i] = 0x6c078965*(MT[i-1] ^ MT[i-1]>>30) + i;
    }

    #define M32(x) (0x80000000 & x) // 32nd MSB
    #define L31(x) (0x7FFFFFFF & x) // 31 LSBs

    #define UNROLL(expr) \
        y = M32(MT[i]) | L31(MT[i+1]); \
        MT[i] = MT[expr] ^ (y >> 1) ^ (((int(y) << 31) >> 31) & MAGIC); \
        ++i;

    void MTGenerator::generateNumbers()
    {
        uint32_t i = 0;
        uint32_t y;

        while (i < DIFF)
        {
            UNROLL(i+PERIOD);
            UNROLL(i+PERIOD);
        }

        while (i < SIZE -1)
        {
            UNROLL(i-DIFF);
            UNROLL(i-DIFF);
            UNROLL(i-DIFF);
            UNROLL(i-DIFF);
            UNROLL(i-DIFF);
            UNROLL(i-DIFF);
            UNROLL(i-DIFF);
            UNROLL(i-DIFF);
            UNROLL(i-DIFF);
            UNROLL(i-DIFF);
            UNROLL(i-DIFF);
        }

        {
            y = M32(MT[SIZE-1]) | L31(MT[0]);
            MT[SIZE-1] = MT[PERIOD-1] ^ (y >> 1) ^ (((int32_t(y) << 31) >> 31) & MAGIC);
        }

        for (size_t i = 0; i < SIZE; ++i)
        {
            y = MT[i];
            y ^= y >> 11;
            y ^= y << 7  & 0x9d2c5680;
            y ^= y << 15 & 0xefc60000;
            y ^= y >> 18;
            MT_TEMPERED[i] = y;
        }

        index = 0;
    }

    //--

} // base
