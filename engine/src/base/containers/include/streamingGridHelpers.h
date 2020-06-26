/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: streaming #]
***/

#pragma once

#include "streamingGrid.h"

namespace base
{
    namespace containers
    {
        //--

        /// position quantizer
        class BASE_CONTAINERS_API StreamingPositionQuantizer
        {
        public:
            StreamingPositionQuantizer(float maxWorldSize);

            struct QuantizedPosition
            {
                uint16_t x;
                uint16_t y;
                uint16_t z;
                uint16_t r;
            };

            INLINE QuantizedPosition quantizePosition(const float* v)
            {
                __m128 pos = _mm_set_ps(0.0f, v[2], v[1], v[0]);

                __m128 qpos = _mm_mul_ps(_mm_add_ps(pos, m_offset), m_scale);
                qpos = _mm_max_ps(qpos, MinC);
                qpos = _mm_min_ps(qpos, MaxC);

                __m128i q = _mm_cvtps_epi32(qpos);

                auto qptr  = (const int*)&q;
                QuantizedPosition ret;
                ret.x = (uint16_t)(qptr[0]);
                ret.y = (uint16_t)(qptr[1]);
                ret.z = (uint16_t)(qptr[2]);
                ret.r = 0;
                return ret;
            }

            INLINE QuantizedPosition quantizePositionAndRadius(const float* v, float r)
            {
                __m128 pos = _mm_set_ps(r, v[2], v[1], v[0]);

                __m128 qpos = _mm_mul_ps(_mm_add_ps(pos, m_offset), m_scale);
                qpos = _mm_max_ps(qpos, MinC);
                qpos = _mm_min_ps(qpos, MaxC);

                __m128i q = _mm_cvtps_epi32(qpos);

                auto qptr  = (const int*)&q;

                QuantizedPosition ret;
                ret.x = (uint16_t)(qptr[0]);
                ret.y = (uint16_t)(qptr[1]);
                ret.z = (uint16_t)(qptr[2]);
                ret.r = (uint16_t)(qptr[3]);
                return ret;
            }

        private:
            static const __m128 MinC;
            static const __m128 MaxC;

            __m128  m_scale;
            __m128  m_offset;
        };

        //-----------------------------

        /// collector for objects from streaming grid
        template< uint32_t N >
        class StreamingGridCollectorStack : public StreamingGridCollector
        {
        public:
            StreamingGridCollectorStack()
                : StreamingGridCollector(&m_data[0], N)
            {}

        private:
            Elem m_data[N];
        };

        //-----------------------------

    } // streaming
} // base

