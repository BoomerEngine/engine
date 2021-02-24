/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import #]
***/

#include "build.h"
#include "renderingMeshCooker.h"
#include "meshopt/meshoptimizer.h"

BEGIN_BOOMER_NAMESPACE(rendering)

//---

void UnpackChunkIntoStreams(const void* vertexData, MeshVertexFormat format, base::Array<meshopt_Stream>& outStreams)
{
    const auto& formatInfo = GetMeshVertexFormatInfo(format);
    for (uint32_t i = 0; i < formatInfo.numStreams; ++i)
    {
        const auto& formatEntry = formatInfo.streams[i];

        auto& outStreamEntry = outStreams.emplaceBack();
        outStreamEntry.data = base::OffsetPtr(vertexData, formatEntry.dataOffset);
        outStreamEntry.size = formatEntry.dataSize;
        outStreamEntry.stride = formatInfo.stride;
    }
}

uint32_t OptimizeVertexBuffer(const void* vertexData, uint32_t vertexCount, MeshVertexFormat format, base::Array<uint32_t>& outRemapTable)
{
    PC_SCOPE_LVL1(OptimizeVertexBuffer);

    const auto& formatInfo = GetMeshVertexFormatInfo(format);

    outRemapTable.reset();
    outRemapTable.resize(vertexCount);

    return meshopt_generateVertexRemap(outRemapTable.typedData(), nullptr, vertexCount, vertexData, vertexCount, formatInfo.stride);
}

base::Buffer RemapVertexBuffer(const void* currentVertexData, uint32_t currentVertexCount, MeshVertexFormat format, uint32_t newVertexCount, const uint32_t* oldToNewRemapTable)
{
    const auto& formatInfo = GetMeshVertexFormatInfo(format);

    base::Buffer outputData;
    outputData.init(POOL_TEMP, formatInfo.stride * newVertexCount, 16);

    meshopt_remapVertexBuffer(outputData.data(), currentVertexData, currentVertexCount, formatInfo.stride, oldToNewRemapTable);

    return outputData;
}

void RemapIndexBuffer(uint32_t* currentIndexData, uint32_t currentIndexCount, const uint32_t* oldToNewRemapTable)
{
    meshopt_remapIndexBuffer(currentIndexData, currentIndexData, currentIndexCount, oldToNewRemapTable);
}

void OptimizeVertexCache(uint32_t* currentIndexData, uint32_t currentIndexCount, uint32_t currentVertexCount)
{
    meshopt_optimizeVertexCache(currentIndexData, currentIndexData, currentIndexCount, currentVertexCount);
}

base::Buffer OptimizeVertexFetch(const void* currentVertexData, uint32_t currentVertexCount, MeshVertexFormat format, uint32_t* currentIndexData, uint32_t currentIndexCount)
{
    const auto& formatInfo = GetMeshVertexFormatInfo(format);

    base::Buffer outputData;
    outputData.init(POOL_TEMP, formatInfo.stride * currentVertexCount, 16);

    meshopt_optimizeVertexFetch(outputData.data(), currentIndexData, currentIndexCount, currentVertexData, currentVertexCount, formatInfo.stride);

    return outputData;
}

base::Buffer CompressVertexBuffer(const void* currentVertexData, MeshVertexFormat format, uint32_t count)
{
    const auto& formatInfo = GetMeshVertexFormatInfo(format);

    const auto maxSize = meshopt_encodeVertexBufferBound(count, formatInfo.stride);

    base::Buffer worstCaseBuffer;
    worstCaseBuffer.init(POOL_TEMP, maxSize);

    const auto actualSize = meshopt_encodeVertexBuffer(worstCaseBuffer.data(), worstCaseBuffer.size(), currentVertexData, count, formatInfo.stride);
    if (actualSize == 0)
        return base::Buffer();

    DEBUG_CHECK(actualSize <= maxSize);
    worstCaseBuffer.adjustSize(actualSize);

    return worstCaseBuffer;
}

base::Buffer CompressIndexBuffer(const void* currentIndexData, uint32_t indexCount, uint32_t vertexCount)
{
    const auto maxSize = meshopt_encodeIndexBufferBound(indexCount, vertexCount);

    base::Buffer worstCaseBuffer;
    worstCaseBuffer.init(POOL_TEMP, maxSize);

    const auto actualSize = meshopt_encodeIndexBuffer(worstCaseBuffer.data(), worstCaseBuffer.size(), (const uint32_t*)currentIndexData, indexCount);
    if (actualSize == 0)
        return base::Buffer();

    DEBUG_CHECK(actualSize <= maxSize);
    worstCaseBuffer.adjustSize(actualSize);

    return worstCaseBuffer;
}

//---
    
END_BOOMER_NAMESPACE(rendering)
