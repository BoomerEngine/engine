/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: streams #]
***/

#include "build.h"
#include "streamIterator.h"
#include "streamPacker.h"

BEGIN_BOOMER_NAMESPACE()

///---

namespace packing
{

    typedef void(*TPackFunction)(const uint8_t* srcData, uint32_t srcStride, uint8_t* destData, uint32_t destStride, uint64_t numElements);

    // no data conversion
    template<typename T>
    static void PackFunction_Identity(const uint8_t* srcData, uint32_t srcStride, uint8_t* destData, uint32_t destStride, uint64_t numElements)
    {
        auto elementsLeft = numElements;
        while (elementsLeft--)
        {
            *(T*)destData = *(const T*)srcData;
            srcData += srcStride;
            destData += destStride;
        }
    }

    // quantization to 8bits
    template<typename T>
    uint8_t Quantize_8Bit_UNorm(const T& val) { return 0; };

    template<>
    uint8_t Quantize_8Bit_UNorm<float>(const float& val)
    {
        return (uint8_t)std::clamp<int>((int)round(val * 255.0f), 0, 255);
    }

    static void PackFunction_Vector3ToColor(const uint8_t* srcData, uint32_t srcStride, uint8_t* destData, uint32_t destStride, uint64_t numElements)
    {
        auto elementsLeft = numElements;
        while (elementsLeft--)
        {
            auto& srcVector = *(const Vector3*)srcData;
            destData[0] = Quantize_8Bit_UNorm(0.5f + 0.5f * srcVector.x);
            destData[1] = Quantize_8Bit_UNorm(0.5f + 0.5f * srcVector.y);
            destData[2] = Quantize_8Bit_UNorm(0.5f + 0.5f * srcVector.z);
            destData[3] = 255;
            srcData += srcStride;
            destData += destStride;
        }
    }

    // quantize 4 to 8bits (color)
    template<typename T>
    static void PackFunction_8Bit_UNorm(const uint8_t* srcData, uint32_t srcStride, uint8_t* destData, uint32_t destStride, uint64_t numElements)
    {
        auto elementsLeft = numElements;
        while (elementsLeft--)
        {
            destData[0] = Quantize_8Bit_UNorm(((const T*)srcData)[0]);
            destData[1] = Quantize_8Bit_UNorm(((const T*)srcData)[1]);
            destData[2] = Quantize_8Bit_UNorm(((const T*)srcData)[2]);
            destData[3] = Quantize_8Bit_UNorm(((const T*)srcData)[3]);
            srcData += srcStride;
            destData += destStride;
        }
    }

    // pack 2 elements to float16
    static void PackFunction_Float16_2(const uint8_t* srcData, uint32_t srcStride, uint8_t* destData, uint32_t destStride, uint64_t numElements)
    {
        auto elementsLeft = numElements;
        while (elementsLeft--)
        {
            ((uint16_t*)destData)[0] = Float16Helper::Compress(((const float*)srcData)[0]);
            ((uint16_t*)destData)[1] = Float16Helper::Compress(((const float*)srcData)[1]);
            srcData += srcStride;
            destData += destStride;
        }
    }

    // pack 4 elements to float16
    static void PackFunction_Float16_4(const uint8_t* srcData, uint32_t srcStride, uint8_t* destData, uint32_t destStride, uint64_t numElements)
    {
        auto elementsLeft = numElements;
        while (elementsLeft--)
        {
            ((uint16_t*)destData)[0] = Float16Helper::Compress(((const float*)srcData)[0]);
            ((uint16_t*)destData)[1] = Float16Helper::Compress(((const float*)srcData)[1]);
            ((uint16_t*)destData)[2] = Float16Helper::Compress(((const float*)srcData)[2]);
            ((uint16_t*)destData)[3] = Float16Helper::Compress(((const float*)srcData)[3]);
            srcData += srcStride;
            destData += destStride;
        }
    }

    // pack 4 elements to float16
    static void PackFunction_Float16_4_from_3(const uint8_t* srcData, uint32_t srcStride, uint8_t* destData, uint32_t destStride, uint64_t numElements)
    {
        auto elementsLeft = numElements;
        while (elementsLeft--)
        {
            ((uint16_t*)destData)[0] = Float16Helper::Compress(((const float*)srcData)[0]);
            ((uint16_t*)destData)[1] = Float16Helper::Compress(((const float*)srcData)[1]);
            ((uint16_t*)destData)[2] = Float16Helper::Compress(((const float*)srcData)[2]);
            ((uint16_t*)destData)[3] = 0.0f;
            srcData += srcStride;
            destData += destStride;
        }
    }

    // get suitable packing function
    static TPackFunction GetPackingFunction(PackedDataFormat streamType, PackedDataFormat dataType)
    {
        if (streamType == PackedDataFormat::Float1)
        {
            if (dataType == PackedDataFormat::Float1)
                return &PackFunction_Identity<float>;
        }
        else if (streamType == PackedDataFormat::Float2)
        {
            if (dataType == PackedDataFormat::Float2)
                return &PackFunction_Identity<Vector2>;
            else if (dataType == PackedDataFormat::Half2)
                return &PackFunction_Float16_2;
        }
        else if (streamType == PackedDataFormat::Float3)
        {
            if (dataType == PackedDataFormat::Float3)
                return &PackFunction_Identity<Vector3>;
            else if (dataType == PackedDataFormat::Half4)
                return &PackFunction_Float16_4_from_3;
            else if (dataType == PackedDataFormat::Byte4)
                return &PackFunction_Vector3ToColor;
        }
        else if (streamType == PackedDataFormat::Float4)
        {
            if (dataType == PackedDataFormat::Float4)
                return &PackFunction_Identity<Vector4>;
            else if (dataType == PackedDataFormat::Half4)
                return &PackFunction_Float16_4;
            else if (dataType == PackedDataFormat::Byte4)
                return &PackFunction_8Bit_UNorm<float>;
        }
        else if (streamType == PackedDataFormat::Byte4)
        {
            if (dataType == PackedDataFormat::Byte4)
                return &PackFunction_Identity<uint32_t>;
            else if (dataType == PackedDataFormat::DWord1)
                return &PackFunction_Identity<uint32_t>;
        }
        else if (streamType == PackedDataFormat::DWord1)
        {
            if (dataType == PackedDataFormat::Byte4)
                return &PackFunction_Identity<uint32_t>;
            else if (dataType == PackedDataFormat::DWord1)
                return &PackFunction_Identity<uint32_t>;
        }
        else if (streamType == PackedDataFormat::DWord2)
        {
            if (dataType == PackedDataFormat::DWord2)
                return &PackFunction_Identity<Point>;
        }
        else if (streamType == PackedDataFormat::DWord3)
        {
            if (dataType == PackedDataFormat::DWord3)
                return &PackFunction_Identity<Vector3>;
        }
        else if (streamType == PackedDataFormat::DWord4)
        {
            if (dataType == PackedDataFormat::DWord4)
                return &PackFunction_Identity<Rect>;
        }

        // TODO: fill with more conversion functions
        FATAL_ERROR("Unknown packing mode");
        return nullptr;
    }

} // packer

///---


PackedStreamInfo::PackedStreamInfo(const MeshDataIteratorBase& it)
    : sourceFormat(GetPackedFormatForStream(it.tag()))
    , packedFormat(GetPackedFormatForStream(it.tag()))
    , sourceData(it.base())
    , sourceStride(it.stride())
{}

///---

PackedDataFormat GetPackedFormatForStream(MeshStreamType stream)
{
    switch (stream)
    {
        case MeshStreamType::Position_3F:
            return PackedDataFormat::Float3;

        case MeshStreamType::Normal_3F:
        case MeshStreamType::Tangent_3F:
        case MeshStreamType::Binormal_3F:
            return PackedDataFormat::Float3; // TODO 10 11 11, etc

        case MeshStreamType::TexCoord0_2F:
            return PackedDataFormat::Float2;

        case MeshStreamType::TexCoord1_2F:
        case MeshStreamType::TexCoord2_2F:
        case MeshStreamType::TexCoord3_2F:
            return PackedDataFormat::Half2;

        case MeshStreamType::Color0_4U8:
        case MeshStreamType::Color1_4U8:
        case MeshStreamType::Color2_4U8:
        case MeshStreamType::Color3_4U8:
            return PackedDataFormat::Byte4;

        case MeshStreamType::SkinningIndices_4U8:
        case MeshStreamType::SkinningWeights_4F:
        case MeshStreamType::SkinningIndicesEx_4U8:
        case MeshStreamType::SkinningWeightsEx_4F:
            return PackedDataFormat::Byte4;

        case MeshStreamType::General0_F4:
        case MeshStreamType::General1_F4:
        case MeshStreamType::General2_F4:
        case MeshStreamType::General3_F4:
        case MeshStreamType::General4_F4:
        case MeshStreamType::General5_F4:
        case MeshStreamType::General6_F4:
        case MeshStreamType::General7_F4:
            return PackedDataFormat::Float4;
    }

    ASSERT("Unknown stream type");
    return PackedDataFormat::Float4;
}

uint32_t GetPackedStreamStride(PackedDataFormat format)
{
    switch (format)
    {
        case PackedDataFormat::Float1:
        case PackedDataFormat::Half2:
        case PackedDataFormat::Word2:
        case PackedDataFormat::DWord1:
        case PackedDataFormat::Byte4:
            return 4;

        case PackedDataFormat::Float2:
        case PackedDataFormat::Half4:
        case PackedDataFormat::DWord2:
            return 8;

        case PackedDataFormat::DWord3:
        case PackedDataFormat::Float3:
            return 12;

        case PackedDataFormat::Float4:
        case PackedDataFormat::Word4:
        case PackedDataFormat::DWord4:
            return 16;
    }

    ASSERT("Unknown packed format");
    return 0;
}

uint64_t CalcPackedSize(const PackedStreamInfo* streams, uint32_t numStreams, uint64_t numElements)
{
    uint32_t structSize = 0;
    for (uint32_t i = 0; i < numStreams; ++i)
        structSize += GetPackedStreamStride(streams[i].packedFormat);

    return structSize * numElements;
}

void PackStreams(const PackedStreamInfo* streams, uint32_t numStreams, uint64_t numElements, void* writePtr, void* writePtrEnd)
{
    uint32_t structSize = 0;
    for (uint32_t i = 0; i < numStreams; ++i)
        structSize += GetPackedStreamStride(streams[i].packedFormat);

    if (structSize == 0)
        return;

    auto maxWrite = ((uint8_t*)writePtrEnd - (uint8_t*)writePtr) / structSize;
    numElements = std::min<uint64_t>(numElements, maxWrite);

    uint32_t innerOffset = 0;
    for (uint32_t i = 0; i < numStreams; ++i)
    {
        auto destStride = GetPackedStreamStride(streams[i].packedFormat);

        if (auto func = packing::GetPackingFunction(streams[i].sourceFormat, streams[i].packedFormat))
        {
            auto destPtr  = OffsetPtr(writePtr, innerOffset);
            func((uint8_t*)streams[i].sourceData, streams[i].sourceStride, (uint8_t*)destPtr, destStride, numElements);
        }

        innerOffset += destStride;
    }
}

//--

END_BOOMER_NAMESPACE()
