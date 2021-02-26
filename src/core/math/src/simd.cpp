/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: simd #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE()

//---

const SIMDQuad FullOnesBitMask[1] = { SIMDQuad((uint32_t)0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF) };

const SIMDQuad BitMasks[16] =
{
    SIMDQuad((uint32_t)0x00000000, 0x00000000, 0x00000000, 0x00000000),
    SIMDQuad((uint32_t)0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF),
    SIMDQuad((uint32_t)0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000),
    SIMDQuad((uint32_t)0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF),
    SIMDQuad((uint32_t)0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000),
    SIMDQuad((uint32_t)0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF),
    SIMDQuad((uint32_t)0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000),
    SIMDQuad((uint32_t)0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF),
    SIMDQuad((uint32_t)0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000),
    SIMDQuad((uint32_t)0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF),
    SIMDQuad((uint32_t)0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000),
    SIMDQuad((uint32_t)0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF),
    SIMDQuad((uint32_t)0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000),
    SIMDQuad((uint32_t)0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF),
    SIMDQuad((uint32_t)0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000),
    SIMDQuad((uint32_t)0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF),
};

const SIMDQuad ZeroVector[1] = { SIMDQuad(0.0f, 0.0f, 0.0f, 0.0f) };
const SIMDQuad OnesVector[1] = { SIMDQuad(1.0f, 1.0f, 1.0f, 1.0f) };
const SIMDQuad EXVector[1] = { SIMDQuad(1.0f, 0.0f, 0.0f, 0.0f) };
const SIMDQuad EYVector[1] = { SIMDQuad(0.0f, 1.0f, 0.0f, 0.0f) };
const SIMDQuad EZVector[1] = { SIMDQuad(0.0f, 0.0f, 1.0f, 0.0f) };
const SIMDQuad EWVector[1] = { SIMDQuad(0.0f, 0.0f, 0.0f, 1.0f) };

const SIMDQuad SignBitVector[1] = { SIMDQuad((uint32_t)0x80000000, (uint32_t)0x80000000, (uint32_t)0x80000000, (uint32_t)0x80000000) };
const SIMDQuad SignBitVector3[1] = { SIMDQuad((uint32_t)0x80000000, (uint32_t)0x80000000, (uint32_t)0x80000000, (uint32_t)0x00000000) };
const SIMDQuad SignMaskVector[1] = { SIMDQuad((uint32_t)0x7FFFFFFF, (uint32_t)0x7FFFFFFF, (uint32_t)0x7FFFFFFF, (uint32_t)0x7FFFFFFF) };
const SIMDQuad SignMaskVector3[1] = { SIMDQuad((uint32_t)0x7FFFFFFF, (uint32_t)0x7FFFFFFF, (uint32_t)0x7FFFFFFF, (uint32_t)0xFFFFFFFF) };

const SIMDQuad MaskQuads[16] =
{
    SIMDQuad((uint32_t)0x00000000, (uint32_t)0x00000000, (uint32_t)0x00000000, (uint32_t)0x00000000),
    SIMDQuad((uint32_t)0xFFFFFFFF, (uint32_t)0x00000000, (uint32_t)0x00000000, (uint32_t)0x00000000),
    SIMDQuad((uint32_t)0x00000000, (uint32_t)0xFFFFFFFF, (uint32_t)0x00000000, (uint32_t)0x00000000),
    SIMDQuad((uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF, (uint32_t)0x00000000, (uint32_t)0x00000000),
    SIMDQuad((uint32_t)0x00000000, (uint32_t)0x00000000, (uint32_t)0xFFFFFFFF, (uint32_t)0x00000000),
    SIMDQuad((uint32_t)0xFFFFFFFF, (uint32_t)0x00000000, (uint32_t)0xFFFFFFFF, (uint32_t)0x00000000),
    SIMDQuad((uint32_t)0x00000000, (uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF, (uint32_t)0x00000000),
    SIMDQuad((uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF, (uint32_t)0x00000000),
    SIMDQuad((uint32_t)0x00000000, (uint32_t)0x00000000, (uint32_t)0x00000000, (uint32_t)0xFFFFFFFF),
    SIMDQuad((uint32_t)0xFFFFFFFF, (uint32_t)0x00000000, (uint32_t)0x00000000, (uint32_t)0xFFFFFFFF),
    SIMDQuad((uint32_t)0x00000000, (uint32_t)0xFFFFFFFF, (uint32_t)0x00000000, (uint32_t)0xFFFFFFFF),
    SIMDQuad((uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF, (uint32_t)0x00000000, (uint32_t)0xFFFFFFFF),
    SIMDQuad((uint32_t)0x00000000, (uint32_t)0x00000000, (uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF),
    SIMDQuad((uint32_t)0xFFFFFFFF, (uint32_t)0x00000000, (uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF),
    SIMDQuad((uint32_t)0x00000000, (uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF),
    SIMDQuad((uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF, (uint32_t)0xFFFFFFFF)
};

const SIMDQuad ZeroValue[1] = { SIMDQuad(0.0f) };
const SIMDQuad OneValue[1] = { SIMDQuad(1.0f) };
const SIMDQuad EpsilonValue[1] = { SIMDQuad(0.0001f) };
const SIMDQuad EpsilonSquaredValue[1] = { SIMDQuad(0.0000001f) };

//---

END_BOOMER_NAMESPACE()
