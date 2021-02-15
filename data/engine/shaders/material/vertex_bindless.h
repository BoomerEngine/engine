/***
* Boomer Engine v4 2015-2017
* Written by Tomasz "Rex Dex" Jonarski
*
* Meshlet support for materials
*
***/

#pragma once

#include "vertex.h"

//----

#define OBJECT_FLAG_SELECTED 1

descriptor ObjectBuffers
{
    attribute(layout=ObjectInfo) Buffer ObjectData;
};

//---- STATIC DATA

struct MeshChunkInfo
{
    uint FirstWordInVertexBuffer;
    uint NumWordsPerVertex;
    uint FirstWordInIndexBuffer;
    uint NumVertices;
    uint NumIndices;
	uint _padding0;
	uint _padding1;
	uint _padding2;
	vec4 QuantizationOffset;
	vec4 QuantizationScale;
}

descriptor MeshBuffers
{
    attribute(format = r32ui) Buffer MeshIndexData;
    attribute(format = r32ui) Buffer MeshVertexData;
    attribute(layout=MeshChunkInfo) Buffer MeshChunkData;
}

//--- RUNTIME DATA

struct MeshDrawChunkInfo
{
    uint ObjectID;
    uint MeshChunkID;
};

descriptor MeshDrawData
{
    attribute(layout=MeshDrawChunkInfo) Buffer MeshChunkDrawData;
}

//--

vec3 UnpackPosition_11_11_10(uint data)
{
    float x = (data & 2047) / 2047.0f;
    float y = ((data >> 11) & 2047) / 2047.0f;
    float z = ((data >> 22) & 1023) / 1023.0f;

    return vec3(x, y, z);
}

vec3 UnpackPosition_11_11_10_NotNorm(uint data)
{
    float x = (data & 2047);
    float y = ((data >> 11) & 2047);
    float z = ((data >> 22) & 1023);

    return vec3(x, y, z);
}

vec3 UnpackPosition_22_22_20(uint dataLo, uint dataHi)
{
    float x = (dataLo & 4194303);
    float y = ((dataLo >> 22) | ((dataHi & 4095) << 10)) & 4194303;
    float z = (dataHi >> 12) & 1048575;

    return vec3(x, y, z);
}

vec3 UnpackNormalVector(uint data)
{
    return normalize((UnpackPosition_11_11_10(data) * 2.0) - 1.0);
}

vec4 UnpackColorRGBA4(uint data)
{
    return vec4(1);//unpackUnorm4x8(data);
}

vec2 UnpackHalf2(uint data)
{
    return unpackHalf2x16(data);
}

export shader MaterialVS
{
    uint FetchObjectIndex()
    {
        uint drawObjectIndex = gl_BaseInstance + gl_InstanceID;
        return MeshChunkDrawData[drawObjectIndex].ObjectID;
    }

    uint FetchVertexOffsetInWords()
    {
        uint drawObjectIndex = gl_BaseInstance + gl_InstanceID;
        MeshDrawChunkInfo drawObject = MeshChunkDrawData[drawObjectIndex];

        uint indexDataOffsetInWords = MeshChunkData[drawObject.MeshChunkID].FirstWordInIndexBuffer;
        uint indexOfVertex = MeshIndexData[indexDataOffsetInWords + gl_VertexID];

        uint vertexStrideInWords = MeshChunkData[drawObject.MeshChunkID].NumWordsPerVertex;
        uint vertexOffsetInWords = MeshChunkData[drawObject.MeshChunkID].FirstWordInVertexBuffer + (vertexStrideInWords * indexOfVertex);

        return vertexOffsetInWords;
    }

    vec3 DecompressQuantizedPosition(vec3 pos)
    {
        uint drawObjectIndex = gl_BaseInstance + gl_InstanceID;

        uint chunkId = MeshChunkDrawData[drawObjectIndex].MeshChunkID;
        vec3 QuantizationOffset = MeshChunkData[chunkId].QuantizationOffset.xyz;
        vec3 QuantizationScale = MeshChunkData[chunkId].QuantizationScale.xyz;

        return (QuantizationScale * pos) + QuantizationOffset;
    }
}

//--