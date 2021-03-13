/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: service #]
***/

#include "build.h"
#include "device.h"
#include "deviceService.h"
#include "deviceUtils.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

BufferObjectPtr CreateBuffer(const BufferCreationInfo& info, const void* data)
{
    if (auto service = GetService<DeviceService>())
    {
        if (auto dev = service->device())
        {
            if (data)
            {
                auto buffer = Buffer::Create(POOL_TEMP, info.size, 16, data);
                auto wrapper = RefNew<SourceDataProviderBuffer>(buffer, 0, info.size);
                return dev->createBuffer(info, wrapper);
            }
            else
            {
                return dev->createBuffer(info);
            }
        }
    }

    return nullptr;
}

BufferObjectPtr CreateVertexBuffer(uint32_t size, const void* data, StringView name)
{
    BufferCreationInfo info;
    info.allowCopies = true;
    info.allowDynamicUpdate = true;
    info.allowVertex = true;
    info.label = StringBuf(name);
    info.size = size;
    
    return CreateBuffer(info, data);
}

BufferObjectPtr CreateIndexBuffer(uint32_t size, const void* data, StringView name)
{
    BufferCreationInfo info;
    info.allowCopies = true;
    info.allowDynamicUpdate = true;
    info.allowIndex = true;
    info.label = StringBuf(name);
    info.size = size;

    return CreateBuffer(info, data);
}

//--

template< typename T >
static T ReadStream(const T*& stream, uint32_t stride)
{
    auto ret = stream[0];
    stream = OffsetPtr(stream, stride);
    return ret;
}

template< typename T >
static void WriteStream(T*& stream, uint32_t stride, const T& value)
{
    stream[0] = value;
    stream = OffsetPtr(stream, stride);
}

void GenerateCubeVertices(Vector3* outVertices, uint32_t vertexStride, Vector3 extents)
{
    Vector3 corners[8];
    Box(-extents, extents).corners(corners);

    const uint16_t indices[] = { 2,1,0,2,3,1, 6,4,5,7,6,5, 4,2,0,6,2,4, 3,5,1,7,5,3, 6,3,2,6,7,3,  1,4,0,5,4,1 };

    for (auto index : indices)
        WriteStream(outVertices, vertexStride, corners[index]);
}

void GenerateNormals(const Vector3* vertices, uint32_t vertexStride, Vector3* normals, uint32_t normalStride, uint32_t numTriangles)
{
    for (uint32_t i = 0; i < numTriangles; ++i)
    {
        const auto a = ReadStream(vertices, vertexStride);
        const auto b = ReadStream(vertices, vertexStride);
        const auto c = ReadStream(vertices, vertexStride);

        const auto n = TriangleNormal(a, b, c);

        WriteStream(normals, normalStride, n);
        WriteStream(normals, normalStride, n);
        WriteStream(normals, normalStride, n);
    }
}

//--

END_BOOMER_NAMESPACE_EX(gpu)

