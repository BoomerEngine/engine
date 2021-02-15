/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

namespace base
{
    namespace replication
    {
        //--

        // bit field size
        // NOTE: we only support bit count up to 32
        struct BitCount
        {
            uint8_t m_bitCount = 0;

            INLINE BitCount() = default;
            INLINE BitCount(uint8_t bitCount) : m_bitCount(bitCount) {};
            INLINE BitCount(const BitCount& other) = default;
            INLINE BitCount(BitCount&& other) = default;
            INLINE BitCount& operator=(const BitCount& other) = default;
            INLINE BitCount& operator=(BitCount&& other) = default;

            // get maximum representable unsigned value with this number of bits
            INLINE uint32_t maxUnsigned() const;

            // get minimum representable signed value with this number of bits
            INLINE int minSigned() const;

            // get maximum representable signed value with this number of bits
            INLINE int maxSigned() const;
        };

        //--

        // simple value quantization helper
        struct BASE_REPLICATION_API Quantization
        {
            BitCount m_bitCount;
            float m_quantizationMin = 0.0f;
            float m_quantizationMax = 0.0f;
            float m_quantizationStep = 0.0f; // NOTE: computed as (m_quantizationMax - m_quantizationMin) / 2^BitCount
            float m_quantizationInvStep = 0.0f; // NOTE: computed as 2^BitCount / (m_quantizationMax - m_quantizationMin)
            QuantizedValue m_quantizationMaxValue = 0; // 2^bitCount - 1

            INLINE Quantization(const Quantization& other) = default;
            INLINE Quantization(Quantization&& other) = default;
            INLINE Quantization& operator=(const Quantization& other) = default;
            INLINE Quantization& operator=(Quantization&& other) = default;

            Quantization();
            Quantization(uint8_t bitCount);
            Quantization(uint8_t bitCount, float valueMin, float valueMax);
            Quantization(uint8_t bitCount, float valueMax);

            //--

            // get absolute quantization error
            INLINE float quantizationError() const;

            // quantize a floating point value, clamps it to range
            INLINE QuantizedValue quantizeFloat(float val) const;

            // convert from quantized float to a full value
            INLINE float unquantizeFloat(QuantizedValue val) const;

            //--

            // quantize unsigned value, limits the value to the allowed bit count
            INLINE QuantizedValue quantizeUnsigned(uint32_t val) const;

            // convert from quantized value to full value
            INLINE uint32_t unquantizeUnsigned(QuantizedValue val) const;

            //--

            // quantize signed value, limits the value to the allowed bit count
            // NOTE: bits higher than the allowed bit count are set to zero (no sign extension)
            INLINE QuantizedValue quantizeSigned(int val) const;

            // convert from quantized value to full value
            INLINE int unquantizeSigned(QuantizedValue val) const;

            //--

            // don't allow the 32 bit corner case
            static const uint32_t MAX_BITS = 31;
        };

        //--

    } // replication
} // base

#include "replicationQuantization.inl"