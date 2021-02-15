/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image #]
***/

#pragma once

namespace base
{
    namespace image
    {

        //--
      
        INLINE float LinearToSRGB(float linear)
        {
            if (linear <= 0.0031308f)
                return linear * 12.92f;
            else
                return 1.055f * pow(linear, 1.0f / 2.4f) - 0.055f;
        }

        INLINE uint8_t LinearToSRGB(uint8_t linear)
        {
            return (uint8_t)std::clamp<float>(LinearToSRGB(linear / 255.0f) * 255.0f, 0.0f, 255.0f);
        }

        INLINE float SRGBToLinear(float srgb)
        {
            if (srgb <= 0.04045f)
                return srgb / 12.92f;
            else
                return powf((srgb + 0.055f) / 1.055f, 2.4f);
        }

        INLINE uint8_t SRGBToLinear(uint8_t srgb)
        {
            return (uint8_t)std::clamp<float>(SRGBToLinear(srgb / 255.0f) * 255.0f, 0.0f, 255.0f);
        }

        //--

    } // base
} // image