/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_image_glue.inl"

BEGIN_BOOMER_NAMESPACE()

//--

// data format of single channel in the pixel
enum class ImagePixelFormat : uint8_t
{
    Uint8_Norm, // normalized to 0-1 range
    Uint16_Norm, // normalized to 0-1 range
    Float16_Raw, // raw format
    Float32_Raw, // raw format
};

/// color space, used by various filters that try to be a little bit more exact when filtering colors
/// it's a good practice not to ignore this
enum class ImageColorSpace : uint8_t
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

/// downsampling mode, especially how are the values averaged together
enum class ImageDownsampleMode : uint8_t
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

class Image;
typedef RefPtr<Image> ImagePtr;
typedef ResourceRef<Image> ImageRef;

class ImageView;

class ImageAtlas;

struct ImageRect;

//--

// load image from file memory
extern CORE_IMAGE_API ImagePtr LoadImageFromMemory(const void* memory, uint64_t size);

// load image from file memory
extern CORE_IMAGE_API ImagePtr LoadImageFromMemory(Buffer ptr);

// load image from file
extern CORE_IMAGE_API ImagePtr LoadImageFromFile(IReadFileHandle* file);

// load image from absolute file
extern CORE_IMAGE_API ImagePtr LoadImageFromAbsolutePath(StringView absolutePath);

// load image from absolute file
extern CORE_IMAGE_API ImagePtr LoadImageFromDepotPath(StringView depotPath);

//--

END_BOOMER_NAMESPACE()
