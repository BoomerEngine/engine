/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math #]
***/

#include "build.h"
#include "mathRandom.h"
#include "box.h"
#include "sphere.h"
#include "obb.h"

BEGIN_BOOMER_NAMESPACE()

//--

static const double INV_MAX_UINT32 = 1.0 / (double)(uint64_t)(1ULL << 32); // 1/4B

//--

IRandom::~IRandom()
{}

//--

FastRandState::FastRandState(uint64_t seed /*= 0*/)
{
    state = seed;
}

void FastRandState::seed(uint32_t val)
{
    state = val;
}

uint32_t FastRandState::next()
{
    uint64_t z = (state + UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30))* UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27))* UINT64_C(0x94D049BB133111EB);
    state = z ^ (z >> 31);
    return (uint32_t)state;
}

//--

#define MT_SIZE 624
#define MT_PERIOD 397
#define MT_DIFF MT_SIZE - MT_PERIOD
#define MT_MAGIC 0x9908b0df

#define M32(x) (0x80000000 & x) // 32nd MSB
#define L31(x) (0x7FFFFFFF & x) // 31 LSBs

#define UNROLL(expr) \
    y = M32(MT[i]) | L31(MT[i+1]); \
    MT[i] = MT[expr] ^ (y >> 1) ^ (((int(y) << 31) >> 31) & MT_MAGIC); \
    ++i;

void MTRandState::generate()
{
    int i = 0;
    uint32_t y;

    while (i < MT_DIFF)
    {
        UNROLL(i + MT_PERIOD);
        UNROLL(i + MT_PERIOD);
    }

    while (i < MT_SIZE - 1)
    {
        UNROLL(i - MT_DIFF);
        UNROLL(i - MT_DIFF);
        UNROLL(i - MT_DIFF);
        UNROLL(i - MT_DIFF);
        UNROLL(i - MT_DIFF);
        UNROLL(i - MT_DIFF);
        UNROLL(i - MT_DIFF);
        UNROLL(i - MT_DIFF);
        UNROLL(i - MT_DIFF);
        UNROLL(i - MT_DIFF);
        UNROLL(i - MT_DIFF);
    }

    {
        y = M32(MT[MT_SIZE - 1]) | L31(MT[0]);
        MT[MT_SIZE - 1] = MT[MT_PERIOD - 1] ^ (y >> 1) ^ (((int32_t(y) << 31) >> 31)& MT_MAGIC);
    }

    for (size_t i = 0; i < MT_SIZE; ++i)
    {
        y = MT[i];
        y ^= y >> 11;
        y ^= y << 7 & 0x9d2c5680;
        y ^= y << 15 & 0xefc60000;
        y ^= y >> 18;
        MT_TEMPERED[i] = y;
    }

    index = 0;
}


void MTRandState::seed(uint32_t seed)
{
    MT[0] = seed;
    for (uint32_t i = 1; i < MT_SIZE; ++i)
        MT[i] = 0x6c078965 * (MT[i - 1] ^ MT[i - 1] >> 30) + i;

    index = MT_SIZE; // generate on next get
}

uint32_t MTRandState::next()
{
    if (index == MT_SIZE)
        generate();

    return MT_TEMPERED[index++];
}

MTRandState::MTRandState(uint32_t val /*= 0*/)
{
    seed(val);
}

//--

END_BOOMER_NAMESPACE()
