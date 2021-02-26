/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include <cmath>

BEGIN_BOOMER_NAMESPACE_EX(replication)

//--

INLINE uint32_t BitCount::maxUnsigned() const
{
    if (m_bitCount == 0 || m_bitCount >= 32)
        return ~0U;

    return (1U << m_bitCount) - 1;
}

INLINE int BitCount::maxSigned() const
{
    if (m_bitCount < 2 || m_bitCount >= 32)
        return std::numeric_limits<int>::max();

    auto half = (int)1 << (m_bitCount-1);
    return half - 1;
}

INLINE int BitCount::minSigned() const
{
    if (m_bitCount < 2 || m_bitCount >= 32)
        return std::numeric_limits<int>::min();

    auto mask = (1U << (m_bitCount - 1)) - 1;
    return (int)~mask;
}

//--

INLINE float Quantization::quantizationError() const
{
    return m_quantizationStep;
}

INLINE QuantizedValue Quantization::quantizeFloat(float val) const
{
    if (val >= m_quantizationMax)
        return m_quantizationMaxValue;

    if (val > m_quantizationMin)
    {
        auto q = (int) std::round((val - m_quantizationMin) * m_quantizationInvStep);
        return (q == m_quantizationMaxValue) ? m_quantizationMaxValue : q;
    }

    return 0;
}

INLINE float Quantization::unquantizeFloat(QuantizedValue val) const
{
    return m_quantizationMin + (val * m_quantizationStep);
}

INLINE QuantizedValue Quantization::quantizeUnsigned(uint32_t val) const
{
    return val >= m_quantizationMaxValue ? m_quantizationMaxValue : val;
}

INLINE uint32_t Quantization::unquantizeUnsigned(QuantizedValue val) const
{
    return val & m_quantizationMaxValue;
}

INLINE QuantizedValue Quantization::quantizeSigned(int val) const
{
    QuantizedValue q = val;

    if (val < m_bitCount.minSigned())
        q = m_bitCount.minSigned();
    else  if (val > m_bitCount.maxSigned())
        q = m_bitCount.maxSigned();

    return q & m_quantizationMaxValue;
}

INLINE int Quantization::unquantizeSigned(QuantizedValue val) const
{
    auto signedBit = 1U << (m_bitCount.m_bitCount - 1);
    if (val & signedBit)
        return (int)(val | ~m_quantizationMaxValue);
    else
        return (int)(val & m_quantizationMaxValue);
}

//--

END_BOOMER_NAMESPACE_EX(replication)
