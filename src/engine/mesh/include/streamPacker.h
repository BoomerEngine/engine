/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: streams #]
***/

#pragma once

#include "streamData.h"

BEGIN_BOOMER_NAMESPACE()
        
///-----

class MeshDataIteratorBase;

/// packed data format
enum class PackedDataFormat : uint8_t
{
    Float1,
    Float2,
    Float3,
    Float4,
    Half2,
    Half4,
    Byte4,
    Word2,
    Word4,
    DWord1,
    DWord2,
    DWord3,
    DWord4,
};

/// stream for packing
struct ENGINE_MESH_API PackedStreamInfo
{
    PackedDataFormat sourceFormat = PackedDataFormat::Float4;
    PackedDataFormat packedFormat = PackedDataFormat::Float4;
    const void* sourceData = nullptr;
    uint32_t sourceStride = 0;

    PackedStreamInfo() {};
    PackedStreamInfo(const MeshDataIteratorBase& it); // takes whole range
};

/// get data packing format for mesh stream type
/// NOTE: we can change this value, we don't have requirement to pack data exactly as specified by this function
extern ENGINE_MESH_API PackedDataFormat GetPackedFormatForStream(MeshStreamType stream);

/// get packed stream stride
extern ENGINE_MESH_API uint32_t GetPackedStreamStride(PackedDataFormat format);

/// calculate size needed to pack streams
extern ENGINE_MESH_API uint64_t CalcPackedSize(const PackedStreamInfo* streams, uint32_t numStreams, uint64_t numElements);

/// pack a data streams into a buffer, streams are packed as AOS
extern ENGINE_MESH_API void PackStreams(const PackedStreamInfo* streams, uint32_t numStreams, uint64_t numElements, void* writePtr, void* writePtrEnd);

///----

END_BOOMER_NAMESPACE()
