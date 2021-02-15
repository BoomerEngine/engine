/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_image_glue.inl"

namespace base
{
    namespace image
    {
        class Image;
        typedef RefPtr<Image> ImagePtr;
        typedef res::Ref<Image> ImageRef;

        class ImageView;

		struct ImageRect;

        /// Image formats (global)
        enum class PixelFormat : uint8_t
        {
            Uint8_Norm, // normalized to 0-1 range
            Uint16_Norm, // normalized to 0-1 range
            Float16_Raw, // raw format
            Float32_Raw, // raw format
        };

        class DynamicAtlas;
        class SpaceAllocator;

        /// color space, used by various filters that try to be a little bit more exact when filtering colors
        /// it's a good practice not to ignore this
        enum class ColorSpace : uint8_t
        {
            // linear color space, values can be just merged together
            Linear,

            // sRGB color space, values represent color in sRGB space, should be converted back and forth before operation
            SRGB,

            // HDR perceptual scale, although values are linear our perception of them is not, use logarithmic weights when mixing color
            HDR,

            // Packed normals (needs unpacking and normalization)
            Normals,
        };

        /// downsampling mode
        enum class DownsampleMode : uint8_t
        {
            Average,
            AverageWithAlphaWeight,
            AverageWithPremultipliedAlphaWeight,
        };

        // cubemap sides enumeration
        enum class CubeSide : uint8_t
        {
            PositiveX,
            NegativeX,
            PositiveY,
            NegativeY,
            PositiveZ,
            NegativeZ,
        };

        //--

    } // image
} // base
