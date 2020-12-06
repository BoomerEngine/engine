/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: data #]
***/

#pragma once

namespace rendering
{
    //---

    enum class ImageFormat : uint8_t
    {
        UNKNOWN = 0,

        R8_SNORM,
        R8_UNORM,
        R8_UINT,
        R8_INT,

        RG8_SNORM,
        RG8_UNORM,
        RG8_UINT,
        RG8_INT,

        RGB8_SNORM,
        RGB8_UNORM,
        RGB8_UINT,
        RGB8_INT,

        RGBA8_SNORM,
        RGBA8_UNORM,
        RGBA8_UINT,
        RGBA8_INT,

        BGRA8_UNORM,

        //--- HDR formats

        R16F,
        RG16F,
        RGBA16F,

        R32F,
        RG32F,
        RGB32F,
        RGBA32F,

        //--- integer formats

        R16_INT,
        R16_UINT,
        R16_UNORM,
        R16_SNORM,

        RG16_INT,
        RG16_UINT,
        RG16_UNORM,
        RG16_SNORM, // -1 to 1

        RGBA16_INT,
        RGBA16_UINT,
        RGBA16_UNORM,
        RGBA16_SNORM,

        R32_INT,
        R32_UINT,

        RG32_INT,
        RG32_UINT,

        RGB32_INT,
        RGB32_UINT,

        RGBA32_INT,
        RGBA32_UINT,

        //--- matrix formats

        MAT22F,
        MAT32F,
        MAT33F,
        MAT42F,
        MAT43F,
        MAT44F,

        //--- packed formats

        R11FG11FB10F,
        RGB10_A2_UNORM,
        RGB10_A2_UINT,
        RGBA4_UNORM,

        //--- compressed formats

        BC1_UNORM, // DXT1, 4x4 blocks, 8 bytes block, 1-bit alpha, linear color space
        BC2_UNORM, // DXT3, 4x4 blocks, 16 bytes block, 4-bit alpha, linear color space
        BC3_UNORM, // DXT5, 4x4 blocks, 16 bytes block, 8-bit alpha, linear color space
        BC4_UNORM, // ATI1, 4x4 blocks, 8 bytes block, single channel, linear color space
        BC5_UNORM, // ATI2, 4x4 blocks, 16 bytes block, two channels, linear color space
        BC6_UNSIGNED, // 4x4 blocks, 16 bytes block, three channels, linear color space, unsigned values
        BC6_SIGNED, // 4x4 blocks, 16 bytes block, three channels, linear color space, signed values
        BC7_UNORM, // 4x4 blocks, 16 bytes block, three or four channels, linear color space

        //--- SRGB

        SRGB8,
        SRGBA8,
        BC1_SRGB, // DXT1, 4x4 blocks, 8 bytes block, 1-bit alpha, sRGB color space
        BC2_SRGB, // DXT3, 4x4 blocks, 16 bytes block, 4-bit alpha, sRGB color space
        BC3_SRGB, // DXT5, 4x4 blocks, 16 bytes block, 8-bit alpha, sRGB color space
        BC7_SRGB, // BC7 with sRGB content

        //--- depth formats

        D24S8,
        D24FS8,
        D32,

        MAX,
    };

    enum class ImageFormatClass
    {
        UNORM,
        SNORM,
        UINT,
        INT,
        FLOAT,
        DEPTH,
        SRGB,
    };

    /// image view type
    enum class ImageViewType : uint8_t
    {
        View1D, // one dimensional view, compatible only with 1D textures or a slice of 1D array
        View2D, // single 2D texture view, can be a single slice of array or face of a cube as well
        View3D, // three dimensional view, compatible only with 3D textures
        ViewCube, // a single cube map, requires 6 slices of 2D texture, texture must be cube-compatible
        View1DArray, // array of 1D textures
        View2DArray, // array of 2D textures
        ViewCubeArray, // array of cube maps
    };

    struct ImageFormatInfo
    {
        const char* name; // enum name
        const char* shaderName; // shader name, GLSL style also as parsed in CSL
        const uint32_t hash; // for permutations
        uint8_t numComponents;
        uint32_t bitsPerPixel; // BITS!
        bool compressed; // blocks
        bool packed; // non standard packing
        ImageFormatClass formatClass;
    };

    extern RENDERING_DEVICE_API const ImageFormatInfo& GetImageFormatInfo(ImageFormat format);
    extern RENDERING_DEVICE_API bool GetImageFormatByDisplayName(base::StringView name, ImageFormat& outFormat);
    extern RENDERING_DEVICE_API bool GetImageFormatByShaderName(base::StringView name, ImageFormat& outFormat);

	extern RENDERING_DEVICE_API bool IsFormatValidForView(ImageFormat format);
	extern RENDERING_DEVICE_API bool IsFormatValidForAtomic(ImageFormat format);

    //---

} // rendering

