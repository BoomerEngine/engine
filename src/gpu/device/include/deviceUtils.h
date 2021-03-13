/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once


BEGIN_BOOMER_NAMESPACE_EX(gpu)

///--

/// create a vertex buffer from given data
extern GPU_DEVICE_API BufferObjectPtr CreateVertexBuffer(uint32_t size, const void* data, StringView name = "CustomVertexBuffer");

/// create a index buffer from given data
extern GPU_DEVICE_API BufferObjectPtr CreateIndexBuffer(uint32_t size, const void* data, StringView name = "CustomIndexBuffer");

/// create buffer from given data
extern GPU_DEVICE_API BufferObjectPtr CreateBuffer(const BufferCreationInfo& info, const void* data);

///--

// generate cube vertices (12 triangles)
extern GPU_DEVICE_API void GenerateCubeVertices(Vector3* outVertices, uint32_t vertexStride, Vector3 extents = Vector3(0.5f, 0.5f, 0.5f));

// compute general normals for triangle soup
extern GPU_DEVICE_API void GenerateNormals(const Vector3* vertices, uint32_t vertexStride, Vector3* normals, uint32_t normalStride, uint32_t numTriangles);

///--

END_BOOMER_NAMESPACE_EX(gpu)

