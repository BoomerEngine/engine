/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

#include "gpu/device/include/imageFormat.h"
#include "streamData.h"

BEGIN_BOOMER_NAMESPACE()

//---

struct MeshVertexStreamInfo
{
    StringView name; // binding name
    MeshStreamType sourceStream; // source stream 
    ImageFormat dataFormat; // format of the data
    uint16_t dataOffset; // offset in the vertex
    uint16_t dataSize; // size of the data in the vertex

    ImageFormat readFormat; // format to read as in the shader, usually R32F or R32_UINT or similar
    StringView readFunction; // unpacking shader function (from vertex.h)
};

struct MeshVertexFormatInfo
{
    StringView name; // binding name
    uint16_t stride; // total vertex stride
    uint8_t numStreams; // number of data streams
    bool quantizedPosition = false;
    const MeshVertexStreamInfo* streams; // data stream
};

//--

struct SourceMeshStream
{
    MeshStreamType stream = MeshStreamType::Position_3F;
    const void* srcData = nullptr;
    uint32_t srcDataStride = 0;
};

//--

struct ENGINE_MESH_API MeshVertexQuantizationHelper
{
    MeshVertexQuantizationHelper(const Box& absoluteBounds);

    //--

    // quantize single vertex to 11_11_10 format
    uint32_t QuantizePosition_11_11_10(const Vector3& pos) const;

    // quantize single vertex to 24_24_16 format
    uint64_t QuantizePosition_22_22_20(const Vector3& pos) const;

    // quantize range of data to 11_11_10 format, input data is expected to be Vector32
    void QuantizePositions_11_11_10(void* outData, uint32_t outDataStride, const void* inData, uint32_t inputDataStride, uint32_t count) const;

    //--

    INLINE Vector3 quantizatonOffset() const { return -m_quantizationOffset;  }
    INLINE Vector3 quantizatonScale_11_11_10() const { return Vector3(1.0f / m_quantizationScale_11_11_10.x, 1.0f / m_quantizationScale_11_11_10.y, 1.0f / m_quantizationScale_11_11_10.z); }
    INLINE Vector3 quantizatonScale_22_22_20() const { return Vector3(1.0f / m_quantizationScale_22_22_20[0], 1.0f / m_quantizationScale_22_22_20[1], 1.0f / m_quantizationScale_22_22_20[2]); }

    //--

private:
    Box m_absoluteBounds;
    Vector3 m_quantizationOffset;
    Vector3 m_quantizationScale_11_11_10;
    double m_quantizationScale_22_22_20[3];
};


//--

// get information about vertex format
extern ENGINE_MESH_API const MeshVertexFormatInfo& GetMeshVertexFormatInfo(MeshVertexFormat format);

// find vertex format by name
extern ENGINE_MESH_API bool GetVertexFormatByName(StringView name, MeshVertexFormat& outFormat);
 
//--

// pack data stream
extern ENGINE_MESH_API void PackStreamData(const MeshVertexQuantizationHelper& quantization, const void* srcData, uint32_t srcStride, MeshStreamType type, void* destData, uint32_t destStride, ImageFormat destFormat, uint32_t count);

// pack vertex stream for given format, missing data is filled with zeros of 1 (for colors)
extern ENGINE_MESH_API void PackVertexData(const MeshVertexQuantizationHelper& quantization, const SourceMeshStream* srcStreams, uint32_t srcStreamCount, void* destData, MeshVertexFormat destFormat, uint32_t count);

//--

// unpack compressed vertex buffer data
extern ENGINE_MESH_API Buffer UncompressVertexBuffer(const void* compressedVertexData, uint32_t compressedDataSize, MeshVertexFormat format, uint32_t count);

// unpack compressed index buffer data
extern ENGINE_MESH_API Buffer UncompressIndexBuffer(const void* compressedIndexData, uint32_t compressedDataSize, uint32_t count);

//--

END_BOOMER_NAMESPACE()
