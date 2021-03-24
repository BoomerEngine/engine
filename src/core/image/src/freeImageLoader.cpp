/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: loader #]
***/

#include "build.h"
#include "freeImageLoader.h"

#include "core/image/include/image.h"
#include "core/image/include/imageVIew.h"
#include "core/resource/include/resource.h"
#include "core/io/include/fileHandle.h"

#ifdef BUILD_AS_LIBS
    #define FREEIMAGE_LIB
#endif

#include <freeimage/FreeImage.h>

BEGIN_BOOMER_NAMESPACE()

#pragma optimize("", off)

namespace loader
{
    static unsigned DLL_CALLCONV ReadProc(void *buffer, unsigned size, unsigned count, fi_handle handle)
    {
        auto reader = (IReadFileHandle*) handle;

        auto totalRead = size * count;
        auto left = reader->size() - reader->pos();
        if (totalRead > left)
            totalRead = range_cast<uint32_t>(left);

        return (unsigned)reader->readSync(buffer, totalRead);
    }

    static unsigned DLL_CALLCONV WriteProc(void *buffer, unsigned size, unsigned count, fi_handle handle)
    {
        return 0;
    }

    static int DLL_CALLCONV SeekProc(fi_handle handle, long offset, int origin)
    {
        auto reader = (IReadFileHandle*) handle;
        if (origin == SEEK_END)
        {
            reader->pos(range_cast<uint64_t>((int64_t) reader->size() + (int64_t) offset));
        }
        else if (origin == SEEK_CUR)
        {
            reader->pos(range_cast<uint64_t>((int64_t) reader->pos() + (int64_t) offset));
        }
        else if (origin == SEEK_SET)
        {
            reader->pos(offset);
        }
        return 0;
    }

    static long DLL_CALLCONV TellProc(fi_handle handle)
    {
        auto reader = (IReadFileHandle*) handle;
        return range_cast<uint32_t>(reader->pos());
    }
} // loader

namespace memory
{
    struct MemoryState
    {
        const uint8_t* m_data = nullptr;
        uint32_t m_size = 0;
        uint32_t m_pos = 0;
    };

    static unsigned DLL_CALLCONV ReadProc(void* buffer, unsigned size, unsigned count, fi_handle handle)
    {
        auto reader  = (MemoryState*) handle;

        auto totalRead = size * count;
        auto left = reader->m_size - reader->m_pos;
        if (totalRead > left)
            totalRead = range_cast<uint32_t>(left);

        if (totalRead)
        {
            memcpy(buffer, reader->m_data + reader->m_pos, totalRead);
            reader->m_pos += totalRead;
        }

        return totalRead / size;
    }

    static unsigned DLL_CALLCONV WriteProc(void* buffer, unsigned size, unsigned count, fi_handle handle)
    {
        return 0;
    }

    static int DLL_CALLCONV SeekProc(fi_handle handle, long offset, int origin)
    {
        auto reader  = (MemoryState*)handle;

        if (origin == SEEK_END)
        {
            if (offset < 0)
                offset = 0;
            else if (offset > reader->m_size)
                offset = reader->m_size;

            reader->m_pos = reader->m_size - offset;
        }
        else if (origin == SEEK_CUR)
        {
            int64_t newPos = (int64_t)reader->m_pos + offset;

            if (newPos < 0)
                newPos = 0;
            else if (newPos > reader->m_size)
                newPos = reader->m_size;

            reader->m_pos = newPos;
        }
        else if (origin == SEEK_SET)
        {
            if (offset < 0)
                offset = 0;
            else if (offset > reader->m_size)
                offset = reader->m_size;

            reader->m_pos = offset;
        }
        return 0;
    }

    static long DLL_CALLCONV TellProc(fi_handle handle)
    {
        auto reader  = (MemoryState*)handle;
        return reader->m_pos;
    }
} // loader

namespace saver
{
    static unsigned STDCALL ReadProc(void *buffer, unsigned size, unsigned count, fi_handle handle)
    {
        return 0;
    }

    static unsigned STDCALL WriteProc(void *buffer, unsigned size, unsigned count, fi_handle handle)
    {
        auto writer = (IWriteFileHandle*) handle;
        return (uint32_t)writer->writeSync(buffer, size * count);
    }

    static int STDCALL SeekProc(fi_handle handle, long offset, int origin)
    {
        auto writer = (IWriteFileHandle*) handle;
        if (origin == SEEK_END)
        {
            writer->pos(range_cast<uint64_t>((int64_t) writer->size() + (int64_t) offset));
        }
        else if (origin == SEEK_CUR)
        {
            writer->pos(range_cast<uint64_t>((int64_t) writer->pos() + (int64_t) offset));
        }
        else if (origin == SEEK_SET)
        {
            writer->pos(offset);
        }
        return 0;
    }

    static long STDCALL TellProc(fi_handle handle)
    {
        auto writer = (IWriteFileHandle*) handle;
        return range_cast<uint32_t>(writer->pos());
    }
} // saver

static FREE_IMAGE_FORMAT GetFormat(const char* typeHint)
{
    if (0 == strcmp(typeHint, "bmp"))
        return FIF_BMP;
    else if (0 == strcmp(typeHint, "dds"))
        return FIF_DDS;
    else if (0 == strcmp(typeHint, "jpg") || 0 == strcmp(typeHint, "jpeg"))
        return FIF_JPEG;
    else if (0 == strcmp(typeHint, "jp2") || 0 == strcmp(typeHint, "jpx"))
        return FIF_JP2;
    else if (0 == strcmp(typeHint, "tga"))
        return FIF_TARGA;
    else if (0 == strcmp(typeHint, "tiff") || 0 == strcmp(typeHint, "tif"))
        return FIF_TIFF;
    else if (0 == strcmp(typeHint, "hdr"))
        return FIF_HDR;
    else if (0 == strcmp(typeHint, "exr"))
        return FIF_EXR;
    else if (0 == strcmp(typeHint, "ppm"))
        return FIF_PPM;
    else if (0 == strcmp(typeHint, "pgm"))
        return FIF_PGM;
    else if (0 == strcmp(typeHint, "pbm"))
        return FIF_PBM;
    else if (0 == strcmp(typeHint, "png"))
        return FIF_PNG;
    else if (0 == strcmp(typeHint, "psd"))
        return FIF_PSD;
    else if (0 == strcmp(typeHint, "xbm"))
        return FIF_XBM;
    else if (0 == strcmp(typeHint, "xpm"))
        return FIF_XPM;
    else if (0 == strcmp(typeHint, "ico"))
        return FIF_ICO;
    else if (0 == strcmp(typeHint, "gif"))
        return FIF_GIF;
    else if (0 == strcmp(typeHint, "webp"))
        return FIF_WEBP;

    return FIF_UNKNOWN;
}

static FREE_IMAGE_FORMAT GetFormat(IReadFileHandle& file, const char* typeHint)
{
    // try to get format with the type hint
    auto format = GetFormat(typeHint);
    if (format != FIF_UNKNOWN)
        return format;

    // set the IO
    FreeImageIO io;
    io.read_proc = loader::ReadProc;
    io.write_proc = loader::WriteProc;
    io.tell_proc = loader::TellProc;
    io.seek_proc = loader::SeekProc;

    // read
    return FreeImage_GetFileTypeFromHandle(&io, (fi_handle)&file, (int)file.size());
}

static FREE_IMAGE_FORMAT GetFormat(memory::MemoryState& buf, const char* typeHint)
{
    // set the IO
    FreeImageIO io;
    io.read_proc = memory::ReadProc;
    io.write_proc = memory::WriteProc;
    io.tell_proc = memory::TellProc;
    io.seek_proc = memory::SeekProc;

    // read
    auto format = FreeImage_GetFileTypeFromHandle(&io, (fi_handle)&buf, (int)buf.m_size);
    if (format != FIF_UNKNOWN)
        return format;

    // try to get format with the type hint
    return GetFormat(typeHint);
}

static const char* GetTypeName(FREE_IMAGE_TYPE type)
{
    switch (type)
    {
        case FIT_BITMAP:
            return "BITMAP";
        case FIT_UINT16:
            return "UINT16";
        case FIT_INT16:
            return "INT16";
        case FIT_UINT32:
            return "UINT32";
        case FIT_INT32:
            return "INT32";
        case FIT_FLOAT:
            return "FLOAT";
        case FIT_DOUBLE:
            return "DOUBLE";
        case FIT_COMPLEX:
            return "COMPLEX";
        case FIT_RGB16:
            return "RGB16";
        case FIT_RGBA16:
            return "RGBA16";
        case FIT_RGBF:
            return "RGBF";
        case FIT_RGBAF:
            return "RGBFA";
    }

    return "UNKNONW";
}

static const char* GetColorTypeName(FREE_IMAGE_COLOR_TYPE type)
{
    switch (type)
    {
        case FIC_MINISWHITE:
            return "MINISWHITE";
        case FIC_MINISBLACK :
            return "MINISBLACK";
        case FIC_RGB:
            return "RGB";
        case FIC_PALETTE:
            return "PALETTE";
        case FIC_RGBALPHA:
            return "FIC_RGBALPHA";
        case FIC_CMYK:
            return "CMYK";
    }

    return "UNKNONW";
}

static void SwapRedBlue(FIBITMAP* bitmap)
{
    auto width = FreeImage_GetWidth(bitmap);
    auto height = FreeImage_GetHeight(bitmap);
    auto pitch = FreeImage_GetPitch(bitmap);
    auto bpp = FreeImage_GetBPP(bitmap);
    auto type = FreeImage_GetImageType(bitmap);

    for (uint32_t i=0; i<height; ++i)
    {
        if (type == FIT_BITMAP && bpp == 24)
        {
            auto ptr  = (uint8_t*)FreeImage_GetScanLine(bitmap, i);
            for (uint32_t x = 0; x < width; x++, ptr += 3)
                std::swap(ptr[0], ptr[2]);
        }
        else if (type == FIT_BITMAP && bpp == 32)
        {
            auto ptr  = (uint8_t*)FreeImage_GetScanLine(bitmap, i);
            for (uint32_t x = 0; x < width; x++, ptr += 4)
                std::swap(ptr[0], ptr[2]);
        }
        else if (type == FIT_RGB16)
        {
            auto ptr  = (uint16_t*)FreeImage_GetScanLine(bitmap, i);
            for (uint32_t x = 0; x < width; x++, ptr += 3)
                std::swap(ptr[0], ptr[2]);
        }
        else if (type == FIT_RGBA16)
        {
            auto ptr  = (uint16_t*)FreeImage_GetScanLine(bitmap, i);
            for (uint32_t x = 0; x < width; x++, ptr += 4)
                std::swap(ptr[0], ptr[2]);
        }
        else if (type == FIT_RGBF)
        {
            auto ptr  = (float*)FreeImage_GetScanLine(bitmap, i);
            for (uint32_t x = 0; x < width; x++, ptr += 3)
                std::swap(ptr[0], ptr[2]);
        }
        else if (type == FIT_RGBAF)
        {
            auto ptr  = (float*)FreeImage_GetScanLine(bitmap, i);
            for (uint32_t x = 0; x < width; x++, ptr += 4)
                std::swap(ptr[0], ptr[2]);
        }
    }
}

static bool ConvertFormat(FIBITMAP*& bitmap, ImagePixelFormat& outFormat, uint8_t& outNumChannels)
{
    auto width = FreeImage_GetWidth(bitmap);
    auto height = FreeImage_GetHeight(bitmap);
    auto bpp = FreeImage_GetBPP(bitmap);
    auto type = FreeImage_GetImageType(bitmap);
    auto color = FreeImage_GetColorType(bitmap);
    TRACE_SPAM("Loaded image {}x{}, bits: {}, type: {}, color: {}", width, height, bpp, GetTypeName(type), GetColorTypeName(color));

    switch (type)
    {
        case FIT_BITMAP:
        {
            // promote to 8-bits per pixel
            if (bpp < 8)
            {
                auto newBitmap = FreeImage_ConvertToGreyscale(bitmap);
                if (!newBitmap)
                    return false;

                bpp = FreeImage_GetBPP(newBitmap);
                DEBUG_CHECK_EX(bpp == 8, "Image not grayscale after conversion");

                if (bitmap != newBitmap)
                {
                    FreeImage_Unload(bitmap);
                    bitmap = newBitmap;
                }
            }

            // use the data directly
            outNumChannels = bpp / 8;
            outFormat = ImagePixelFormat::Uint8_Norm;
            return true;
        }

        case FIT_FLOAT:
        {
            outNumChannels = 1;
            outFormat = ImagePixelFormat::Float32_Raw;
            return true;
        }

        case FIT_UINT16:
        {
            outNumChannels = bpp / 16;
            outFormat = ImagePixelFormat::Uint16_Norm;
            return true;
        }

        case FIT_RGBF:
        {
            outNumChannels = 3;
            outFormat = ImagePixelFormat::Float32_Raw;
            return true;
        }

        case FIT_RGB16:
        {
            outNumChannels = 3;
            outFormat = ImagePixelFormat::Uint16_Norm;
            return true;
        }

        case FIT_RGBA16:
        {
            outNumChannels = 4;
            outFormat = ImagePixelFormat::Uint16_Norm;
            return true;
        }
    }

    return false;
}

static bool FormatSupported(FIBITMAP* bitmap)
{
    auto bpp = FreeImage_GetBPP(bitmap);
    auto type = FreeImage_GetImageType(bitmap);

    switch (type)
    {
    case FIT_FLOAT:
    case FIT_BITMAP:
    case FIT_UINT16:
    case FIT_RGBF:
    case FIT_RGB16:
    case FIT_RGBA16:
        return true;
    }

    return false;
}

//---

FreeImageLoadedData::FreeImageLoadedData(void* object_)
{
    auto dib = (FIBITMAP*)object_;

    ConvertFormat(dib, format, channels);

    if (format == ImagePixelFormat::Uint8_Norm)
        SwapRedBlue(dib);

    data = FreeImage_GetBits(dib);
    width = FreeImage_GetWidth(dib);
    height = FreeImage_GetHeight(dib);
    rowPitch = FreeImage_GetPitch(dib);
    pixelPitch = rowPitch / width;

    object = dib;
}

FreeImageLoadedData::~FreeImageLoadedData()
{
    if (object)
    {
        FreeImage_Unload((FIBITMAP*)object);
        object = nullptr;
    }
}

static void FreeImageOutput(FREE_IMAGE_FORMAT fif, const char* msg)
{
    const char* name = FreeImage_GetFormatFromFIF(fif);
    TRACE_WARNING("FreeImage: {}: {}", name, msg);
}

static void InitFreeImage()
{
    static bool initialized = false;
    if (!initialized)
    {
        FreeImage_Initialise(true);
        FreeImage_SetOutputMessage(&FreeImageOutput);
        initialized = true;
    }
}

RefPtr<FreeImageLoadedData> LoadImageWithFreeImage(const void* data, uint64_t dataSize)
{
    InitFreeImage();

    // get the extension of the file we are importing from
    memory::MemoryState memoryState;
    memoryState.m_pos = 0;
    memoryState.m_size = dataSize;
    memoryState.m_data = (const uint8_t*)data;

    // match format
    auto format = GetFormat(memoryState, "");
    if (format == FIF_UNKNOWN)
        return nullptr;

    // set the IO
    FreeImageIO io;
    io.read_proc = memory::ReadProc;
    io.write_proc = memory::WriteProc;
    io.tell_proc = memory::TellProc;
    io.seek_proc = memory::SeekProc;

    // load the image
    memoryState.m_pos = 0;
    auto* dib = FreeImage_LoadFromHandle(format, &io, (fi_handle)&memoryState);
    if (!dib)
        return nullptr;

    // format is not supported ?
    if (!FormatSupported(dib))
    {
        FreeImage_Unload(dib);
        return nullptr;
    }

    // flip the image
    FreeImage_FlipVertical(dib);

    // create a wrapper
    return RefNew<FreeImageLoadedData>(dib);
}

ImageView FreeImageLoadedData::view() const
{
    return ImageView(format, channels, data, width, height, pixelPitch, rowPitch);
}

//---

/*// FreeImage saver
class SaverFreeImage : public ISaver
{
    RTTI_DECLARE_VIRTUAL_CLASS(SaverFreeImage, ISaver);

public:
    SaverFreeImage()
    {}

    virtual bool save(IFileHandle& file, const Image& img, const char* typeHint)override final
    {
        // match format
        auto format = GetFormat(typeHint);
        if (format == FIF_UNKNOWN)
            return false;

        // extract data
        BitmapPtr dib;
        switch (img.pixelFormat())
        {
            case ImagePixelFormat::Uint8_Norm:
            {
                if (img.size().m_channels == 1)
                {
                    dib = FreeImage_ConvertFromRawBits((BYTE*)img.pixelBuffer(), img.size().width, img.size().height, img.layout().m_rowPitch, 8, 0, 0, 0, TRUE);
                }
                else if (img.size().m_channels == 2)
                {
                    dib = FreeImage_ConvertFromRawBits((BYTE*)img.pixelBuffer(), img.size().width, img.size().height, img.layout().m_rowPitch, 16, 0, 0, 0, TRUE);
                }
                else if (img.size().m_channels == 3)
                {
                    dib = FreeImage_ConvertFromRawBits((BYTE*)img.pixelBuffer(), img.size().width, img.size().height, img.layout().m_rowPitch, 24, 0, 0, 0, TRUE);
                }
                else if (img.size().m_channels == 4)
                {
                    dib = FreeImage_ConvertFromRawBits((BYTE*)img.pixelBuffer(), img.size().width, img.size().height, img.layout().m_rowPitch, 32, 0, 0, 0, TRUE);
                }
                break;
            }
        }

        // no image to save ?
        if (dib.empty())
        {
            TRACE_ERROR("Image has incompatible format for saving");
            return false;
        }

        // swap channels
        SwapRedBlue(dib);

        // set the IO
        FreeImageIO io;
        io.read_proc = (FI_ReadProc)&saver::ReadProc;
        io.write_proc = (FI_WriteProc)&saver::WriteProc;
        io.tell_proc = (FI_TellProc)&saver::TellProc;
        io.seek_proc = (FI_SeekProc)&saver::SeekProc;

        // save the image
        if (!FreeImage_SaveToHandle(format, dib, &io, (fi_handle)&file, 0))
        {
            TRACE_ERROR("Failed to save imaged");
            return false;
        }

        // image saved
        return true;
    }
};

RTTI_BEGIN_TYPE_CLASS(SaverFreeImage);
RTTI_END_TYPE();*/

//---

END_BOOMER_NAMESPACE()
