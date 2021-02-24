/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\float16 #]
***/

#include "build.h"

BEGIN_BOOMER_NAMESPACE(base)

uint16_t Float16Helper::Compress(float value)
{
    Bits v, s;
    v.f = value;
    uint32_t sign = v.si & signN;
    v.si ^= sign;
    sign >>= shiftSign; // logical shift
    s.si = mulN;
    s.si = (int)(s.f * v.f); // correct subnormals
    v.si ^= (s.si ^ v.si) & -(minN > v.si);
    v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
    v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));
    v.ui >>= shift; // logical shift
    v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
    v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
    return (uint16_t)(v.ui | sign);
}

float Float16Helper::Decompress(uint16_t value)
{
    Bits v;
    v.ui = value;
    int sign = v.si & signC;
    v.si ^= sign;
    sign <<= shiftSign;
    v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
    v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
    Bits s;
    s.si = mulC;
    s.f *= v.si;
    int mask = -(norC > v.si);
    v.si <<= shift;
    v.si ^= (s.si ^ v.si) & mask;
    v.si |= sign;
    return v.f;
}

END_BOOMER_NAMESPACE(base)