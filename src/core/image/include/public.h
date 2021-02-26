/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_image_glue.inl"

BEGIN_BOOMER_NAMESPACE_EX(image)

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

END_BOOMER_NAMESPACE_EX(image)

//--

BEGIN_BOOMER_NAMESPACE()

// load image from file memory
extern CORE_IMAGE_API image::ImagePtr LoadImageFromMemory(const void* memory, uint64_t size);

// load image from file memory
extern CORE_IMAGE_API image::ImagePtr LoadImageFromMemory(Buffer ptr);

// load image from file
extern CORE_IMAGE_API image::ImagePtr LoadImageFromFile(io::IReadFileHandle* file);

// load image from absolute file
extern CORE_IMAGE_API image::ImagePtr LoadImageFromAbsolutePath(StringView absolutePath);

// load image from absolute file
extern CORE_IMAGE_API image::ImagePtr LoadImageFromDepotPath(StringView depotPath);

END_BOOMER_NAMESPACE()

//--