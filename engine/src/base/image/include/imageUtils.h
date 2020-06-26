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

        ///----------------------------

        /// Convert value from linear space to sRGB space
        /// NOTE: value should be in 0-1 range
        static INLINE float LinearToSRGB(float linear);

        /// Convert value from linear space to sRGB space (for UNORM values)
        static INLINE uint8_t LinearToSRGB(uint8_t linear);

        /// Convert value from sRGB to linear color space
        /// NOTE: value should be in 0-1 range
        static INLINE float SRGBToLinear(float srgb);

        /// Convert value from sRGB to linear color space (for UNORM values)
        static INLINE uint8_t SRGBToLinear(uint8_t srgb);

        ///----------------------------

        /// Add/remove channels on a single line of image
        extern BASE_IMAGE_API void ConvertChannelsLine(PixelFormat format, const void* srcMem, uint8_t srcChannelCount, void* destMem, uint8_t destChannelCount, uint32_t width, const void* defaults);

        /// Convert channel count from one image to other, for new channels values are suck from defaults
        extern BASE_IMAGE_API void ConvertChannels(const ImageView& src, const ImageView& dest, const void* defaults = nullptr);

        ///----------------------------

        /// Swap/Replicate channel contents, operation is done in place
        extern BASE_IMAGE_API void CopyChannelsLine(PixelFormat format, void* mem, uint8_t numChannels, const uint8_t* mapping, uint32_t width, const void* defaults);

        /// Swap/Replicate channel contents within an image
        extern BASE_IMAGE_API void CopyChannels(const ImageView& dest, const uint8_t* mapping = nullptr, const void* defaults = nullptr);

        ///----------------------------

        /// Fill line with values
        extern BASE_IMAGE_API void FillLine(PixelFormat format, void* mem, uint8_t numChannels, uint32_t width, const void* defaults);

        /// Fill image content with a single value
        extern BASE_IMAGE_API void Fill(const ImageView& dest, const void* defaults = nullptr);

        ///----------------------------

        /// Copy single lines
        extern BASE_IMAGE_API void CopyLine(PixelFormat format, const void* srcMem, void* destMem, uint8_t numChannels, uint32_t width);

        /// Copy image content between views
        extern BASE_IMAGE_API void Copy(const ImageView& src, const ImageView& dest);

        ///----------------------------

        /// Flip image in X direction
        extern BASE_IMAGE_API void FlipX(const ImageView& dest);

        /// Flip image in Y direction
        extern BASE_IMAGE_API void FlipY(const ImageView& dest);

        ///----------------------------

        /// Unpack image into a floating point array
        extern BASE_IMAGE_API void UnpackIntoFloats(const ImageView& src, const ImageView& dest);

        /// Pack image into from floating point array
        extern BASE_IMAGE_API void PackFromFloats(const ImageView& src, const ImageView& dest);

        ///----------------------------

        /// Convert FLOAT32 image from a color space to linear data
        extern BASE_IMAGE_API void ConvertFloatsFromColorSpace(const ImageView& data, ColorSpace space);

        /// Convert FLOAT32 image float linear data to specific color space
        extern BASE_IMAGE_API void ConvertFloatsToColorSpace(const ImageView& data, ColorSpace space);

        ///----------------------------

        /// Merge slices [N,N+1] -> [N/2]
        extern BASE_IMAGE_API ImageView DownsampleZ(const ImageView& data, DownsampleMode mode);

        /// Merge rows [N,N+1] -> [N/2] in each slice
        extern BASE_IMAGE_API ImageView DownsampleY(const ImageView& data, DownsampleMode mode);

        /// Merge pixels [N,N+1] -> [N/2] in each row of each slice
        extern BASE_IMAGE_API ImageView DownsampleX(const ImageView& data, DownsampleMode mode);

        /// Downsample image, destination image view MUST be twice smaller: [max(1,ceil(width/2)), max(1,ceil(height/2)), max(1,ceil(depth/2))]
        /// NOTE: optional pixel mask may be used
        extern BASE_IMAGE_API void Downsample(const ImageView& src, const ImageView& dest, DownsampleMode mode, ColorSpace space);

        ///----------------------------

        /// Premultiply image color by it's alpha
        extern BASE_IMAGE_API void PremultiplyAlpha(const ImageView& data);

        ///----------------------------

    } // image
} // base

///--------------------------------------------------------------------------

#include "imageUtils.inl"

///--------------------------------------------------------------------------
