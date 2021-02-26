/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: streams #]
***/

#pragma once

#include "renderingMeshStreamData.h"
#include "renderingMeshStreamIterator.h"

BEGIN_BOOMER_NAMESPACE()

//--

struct MeshRawChunk;

// helper class for building mesh chunks - basically SOA with extra steps
class ENGINE_MESH_API MeshRawStreamBuilder : public NoCopy
{
public:
    MeshRawStreamBuilder(MeshTopologyType topology = MeshTopologyType::Triangles);
    MeshRawStreamBuilder(const MeshRawStreamBuilder& source); // make a copy
    ~MeshRawStreamBuilder();

    //--

    // maximum number of vertices we can output here
    INLINE uint32_t maxVertices() const { return m_maxVeritces; }

    // topology of the chunk
    INLINE MeshTopologyType topology() const { return m_topology; }

    // currently allocated streams
    INLINE MeshStreamMask streams() const { return m_validStreams; }

    // number of vertices we consider valid (must be <- maxVertices)
    uint32_t numVeritces = 0;

    //---

    // get typed pointer to vertex
    template< MeshStreamType ST >
    INLINE typename MeshStreamTypeDataResolver<ST>::TYPE* vertexData() { return (typename MeshStreamTypeDataResolver<ST>::TYPE*) m_verticesRaw[(uint32_t)ST]; }

    // get typed pointer to vertex
    template< MeshStreamType ST >
    INLINE typename const MeshStreamTypeDataResolver<ST>::TYPE* vertexData() const { return (typename MeshStreamTypeDataResolver<ST>::TYPE*) m_verticesRaw[(uint32_t)ST]; }

    //--

    /// release all data
    void clear();

    /// load existing chunk into the unpacker, DOES NOT copy any data, good for read only operations
    /// NOTE: if you wish to modify the data you must make it resident first or resize the buffer
    void bind(const MeshRawChunk& chunk, MeshStreamMask meshStreamMask = INDEX_MAX64);

    /// resize mesh to support given number of vertices - makes all streams resident
    void reserveVertices(uint32_t maxVertices, MeshStreamMask newStreamsToAllocate);

    //--

    // make given vertex stream resident (make a data copy if we don't already have it)
    void makeVertexStreamResident(MeshStreamType type);

    // triangulate quads into triangles (requires index buffer and it's index only operation)
    void expandQuadsToTriangles();

    //--

    /// export data back into the chunk
    void extract(MeshRawChunk& outChunk) const;

    //--

private:
    static const uint32_t MAX_STREAMS = 64;

    uint32_t m_maxVeritces = 0;

    void* m_verticesRaw[MAX_STREAMS];

    MeshTopologyType m_topology = MeshTopologyType::Triangles;
    MeshStreamMask m_validStreams = 0;
    MeshStreamMask m_ownedStreams = 0;

    void migrate(MeshRawStreamBuilder&& source);
};

//--

END_BOOMER_NAMESPACE()
