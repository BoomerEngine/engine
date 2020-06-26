/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

#include "mesh.h"
#include "meshStreamIterator.h"

namespace base
{
    namespace mesh
    {

        //--

        struct MeshModelChunk;

        // helper class for building mesh chunks
        class BASE_GEOMETRY_API MeshStreamBuilder : public base::NoCopy
        {
        public:
            MeshStreamBuilder(MeshTopologyType topology = MeshTopologyType::Triangles);
            MeshStreamBuilder(const MeshStreamBuilder& source); // make a copy
            ~MeshStreamBuilder();

            //--

            // maximum number of vertices we can output here
            INLINE uint32_t maxVertices() const { return m_maxVeritces; }

            // maximum number of indices we can output here (0 for unpacked geometry)
            //INLINE uint32_t maxIndices() const { return m_maxIndices; }

            // topology of the chunk
            INLINE MeshTopologyType topology() const { return m_topology; }

            // currently allocated streams
            INLINE MeshStreamMask streams() const { return m_validStreams; }

            // number of vertices we consider valid (must be <- maxVertices)
            uint32_t numVeritces = 0;

            // number of indices we consider valid (must be <= maxIndices)
            //uint32_t numIndices = 0;

            // is this indexed chunk ?
            //INLINE bool indexed() const { return m_indicesRaw; }

            // get pointer to index data
            //INLINE uint32_t* indexData() { return m_indicesRaw; }

            // get pointer to index data
            //INLINE const uint32_t* indexData() const { return m_indicesRaw; }

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
            void bind(const MeshModelChunk& chunk, MeshStreamMask meshStreamMask = INDEX_MAX64);

            /// resize mesh to support given number of vertices - makes all streams resident
            void reserveVertices(uint32_t maxVertices, MeshStreamMask newStreamsToAllocate);

            /// resize mesh to support given number of indices - makes the index buffer resident
            //void reserveIndices(uint32_t maxIndices);

            //--

            // make index buffer writable (make copy if we don't already have it)
            //void makeIndexBufferResident();

            // make given vertex stream resident (make a data copy if we don't already have it)
            void makeVertexStreamResident(MeshStreamType type);

            // remove index buffer by duplicating vertices, than delete it
            //void removeIndexBufferAndDuplicateVertices();

            // triangulate quads into triangles (requires index buffer and it's index only operation)
            void expandQuadsToTriangles();

            // generate automatic index buffer for triangle list
            //void generateAutomaticIndexBuffer(base::mesh::MeshTopologyType top, bool flipFaces);

            //--

            /// export data back into the chunk
            void extract(MeshModelChunk& outChunk) const;

            //--

        private:
            static const uint32_t MAX_STREAMS = 64;

            uint32_t m_maxVeritces = 0;
            //uint32_t m_maxIndices = 0;

            void* m_verticesRaw[MAX_STREAMS];
            //uint32_t* m_indicesRaw = nullptr;

            MeshTopologyType m_topology = MeshTopologyType::Triangles;
            MeshStreamMask m_validStreams = 0;
            MeshStreamMask m_ownedStreams = 0;
            //bool m_ownedIndices = false;

            void migrate(MeshStreamBuilder&& source);
        };

        //--

    } // content
} // rendering
