/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: streaming #]
***/

#include "build.h"
#include "streamingGrid.h"
#include "streamingGridHelpers.h"

namespace base
{
    namespace containers
    {

        const __m128 StreamingPositionQuantizer::MinC = _mm_setzero_ps();
        const __m128 StreamingPositionQuantizer::MaxC = _mm_set1_ps(65535.0f);

        StreamingPositionQuantizer::StreamingPositionQuantizer(float worldSize)
        {
            float ofs = worldSize / 2.0f;
            m_offset = _mm_set_ps(0.0f, ofs, ofs, ofs);
            m_scale = _mm_set1_ps(65535.0f / worldSize);
        }       

    } // streaming
} // base