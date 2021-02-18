/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooker #]
***/

#pragma once

#include "rendering/mesh/include/renderingMeshFormat.h"
#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/mesh/include/renderingMeshStreamData.h"

namespace rendering
{
    //---

    // pack rendering data from source data
    // NOTE: this is the rendering-only cooking, no additional mesh data is cooked (SDF, physics, sound occlusion, etc)
    extern IMPORT_MESH_LOADER_API bool BuildChunks(const base::Array<MeshRawChunk>& sourceChunks, const MeshImportConfig& settings, base::IProgressTracker& progressTracker, base::Array<MeshChunk>& outRenderChunks, base::Box& outBounds);

    //---

    // merge duplicated vertex buffer entries, returns new number of vertices and remapping table
    extern IMPORT_MESH_LOADER_API uint32_t OptimizeVertexBuffer(const void* vertexData, uint32_t vertexCount, MeshVertexFormat format, base::Array<uint32_t>& outRemapTable);

    // remap vertices using a remap table
    extern IMPORT_MESH_LOADER_API base::Buffer RemapVertexBuffer(const void* currentVertexData, uint32_t currentVertexCount, MeshVertexFormat format, uint32_t newVertexCount, const uint32_t* oldToNewRemapTable);

    // remap indices using a remap table
    extern IMPORT_MESH_LOADER_API void RemapIndexBuffer(uint32_t* currentIndexData, uint32_t currentIndexCount, const uint32_t* oldToNewRemapTable);

    // optimize vertex cache reuse 
    extern IMPORT_MESH_LOADER_API void OptimizeVertexCache(uint32_t* currentIndexData, uint32_t currentIndexCount, uint32_t currentVertexCount);

    // optimize vertex cache reuse 
    extern IMPORT_MESH_LOADER_API base::Buffer OptimizeVertexFetch(const void* currentVertexData, uint32_t currentVertexCount, MeshVertexFormat format, uint32_t* currentIndexData, uint32_t currentIndexCount);

    // pack vertex buffer data
    extern IMPORT_MESH_LOADER_API base::Buffer CompressVertexBuffer(const void* currentVertexData, MeshVertexFormat format, uint32_t count);

    // pack index buffer data
    extern IMPORT_MESH_LOADER_API base::Buffer CompressIndexBuffer(const void* currentIndexData, uint32_t indexCount, uint32_t vertexCount);

    //---

} // rendering
