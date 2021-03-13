/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image #]
***/

#include "build.h"
#pragma hdrstop

#include "imageUtils.h"
#include "imageView.h"
#include "image.h"

BEGIN_BOOMER_NAMESPACE_EX(image)

//--

template< typename T >
static void ConvertChannelsLineWorker(const T* readPtr, uint8_t srcChannelCount, T* writePtr, uint8_t destChannelCount, uint32_t width, const T* defaultPtr)
{
    // do we have channels to fill with defaults ?
    if (srcChannelCount < destChannelCount)
    {
        auto missingChannelCount = destChannelCount - srcChannelCount;
        auto missingChannelOffset = srcChannelCount;

        auto defaultData  = defaultPtr + missingChannelOffset;
        auto lineWritePtr  = writePtr + missingChannelOffset;
        auto lineWriteEndPtr  = lineWritePtr + (width * destChannelCount);

        if (missingChannelCount == 1)
        {
            while (lineWritePtr < lineWriteEndPtr)
            {
                lineWritePtr[0] = defaultData[0];
                lineWritePtr += destChannelCount;
            }
        }
        else if (missingChannelCount == 2)
        {
            while (lineWritePtr < lineWriteEndPtr)
            {
                lineWritePtr[0] = defaultData[0];
                lineWritePtr[1] = defaultData[1];
                lineWritePtr += destChannelCount;
            }
        }
        else if (missingChannelCount == 3)
        {
            while (lineWritePtr < lineWriteEndPtr)
            {
                lineWritePtr[0] = defaultData[0];
                lineWritePtr[1] = defaultData[1];
                lineWritePtr[2] = defaultData[2];
                lineWritePtr += destChannelCount;
            }
        }
        else if (missingChannelCount == 4)
        {
            while (lineWritePtr < lineWriteEndPtr)
            {
                lineWritePtr[0] = defaultData[0];
                lineWritePtr[1] = defaultData[1];
                lineWritePtr[2] = defaultData[2];
                lineWritePtr[3] = defaultData[3];
                lineWritePtr += destChannelCount;
            }
        }
    }

    // copy existing data
    auto copiableChannelCount = std::min(destChannelCount, srcChannelCount);
    if (copiableChannelCount == 1)
    {
        for (uint32_t i = 0; i < width; ++i)
        {
            writePtr[0] = readPtr[0];

            readPtr += srcChannelCount;
            writePtr += destChannelCount;
        }
    }
    else if (copiableChannelCount == 2)
    {
        for (uint32_t i = 0; i < width; ++i)
        {
            writePtr[0] = readPtr[0];
            writePtr[1] = readPtr[1];

            readPtr += srcChannelCount;
            writePtr += destChannelCount;
        }
    }
    else if (copiableChannelCount == 3)
    {
        for (uint32_t i = 0; i < width; ++i)
        {
            writePtr[0] = readPtr[0];
            writePtr[1] = readPtr[1];
            writePtr[2] = readPtr[2];

            readPtr += srcChannelCount;
            writePtr += destChannelCount;
        }
    }
    else if (copiableChannelCount == 4)
    {
        for (uint32_t i = 0; i < width; ++i)
        {
            writePtr[0] = readPtr[0];
            writePtr[1] = readPtr[1];
            writePtr[2] = readPtr[2];
            writePtr[3] = readPtr[3];

            readPtr += srcChannelCount;
            writePtr += destChannelCount;
        }
    }
}

static uint8_t COLOR_DEFAULTS_UINT8[4] = { 0, 0, 0, 255 }; // is this a hack ?
static uint16_t COLOR_DEFAULTS_UINT16[4] = { 0, 0, 0, 65535 };
static uint16_t COLOR_DEFAULTS_FLOAT16[4] = { 0, 0, 0, 0 }; // TODO: 1
static uint32_t COLOR_DEFAULTS_FLOAT32[4] = { 0, 0, 0, 0 }; // TODO: 1

void ConvertChannelsLine(PixelFormat format, const void* srcMem, uint8_t srcChannelCount, void* destMem, uint8_t destChannelCount, uint32_t width, const void* defaults)
{
    if (format == PixelFormat::Uint8_Norm)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_UINT8;

        ConvertChannelsLineWorker<uint8_t>((const uint8_t*)srcMem, srcChannelCount, (uint8_t*)destMem, destChannelCount, width, (const uint8_t*)defaults);
    }
    else if (format == PixelFormat::Uint16_Norm)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_UINT16;

        ConvertChannelsLineWorker<uint16_t>((const uint16_t*)srcMem, srcChannelCount, (uint16_t*)destMem, destChannelCount, width, (const uint16_t*)defaults);
    }
    else if (format == PixelFormat::Float16_Raw)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_FLOAT16;

        ConvertChannelsLineWorker<uint16_t>((const uint16_t*)srcMem, srcChannelCount, (uint16_t*)destMem, destChannelCount, width, (const uint16_t*)defaults);
    }
    else if (format == PixelFormat::Float32_Raw)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_FLOAT32;

        ConvertChannelsLineWorker<uint32_t>((const uint32_t*)srcMem, srcChannelCount, (uint32_t*)destMem, destChannelCount, width, (const uint32_t*)defaults);
    }
}

void ConvertChannels(const ImageView& src, const ImageView& dest, const void* defaults /*= nullptr*/)
{
    PC_SCOPE_LVL2(ConvertChannels);

    DEBUG_CHECK(src.format() == dest.format());
    DEBUG_CHECK(src.width() == dest.width());
    DEBUG_CHECK(src.height() == dest.height());
    DEBUG_CHECK(src.depth() == dest.depth());

    auto readPtr  = (const uint8_t*)src.data();
    auto writePtr  = (uint8_t*)dest.data();

    for (uint32_t z = 0; z < dest.depth(); ++z)
    {
        auto sliceReadPtr  = readPtr;
        auto sliceWritePtr  = writePtr;

        for (uint32_t y = 0; y < dest.height(); ++y)
        {
            ConvertChannelsLine(src.format(), readPtr, src.channels(), writePtr, dest.channels(), dest.width(), defaults);
            readPtr += src.rowPitch();
            writePtr += dest.rowPitch();
        }

        readPtr = sliceReadPtr + src.slicePitch();
        writePtr = sliceWritePtr + dest.slicePitch();
    }
}

ImagePtr ConvertChannels(const ImageView& src, uint8_t destChannelCount, const void* defaults /*= nullptr*/)
{
	auto ret = RefNew<Image>(src.format(), destChannelCount, src.width(), src.height(), src.depth());
	ConvertChannels(src, ret->view(), defaults);
	return ret;
}

//---

template< typename T >
static void CopyChannelsLineWorker(T* mem, uint8_t numChannels, uint32_t width, const uint8_t* mapping, T* tempWithDefaults)
{
    if (numChannels == 1)
    {
        auto endPtr  = mem + width;
        while (mem < endPtr)
        {
            tempWithDefaults[0] = mem[0];
            mem[0] = tempWithDefaults[mapping[0]];
            mem += 1;
        }
    }
    else if (numChannels == 2)
    {
        auto endPtr  = mem + width;
        while (mem < endPtr)
        {
            tempWithDefaults[0] = mem[0];
            tempWithDefaults[1] = mem[1];
            mem[0] = tempWithDefaults[mapping[0]];
            mem[1] = tempWithDefaults[mapping[1]];
            mem += 2;
        }
    }
    else if (numChannels == 3)
    {
        auto endPtr  = mem + width;
        while (mem < endPtr)
        {
            tempWithDefaults[0] = mem[0];
            tempWithDefaults[1] = mem[1];
            tempWithDefaults[2] = mem[2];
            mem[0] = tempWithDefaults[mapping[0]];
            mem[1] = tempWithDefaults[mapping[1]];
            mem[2] = tempWithDefaults[mapping[2]];
            mem += 3;
        }
    }
    else if (numChannels == 4)
    {
        auto endPtr  = mem + width;
        while (mem < endPtr)
        {
            tempWithDefaults[0] = mem[0];
            tempWithDefaults[1] = mem[1];
            tempWithDefaults[2] = mem[2];
            tempWithDefaults[3] = mem[3];
            mem[0] = tempWithDefaults[mapping[0]];
            mem[1] = tempWithDefaults[mapping[1]];
            mem[2] = tempWithDefaults[mapping[2]];
            mem[3] = tempWithDefaults[mapping[3]];
            mem += 4;
        }
    }
}

template< typename T >
static void CopyChannelsLineWorker2(T* mem, uint8_t numChannels, uint32_t width, const uint8_t* mapping, const T* defaults)
{
    T temp[8];

    for (uint32_t i = 0; i < numChannels; ++i)
        temp[numChannels + i] = defaults[i];

    CopyChannelsLineWorker(mem, numChannels, width, mapping, temp);
}

void CopyChannelsLine(PixelFormat format, void* mem, uint8_t numChannels, const uint8_t* mapping, uint32_t width, const void* defaults)
{
    if (format == PixelFormat::Uint8_Norm)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_UINT8;

        CopyChannelsLineWorker2<uint8_t>((uint8_t*)mem, numChannels, width, mapping, (const uint8_t*)defaults);
    }
    else if (format == PixelFormat::Uint16_Norm)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_UINT16;

        CopyChannelsLineWorker2<uint16_t>((uint16_t*)mem, numChannels, width, mapping, (const uint16_t*)defaults);
    }
    else if (format == PixelFormat::Float16_Raw)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_FLOAT16;

        CopyChannelsLineWorker2<uint16_t>((uint16_t*)mem, numChannels, width, mapping, (const uint16_t*)defaults);
    }
    else if (format == PixelFormat::Float32_Raw)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_FLOAT32;

        CopyChannelsLineWorker2<uint32_t>((uint32_t*)mem, numChannels, width, mapping, (const uint32_t*)defaults);
    }
}

static const uint8_t DEFAULT_MAPPING[4] = { 0,1,2,3 };

void CopyChannels(const ImageView& dest, const uint8_t* mapping /*= nullptr*/, const void* defaults /*= nullptr*/)
{
    PC_SCOPE_LVL2(CopyChannels);

    if (!mapping)
        mapping = DEFAULT_MAPPING;

    auto writePtr  = (uint8_t*)dest.data();
    for (uint32_t z = 0; z < dest.depth(); ++z)
    {
        auto sliceWritePtr  = writePtr;

        for (uint32_t y = 0; y < dest.height(); ++y)
        {
            CopyChannelsLine(dest.format(), writePtr, dest.channels(), mapping, dest.width(), defaults);
            writePtr += dest.rowPitch();
        }

        writePtr = sliceWritePtr + dest.slicePitch();
    }
}

//---

template< typename T >
static void FillLineWorker(T* mem, uint8_t numChannels, uint32_t width, const T* defaults)
{
    if (numChannels == 1)
    {
        auto endPtr  = mem + width;
        while (mem < endPtr)
        {
            mem[0] = defaults[0];
            mem += 1;
        }
    }
    else if (numChannels == 2)
    {
        auto endPtr  = mem + 2*width;
        while (mem < endPtr)
        {
            mem[0] = defaults[0];
            mem[1] = defaults[1];
            mem += 2;
        }
    }
    else if (numChannels == 3)
    {
        auto endPtr  = mem + 3 * width;
        while (mem < endPtr)
        {
            mem[0] = defaults[0];
            mem[1] = defaults[1];
            mem[2] = defaults[2];
            mem += 3;
        }
    }
    else if (numChannels == 4)
    {
        auto endPtr  = mem + 4*width;
        while (mem < endPtr)
        {
            mem[0] = defaults[0];
            mem[1] = defaults[1];
            mem[2] = defaults[2];
            mem[3] = defaults[3];
            mem += 4;
        }
    }
}

void FillLine(PixelFormat format, void* mem, uint8_t numChannels, uint32_t width, const void* defaults)
{
    if (format == PixelFormat::Uint8_Norm)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_UINT8;

        FillLineWorker<uint8_t>((uint8_t*)mem, numChannels, width, (const uint8_t*)defaults);
    }
    else if (format == PixelFormat::Uint16_Norm)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_UINT16;

        FillLineWorker<uint16_t>((uint16_t*)mem, numChannels, width, (const uint16_t*)defaults);
    }
    else if (format == PixelFormat::Float16_Raw)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_FLOAT16;

        FillLineWorker<uint16_t>((uint16_t*)mem, numChannels, width, (const uint16_t*)defaults);
    }
    else if (format == PixelFormat::Float32_Raw)
    {
        if (!defaults)
            defaults = COLOR_DEFAULTS_FLOAT32;

        FillLineWorker<uint32_t>((uint32_t*)mem, numChannels, width, (const uint32_t*)defaults);
    }
}

void Fill(const ImageView& dest, const void* defaults /*= nullptr*/)
{
    PC_SCOPE_LVL2(ImageFill);

    auto writePtr  = (uint8_t*)dest.data();
    for (uint32_t z = 0; z < dest.depth(); ++z)
    {
        auto sliceWritePtr  = writePtr;

        for (uint32_t y = 0; y < dest.height(); ++y)
        {
            FillLine(dest.format(), writePtr, dest.channels(), dest.width(), defaults);
            writePtr += dest.rowPitch();
        }

        writePtr = sliceWritePtr + dest.slicePitch();
    }
}

//---

void CopyLine(PixelFormat format, const void* srcMem, void* destMem, uint8_t numChannels, uint32_t width)
{
    uint32_t dataSize = 0;
    switch (format)
    {
        case PixelFormat::Uint8_Norm: dataSize = numChannels * width; break;
        case PixelFormat::Uint16_Norm: dataSize = 2 * numChannels * width; break;
        case PixelFormat::Float16_Raw: dataSize = 2 * numChannels * width; break;
        case PixelFormat::Float32_Raw: dataSize = 4 * numChannels * width; break;
    }

    memcpy(destMem, srcMem, dataSize);
}

void Copy(const ImageView& src, const ImageView& dest)
{
    PC_SCOPE_LVL2(ImageCopy);

    DEBUG_CHECK(src.format() == dest.format());
    DEBUG_CHECK(src.channels() == dest.channels());
    DEBUG_CHECK(src.width() == dest.width());
    DEBUG_CHECK(src.height() == dest.height());
    DEBUG_CHECK(src.depth() == dest.depth());

    uint32_t dataSize = 0;
    switch (dest.format())
    {
        case PixelFormat::Uint8_Norm: dataSize = dest.channels() * dest.width(); break;
        case PixelFormat::Uint16_Norm: dataSize = 2 * dest.channels() * dest.width(); break;
        case PixelFormat::Float16_Raw: dataSize = 2 * dest.channels() * dest.width(); break;
        case PixelFormat::Float32_Raw: dataSize = 4 * dest.channels() * dest.width(); break;
    }

    auto readPtr  = (const uint8_t*)src.data();
    auto writePtr  = (uint8_t*)dest.data();

    for (uint32_t z = 0; z < dest.depth(); ++z)
    {
        auto sliceReadPtr  = readPtr;
        auto sliceWritePtr  = writePtr;

        for (uint32_t y = 0; y < dest.height(); ++y)
        {
            memcpy(writePtr, readPtr, dataSize);

            readPtr += src.rowPitch();
            writePtr += dest.rowPitch();
        }

        readPtr = sliceReadPtr + src.slicePitch();
        writePtr = sliceWritePtr + dest.slicePitch();
    }
}

//---


template< typename T >
static void FlipXLine(T* mem, uint8_t numChannels, uint32_t width)
{
    auto* memEnd = mem + (numChannels * (width - 1));

    if (numChannels == 1)
    {
        while (mem < memEnd)
        {
            std::swap(mem[0], memEnd[0]);
            memEnd -= 1;
            mem += 1;
        }
    }
    else if (numChannels == 2)
    {
        while (mem < memEnd)
        {
            std::swap(mem[0], memEnd[0]);
            std::swap(mem[1], memEnd[1]);
            memEnd -= 2;
            mem += 2;
        }
    }
    else if (numChannels == 3)
    {
        while (mem < memEnd)
        {
            std::swap(mem[0], memEnd[0]);
            std::swap(mem[1], memEnd[1]);
            std::swap(mem[2], memEnd[2]);
            memEnd -= 3;
            mem += 3;
        }
    }
    else if (numChannels == 4)
    {
        while (mem < memEnd)
        {
            std::swap(mem[0], memEnd[0]);
            std::swap(mem[1], memEnd[1]);
            std::swap(mem[2], memEnd[2]);
            std::swap(mem[3], memEnd[3]);
            memEnd -= 4;
            mem += 4;
        }
    }
}

void FlipXLine(PixelFormat format, void* mem, uint8_t numChannels, uint32_t width)
{
    if (format == PixelFormat::Uint8_Norm)
        FlipXLine<uint8_t>((uint8_t*)mem,  numChannels, width);
    else if (format == PixelFormat::Uint16_Norm)
        FlipXLine<uint16_t>((uint16_t*)mem, numChannels, width);
    else if (format == PixelFormat::Float16_Raw)
        FlipXLine<uint16_t>((uint16_t*)mem, numChannels, width);
    else if (format == PixelFormat::Float32_Raw)
        FlipXLine<uint32_t>((uint32_t*)mem, numChannels, width);
}

void FlipX(const ImageView& dest)
{
    PC_SCOPE_LVL2(FlipX);

    auto writePtr = (uint8_t*)dest.data();
    for (uint32_t z = 0; z < dest.depth(); ++z)
    {
        auto sliceWritePtr = writePtr;

        for (uint32_t y = 0; y < dest.height(); ++y)
        {
            FlipXLine(dest.format(), writePtr, dest.channels(), dest.width());
            writePtr += dest.rowPitch();
        }

        writePtr = sliceWritePtr + dest.slicePitch();
    }
}

///----------------------------

void FlipY(const ImageView& dest)
{
    PC_SCOPE_LVL2(FlipY);

    auto writePtr = (uint8_t*)dest.data();
    for (uint32_t z = 0; z < dest.depth(); ++z)
    {
        auto sliceWritePtr = writePtr;
        auto sliceEndWritePtr = writePtr + dest.rowPitch() * (dest.height() - 1);

        while (writePtr < sliceEndWritePtr)
        {
            auto* rowAPtr = writePtr;
            auto* rowAEndPtr = writePtr + dest.rowPitch();
            auto* rowBPtr = sliceEndWritePtr;

            while (rowAPtr < rowAEndPtr)
                std::swap(*rowAPtr++, *rowBPtr++);

            writePtr += dest.rowPitch();
            sliceEndWritePtr -= dest.rowPitch();
        }

        writePtr = sliceWritePtr + dest.slicePitch();
    }
}

///----------------------------

void SwapXY(const ImageView& dest)
{
    PC_SCOPE_LVL2(SwapXY);

    DEBUG_CHECK_RETURN_EX(dest.width() == dest.height(), "Only square images can be XY flipped");

    auto writePtr = (uint8_t*)dest.data();
    for (uint32_t z = 0; z < dest.depth(); ++z)
    {
        auto sliceWritePtr = writePtr;
        auto sliceEndWritePtr = writePtr + dest.rowPitch() * (dest.height() - 1);

        uint32_t rowIndex = 0;

        while (writePtr < sliceEndWritePtr)
        {
            auto* colPtr = sliceWritePtr + (rowIndex * dest.pixelPitch());

            auto* rowPtr = writePtr;
            auto* rowEndPtr = rowPtr + dest.rowPitch();

            // skip lower part (only half of the triangle has to be swapped)
            rowPtr += dest.pixelPitch() * (rowIndex + 1);
            colPtr += dest.rowPitch() * (rowIndex + 1);

            while (rowPtr < rowEndPtr)
            {
                for (uint32_t i = 0; i < dest.pixelPitch(); ++i)
                    std::swap(rowPtr[i], colPtr[i]);

                rowPtr += dest.pixelPitch();
                colPtr += dest.rowPitch();
            }

            writePtr += dest.rowPitch();
            rowIndex += 1;
        }

        writePtr = sliceWritePtr + dest.slicePitch();
    }
}

///----------------------------

float srgb_to_linear_float(float x) {
    if (x <= 0.0f)
        return 0.0f;
    else if (x >= 1.0f)
        return 1.0f;
    else if (x < 0.04045f)
        return x / 12.92f;
    else
        return powf((x + 0.055f) / 1.055f, 2.4f);
}


float linear_to_srgb_float(float x) {
    if (x <= 0.0f)
        return 0.0f;
    else if (x >= 1.0f)
        return 1.0f;
    else if (x < 0.0031308f)
        return x * 12.92f;
    else
        return powf(x, 1.0f / 2.4f) * 1.055f - 0.055f;
}

class SRGBLookupTables
{
public:
    float Uint8ToLinear[256];
    float Uint8SrgbToLinear[256];
    float Uint16SrgbToLinear[65536];

    uint8_t Uint8LinearToSrgb[65537];
    uint16_t Uint16LinearToSrgb[65537];

    SRGBLookupTables()
    {
        for (int i = 0; i < 256; ++i)
        {
            float alpha = i / 255.0f;
            Uint8ToLinear[i] = alpha;
            Uint8SrgbToLinear[i] = srgb_to_linear_float(alpha);
        }

        for (int i = 0; i < 65536; ++i)
        {
            float alpha = i / 65535.0f;
            Uint16SrgbToLinear[i] = srgb_to_linear_float(alpha);
        }

        for (int i = 0; i <= 65536; ++i)
        {
            float alpha = i / 65536.0f;
            Uint8LinearToSrgb[i] = FloatTo255(linear_to_srgb_float(alpha));
            Uint16LinearToSrgb[i] = (uint16_t)std::clamp<float>(linear_to_srgb_float(alpha) * 65535.0f, 0.0f, 65535.0f);
        }
    }

};

static SRGBLookupTables GColorLookup;

struct Half
{
    uint16_t val;
};

struct UnpackerLinear
{
    static  void Unpack(uint8_t from, float& to) { to = GColorLookup.Uint8ToLinear[from]; }
    static void Unpack(uint16_t from, float& to) { to = from / 65535.0f; }
    static void Unpack(Half from, float& to) { to = Float16Helper::Decompress(from.val); }
    static void Unpack(float from, float& to) { to = from; }
};

struct UnpackerSRGB
{
    static void Unpack(uint8_t from, float& to) { to = GColorLookup.Uint8SrgbToLinear[from]; }
    static void Unpack(uint16_t from, float& to) { to = GColorLookup.Uint16SrgbToLinear[from]; }
    static void Unpack(Half from, float& to) { to = srgb_to_linear_float(Float16Helper::Decompress(from.val)); }
    static  void Unpack(float from, float& to) { to = srgb_to_linear_float(from); }
};

struct UnpackerHDR
{
    static void Unpack(uint8_t from, float& to) { to = from / 255.0f; }
    static void Unpack(uint16_t from, float& to) { to = from / 65535.0f; }
    static void Unpack(Half from, float& to) { to = log2(std::max<float>(0.001f, Float16Helper::Decompress(from.val))); }
    static void Unpack(float from, float& to) { to = log2(std::max<float>(0.001f, from)); }
};


template< int N, typename ST, typename Unpacker >
void UnpackData(const ImageView& src, float* writePtr)
{
    for (image::ImageViewSliceIterator z(src); z; ++z)
    {
        for (image::ImageViewRowIterator y(z); y; ++y)
        {
            const auto* src = (const ST*)y.data();
            for (image::ImageViewPixelIterator x(y); x; ++x, writePtr += N, src += N)
            {
                switch (N)
                {
                    case 4: typename Unpacker::Unpack(src[3], writePtr[3]);
                    case 3: typename Unpacker::Unpack(src[2], writePtr[2]);
                    case 2: typename Unpacker::Unpack(src[1], writePtr[1]);
                    case 1: typename Unpacker::Unpack(src[0], writePtr[0]);
                }
            }
        }
    }
}

template< int N, typename Unpacker >
void UnpackDataN(const ImageView& src, float* writePtr)
{
    switch (src.format())
    {
        case PixelFormat::Uint8_Norm: UnpackData<N, uint8_t, Unpacker>(src, writePtr); break;
        case PixelFormat::Uint16_Norm: UnpackData<N, uint16_t, Unpacker>(src, writePtr); break;
        case PixelFormat::Float16_Raw: UnpackData<N, Half, Unpacker>(src, writePtr); break;
        case PixelFormat::Float32_Raw: UnpackData<N, float, Unpacker>(src, writePtr); break;
    }
}

void UnpackIntoFloats(const ImageView& src, const ImageView& dest, ColorSpace space)
{
    DEBUG_CHECK_EX(dest.width() == src.width(), "Invalid destination width");
    DEBUG_CHECK_EX(dest.height() == src.height(), "Invalid destination width");
    DEBUG_CHECK_EX(dest.depth() == src.depth(), "Invalid destination width");
    DEBUG_CHECK_EX(dest.channels() == src.channels(), "Channel count must match");
    DEBUG_CHECK_EX(dest.format() == PixelFormat::Float32_Raw, "Destination format must be float array");
    DEBUG_CHECK_EX(dest.rowPitch() == dest.width() * dest.pixelPitch(), "Destination row pitch must be native (no gaps)");
    DEBUG_CHECK_EX(dest.slicePitch() == dest.width() * dest.height() * dest.pixelPitch(), "Destination slice pitch must be native (no gaps)");

    if (space == ColorSpace::SRGB)
    {
        switch (dest.channels())
        {
            case 1: UnpackDataN<1, UnpackerSRGB>(src, (float*)dest.data()); break;
            case 2: UnpackDataN<2, UnpackerSRGB>(src, (float*)dest.data()); break;
            case 3: UnpackDataN<3, UnpackerSRGB>(src, (float*)dest.data()); break;
            case 4: UnpackDataN<4, UnpackerSRGB>(src, (float*)dest.data()); break;
        }
    }
    else if (space == ColorSpace::HDR)
    {
        switch (dest.channels())
        {
            case 1: UnpackDataN<1, UnpackerHDR>(src, (float*)dest.data()); break;
            case 2: UnpackDataN<2, UnpackerHDR>(src, (float*)dest.data()); break;
            case 3: UnpackDataN<3, UnpackerHDR>(src, (float*)dest.data()); break;
            case 4: UnpackDataN<4, UnpackerHDR>(src, (float*)dest.data()); break;
        }
    }
    else
    {
        switch (dest.channels())
        {
        case 1: UnpackDataN<1, UnpackerLinear>(src, (float*)dest.data()); break;
        case 2: UnpackDataN<2, UnpackerLinear>(src, (float*)dest.data()); break;
        case 3: UnpackDataN<3, UnpackerLinear>(src, (float*)dest.data()); break;
        case 4: UnpackDataN<4, UnpackerLinear>(src, (float*)dest.data()); break;
        }
    }
}

//--

int PackFloat255(float x)
{
    int ret = (int)(255.0 * x);
    return std::clamp<int>(ret, 0, 255);
}

int PackFloat65535(float x)
{
    int ret = (int)(65535.0 * x);
    return std::clamp<int>(ret, 0, 65535);
}

int PackFloat65536(float x)
{
    int ret = (int)(65536.0 * x);
    return std::clamp<int>(ret, 0, 65536);
}

struct PackerLinear
{
    static void Pack(float from, uint8_t& to) { to = (uint8_t)PackFloat255(from); }
    static void Pack(float from, uint16_t& to) { to = (uint16_t)PackFloat65535(from); }
    static void Pack(float from, Half& to) { to.val = Float16Helper::Compress(from); }
    static void Pack(float from, float& to) { to = from; }
};

struct PackerSRGB
{
    static void Pack(float from, uint8_t& to) { to = GColorLookup.Uint8LinearToSrgb[PackFloat65536(from)]; } // the lookup table is 16 bit, so PackFloat65536 not 255
    static void Pack(float from, uint16_t& to) { to = GColorLookup.Uint16LinearToSrgb[PackFloat65536(from)]; }
    static void Pack(float from, Half& to) { to.val = Float16Helper::Compress(linear_to_srgb_float(from)); }
    static void Pack(float from, float& to) { to = linear_to_srgb_float(from); }
};

struct PackerHDR
{
    static void Pack(float from, uint8_t& to) { to = (uint8_t)PackFloat255(from); }
    static void Pack(float from, uint16_t& to) { to = (uint16_t)PackFloat65535(from); }
    static void Pack(float from, Half& to) { to.val = exp2(Float16Helper::Compress(from)); }
    static void Pack(float from, float& to) { to = exp2(from); }
};

template< int N, typename DT, typename Packer >
void PackData(const ImageView& dest, const float* readPtr)
{
    for (image::ImageViewSliceIterator z(dest); z; ++z)
    {
        for (image::ImageViewRowIterator y(z); y; ++y)
        {
            auto* dest = (DT*)y.data();
            for (image::ImageViewPixelIterator x(y); x; ++x, readPtr += N, dest += N)
            {
                switch (N)
                {
                    case 4: typename Packer::Pack(readPtr[3], dest[3]);
                    case 3: typename Packer::Pack(readPtr[2], dest[2]);
                    case 2: typename Packer::Pack(readPtr[1], dest[1]);
                    case 1: typename Packer::Pack(readPtr[0], dest[0]);
                }
            }
        }
    }
}

template< int N, typename Packer >
void PackDataN(const ImageView& dest, const float* readPtr)
{
    switch (dest.format())
    {
    case PixelFormat::Uint8_Norm: PackData<N, uint8_t, Packer>(dest, readPtr); break;
    case PixelFormat::Uint16_Norm: PackData<N, uint16_t, Packer>(dest, readPtr); break;
    case PixelFormat::Float16_Raw: PackData<N, Half, Packer>(dest, readPtr); break;
    case PixelFormat::Float32_Raw: PackData<N, float, Packer>(dest, readPtr); break;
    }
}

void PackFromFloats(const ImageView& src, const ImageView& dest, ColorSpace space)
{
    DEBUG_CHECK_EX(dest.width() == src.width(), "Invalid destination width");
    DEBUG_CHECK_EX(dest.height() == src.height(), "Invalid destination width");
    DEBUG_CHECK_EX(dest.depth() == src.depth(), "Invalid destination width");
    DEBUG_CHECK_EX(dest.channels() == src.channels(), "Channel count must match");
    DEBUG_CHECK_EX(src.format() == PixelFormat::Float32_Raw, "Source format must be float array");
    DEBUG_CHECK_EX(src.rowPitch() == src.width() * src.pixelPitch(), "Source row pitch must be native (no gaps)");
    DEBUG_CHECK_EX(src.slicePitch() == src.width() * src.height() * src.pixelPitch(), "Source slice pitch must be native (no gaps)");

    if (space == ColorSpace::SRGB)
    {
        switch (dest.channels())
        {
        case 1: PackDataN<1, PackerSRGB>(dest, (float*)src.data()); break;
        case 2: PackDataN<2, PackerSRGB>(dest, (float*)src.data()); break;
        case 3: PackDataN<3, PackerSRGB>(dest, (float*)src.data()); break;
        case 4: PackDataN<4, PackerSRGB>(dest, (float*)src.data()); break;
        }
    }
    else if (space == ColorSpace::HDR)
    {
        switch (dest.channels())
        {
        case 1: PackDataN<1, PackerHDR>(dest, (float*)src.data()); break;
        case 2: PackDataN<2, PackerHDR>(dest, (float*)src.data()); break;
        case 3: PackDataN<3, PackerHDR>(dest, (float*)src.data()); break;
        case 4: PackDataN<4, PackerHDR>(dest, (float*)src.data()); break;
        }
    }
    else
    {
        switch (dest.channels())
        {
        case 1: PackDataN<1,PackerLinear>(dest, (float*)src.data()); break;
        case 2: PackDataN<2, PackerLinear>(dest, (float*)src.data()); break;
        case 3: PackDataN<3, PackerLinear>(dest, (float*)src.data()); break;
        case 4: PackDataN<4, PackerLinear>(dest, (float*)src.data()); break;
        }
    }
}

//--

void DownsampleStream(const float* base, const float* extra, float* write, uint32_t count, DownsampleMode mode)
{
    auto* baseEnd = base + count;

    if (mode == DownsampleMode::Average)
    {
        while (base < baseEnd)
            *write++ = (*base++ + *extra++) * 0.5f;
    }
    else if (mode == DownsampleMode::AverageWithAlphaWeight)
    {
        while (base < baseEnd)
        {
            float t[4] = { 0,0,0,0 };
            t[0] = (base[0] * base[3]) + (extra[0] * extra[3]);
            t[1] = (base[1] * base[3]) + (extra[1] * extra[3]);
            t[2] = (base[2] * base[3]) + (extra[2] * extra[3]);
            t[3] = base[3] + extra[3];

            if (t[3] > 0.00001f)
            {
                const auto inv = 1.0f / t[3];
                write[0] = t[0] * inv;
                write[1] = t[1] * inv;
                write[2] = t[2] * inv;
                write[3] = t[3] * 0.5f;
            }
            else
            {
                write[0] = 0.0f;
                write[1] = 0.0f;
                write[2] = 0.0f;
                write[3] = 0.0f;
            }

            base += 4;
            extra += 4;
            write += 4;
        }
    }
    else if (mode == DownsampleMode::AverageWithPremultipliedAlphaWeight)
    {
        while (base < baseEnd)
        {
            float t[4] = { 0,0,0,0 };
            t[0] = base[0] + extra[0];
            t[1] = base[1] + extra[1];
            t[2] = base[2] + extra[2];
            t[3] = base[3] + extra[3];

            write[0] = t[0] * 0.50f;
            write[1] = t[1] * 0.50f;
            write[2] = t[2] * 0.50f;
            write[3] = t[3] * 0.50f;

            base += 4;
            extra += 4;
            write += 4;
        }
    }
}

ImageView DownsampleZ(const ImageView& data, DownsampleMode mode)
{
    DEBUG_CHECK_EX(data.format() == PixelFormat::Float32_Raw, "Source format must be float array");
    DEBUG_CHECK_EX(data.rowPitch() == data.width() * data.pixelPitch(), "Source row pitch must be native (no gaps)");
    DEBUG_CHECK_EX(data.slicePitch() == data.width() * data.height() * data.pixelPitch(), "Source slice pitch must be native (no gaps)");

    const auto sliceDataSize = data.slicePitch();
    const auto sliceElementCount = sliceDataSize / 4;
    const auto numSlicesToMerge = data.depth() / 2;

    if (mode == DownsampleMode::AverageWithAlphaWeight && data.channels() != 4)
        mode = DownsampleMode::Average;

    auto* writePtr = (float*)data.data();
    auto* readPtr = (float*)data.data();

    uint32_t z = 0;
    while (z < (numSlicesToMerge * 2))
    {
        DownsampleStream(readPtr, readPtr + sliceElementCount, writePtr, sliceElementCount, mode);
        writePtr += sliceElementCount;
        readPtr += sliceElementCount * 2;
        z += 2;
    }

    /*// copy last slice
    if (z < data.depth())
    {
        if (writePtr != readPtr)
            memmove(writePtr, readPtr, sliceDataSize);
        z += 1;
    }*/

    const auto newDepth = std::max<uint32_t>(1, data.depth() / 2);
    return ImageView(data.format(), data.channels(), data.data(), data.width(), data.height(), newDepth, data.pixelPitch(), data.rowPitch(), data.slicePitch());
}

ImageView DownsampleY(const ImageView& data, DownsampleMode mode)
{
    DEBUG_CHECK_EX(data.format() == PixelFormat::Float32_Raw, "Source format must be float array");
    DEBUG_CHECK_EX(data.rowPitch() == data.width() * data.pixelPitch(), "Source row pitch must be native (no gaps)");
    DEBUG_CHECK_EX(data.slicePitch() == data.width() * data.height() * data.pixelPitch(), "Source slice pitch must be native (no gaps)");

    const auto rowDataSize = data.rowPitch();
    const auto rowElementCount = rowDataSize / 4;
    const auto numRowsToMerge = data.height() / 2;

    if (mode == DownsampleMode::AverageWithAlphaWeight && data.channels() != 4)
        mode = DownsampleMode::Average;

    auto* writePtr = (float*)data.data();
    auto* readPtr = (float*)data.data();

    for (uint32_t z = 0; z < data.depth(); ++z)
    {
        uint32_t y = 0;
        while (y < (numRowsToMerge * 2))
        {
            DownsampleStream(readPtr, readPtr + rowElementCount, writePtr, rowElementCount, mode);
            writePtr += rowElementCount;
            readPtr += rowElementCount * 2;
            y += 2;
        }

        // copy last slice
        if (y < data.height())
        {
            //if (writePtr != readPtr)
                //  memmove(writePtr, readPtr, rowDataSize);
            //writePtr += rowElementCount;
            readPtr += rowElementCount;
            y += 1;
        }
    }

    const auto newHeight = std::max<uint32_t>(1, data.height() / 2);
    const auto newSlicePitch = newHeight * data.rowPitch();
    return ImageView(data.format(), data.channels(), data.data(), data.width(), newHeight, data.depth(), data.pixelPitch(), data.rowPitch(), newSlicePitch);
}

ImageView DownsampleX(const ImageView& data, DownsampleMode mode)
{
    DEBUG_CHECK_EX(data.format() == PixelFormat::Float32_Raw, "Source format must be float array");
    DEBUG_CHECK_EX(data.rowPitch() == data.width() * data.pixelPitch(), "Source row pitch must be native (no gaps)");
    DEBUG_CHECK_EX(data.slicePitch() == data.width() * data.height() * data.pixelPitch(), "Source slice pitch must be native (no gaps)");

    const auto pixelDataSize = data.pixelPitch();
    const auto pixelElementCount = pixelDataSize / 4;
    const auto numPixelsToMerge = data.width() / 2;

    if (mode == DownsampleMode::AverageWithAlphaWeight && data.channels() != 4)
        mode = DownsampleMode::Average;

    auto* writePtr = (float*)data.data();
    auto* readPtr = (float*)data.data();

    for (uint32_t z = 0; z < data.depth(); ++z)
    {
        for (uint32_t y = 0; y < data.height(); ++y)
        {
            uint32_t x = 0;
            while (x < (numPixelsToMerge * 2))
            {
                DownsampleStream(readPtr, readPtr + pixelElementCount, writePtr, pixelElementCount, mode);
                writePtr += pixelElementCount;
                readPtr += pixelElementCount * 2;
                x += 2;
            }

            // copy last slice
            if (x < data.width())
            {
                //if (writePtr != readPtr)
                    //  memmove(writePtr, readPtr, pixelDataSize);
                //writePtr += pixelElementCount;
                readPtr += pixelElementCount;
                x += 1;
            }
        }
    }

    const auto newWidth = std::max<uint32_t>(1, data.width() / 2);
    const auto newRowPitch = newWidth * data.pixelPitch();
    const auto newSlicePitch = data.height() * newRowPitch;
    return ImageView(data.format(), data.channels(), data.data(), newWidth, data.height(), data.depth(), data.pixelPitch(), newRowPitch, newSlicePitch);
}

void Downsample(const ImageView& src, const ImageView& dest, DownsampleMode mode, ColorSpace space)
{
    DEBUG_CHECK_EX(dest.width() == std::max<uint32_t>(1, src.width() / 2), "Invalid destination width");
    DEBUG_CHECK_EX(dest.height() == std::max<uint32_t>(1, src.height() / 2), "Invalid destination width");
    DEBUG_CHECK_EX(dest.depth() == std::max<uint32_t>(1, src.depth() / 2), "Invalid destination width");
    DEBUG_CHECK_EX(dest.format() == src.format(), "Formats must match");
    DEBUG_CHECK_EX(dest.channels() == src.channels(), "Channel count must match");

    if (src.empty() || dest.empty())
        return;

    // prepare temporary storage
    Array<float> tempData;

    const auto numFloats = src.channels() * src.width() * src.height() * src.depth();
    tempData.resize(numFloats);

    ImageView tempDataView = ImageView(NATIVE_LAYOUT, PixelFormat::Float32_Raw, src.channels(), tempData.data(), src.width(), src.height(), src.depth());

    // load source data
    UnpackIntoFloats(src, tempDataView, space);

    // downsample data
    tempDataView = DownsampleZ(tempDataView, mode);
    tempDataView = DownsampleY(tempDataView, mode);
    tempDataView = DownsampleX(tempDataView, mode);

    DEBUG_CHECK_EX(tempDataView.width() == dest.width(), "Invalid image size after downscaling");
    DEBUG_CHECK_EX(tempDataView.height() == dest.height(), "Invalid image size after downscaling");
    DEBUG_CHECK_EX(tempDataView.depth() == dest.depth(), "Invalid image size after downscaling");

    // pack into destination data
    PackFromFloats(tempDataView, dest, space);
}

ImagePtr Downsampled(const ImageView& src, DownsampleMode mode, ColorSpace space)
{
    if (src.empty())
        return nullptr;

    const auto width = std::max<uint32_t>(1, src.width() / 2);
    const auto height = std::max<uint32_t>(1, src.height() / 2);
    const auto depth = std::max<uint32_t>(1, src.depth() / 2);

    auto ret = RefNew<Image>(src.format(), src.channels(), width, height, depth);
    Downsample(src, ret->view(), mode, space);

    return ret;
}

///----------------------------

template< typename T, typename BT = T >
void PremultiplyLine(T* destMem, uint32_t width, BT divisor = (BT)1)
{
    auto* destEnd = destMem + (width * 4);
    while (destMem < destEnd)
    {
        auto t0 = (BT)destMem[0] * (BT)destMem[3];
        auto t1 = (BT)destMem[1] * (BT)destMem[3];
        auto t2 = (BT)destMem[2] * (BT)destMem[3];
        destMem[0] = (T)(t0 / divisor);
        destMem[1] = (T)(t1 / divisor);
        destMem[2] = (T)(t2 / divisor);
        destMem += 4;
    }
}

void PremultiplyLine(PixelFormat format, void* destMem, uint32_t width)
{
    switch (format)
    {
        case PixelFormat::Uint8_Norm: PremultiplyLine<uint8_t, uint16_t>((uint8_t*)destMem, width, 255); break;
        case PixelFormat::Uint16_Norm: PremultiplyLine<uint16_t, uint32_t>((uint16_t*)destMem, width, 65535); break;
        case PixelFormat::Float16_Raw: PremultiplyLine<uint16_t, uint32_t>((uint16_t*)destMem, width, 65535);  break;// TODO!
        case PixelFormat::Float32_Raw: PremultiplyLine<float>((float*)destMem, width);  break;// TODO!
    }
}

void PremultiplyAlpha(const ImageView& dest)
{
    PC_SCOPE_LVL2(ImagePremultiply);

    DEBUG_CHECK_EX(dest.channels() == 4, "Alpha premultiply works only on RGBA images");
    if (dest.channels() != 4)
        return;

    auto writePtr = (uint8_t*)dest.data();
    for (uint32_t z = 0; z < dest.depth(); ++z)
    {
        auto sliceWritePtr = writePtr;

        for (uint32_t y = 0; y < dest.height(); ++y)
        {
            PremultiplyLine(dest.format(), writePtr, dest.width());
            writePtr += dest.rowPitch();
        }

        writePtr = sliceWritePtr + dest.slicePitch();
    }
}

///----------------------------

END_BOOMER_NAMESPACE_EX(image)
