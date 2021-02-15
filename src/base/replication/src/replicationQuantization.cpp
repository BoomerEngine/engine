/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "replicationQuantization.h"

namespace base
{
    namespace replication
    {

        //--

        Quantization::Quantization()
        {
            m_quantizationMaxValue = std::numeric_limits<uint32_t>::max();
        }

        Quantization::Quantization(uint8_t bitCount)
            : m_bitCount(bitCount)
        {
            ASSERT(bitCount <= MAX_BITS);
            m_quantizationMaxValue = (1U << bitCount) - 1;
        }

        Quantization::Quantization(uint8_t bitCount, float valueMin, float valueMax)
            : m_bitCount(bitCount)
        {
            ASSERT(bitCount <= MAX_BITS);
            ASSERT(valueMin < valueMax);
            m_quantizationMaxValue = (1U << bitCount) - 1;
            m_quantizationMin = valueMin;
            m_quantizationMax = valueMax;
            m_quantizationStep = (m_quantizationMax - m_quantizationMin) / (1U << bitCount);
            m_quantizationInvStep = (1U << bitCount) / (m_quantizationMax - m_quantizationMin);
        }

        Quantization::Quantization(uint8_t bitCount, float valueMax)
        {
            ASSERT(bitCount <= MAX_BITS);
            ASSERT(valueMax > 0.0f);
            m_quantizationMaxValue = (1U << bitCount) - 1;
            m_quantizationMin = 0.0f;
            m_quantizationMax = valueMax;
            m_quantizationStep = m_quantizationMax / (1U << bitCount);
            m_quantizationInvStep = (1U << bitCount) / m_quantizationMax;
        }

        //--

    } // replication
} // base