/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: wavefront\utils #]
***/

#pragma once

namespace wavefront
{
    /// helper class to generate merged vertex buffers
    template< typename V >
    class MeshVertexCache
    {
    public:
        // create empty cache, it's preallocated to fit most use cases
        MeshVertexCache()
        {
            m_maxVerticesInGroup = 0;
            m_groupLimitReached = true;
            m_vertices.reserve(128*1024);
        }

        // get generated vertices
        INLINE const base::Array<V>& vertices() const
        {
            return m_vertices;
        }

        // reset (removes all vertices)
        INLINE void reset()
        {
            m_vertices.clear();
        }

        // start new batch, returns first vertex in this group in the vertex buffer
        uint32_t beingGroup(uint32_t maxVerticesInGroup = 65536)
        {
            m_firstVertex = m_vertices.size();
            m_maxVerticesInGroup = maxVerticesInGroup;
            m_groupLimitReached = false;
            m_hashEntries.reset();

            for (uint32_t i=0; i<MAX_BUCKETS; ++i)
                m_hashBuckets[i] = INDEX_NONE;

            return m_firstVertex;
        }

        // finish group, returns number of vertices added
        uint32_t endGroup()
        {
            auto ret = m_hashEntries.size();
            m_groupLimitReached = true;
            return ret;
        }

        // add triangle, returns false if the triangle cannot be added without adding to much vertices
        bool addTriangle(const V& a,const V& b, const V& c, uint32_t& outA, uint32_t& outB, uint32_t& outC)
        {
            // we are already past the group limit
            if (m_groupLimitReached)
                return false;

            // capture current count
            auto numVertices = m_vertices.size();
            auto numEntries = m_hashEntries.size();

            // add vertices
            if (addVertex(a, outA) && addVertex(b, outB) && addVertex(c, outC))
                return true;

            // revert
            m_groupLimitReached = true;
            while (m_vertices.size() > numVertices) m_vertices.popBack();
            while (m_hashEntries.size() > numEntries) m_hashEntries.popBack();

            // we cannot add more triangles
            return false;
        }

    private:
        static const uint32_t MAX_BUCKETS = 65536;

        // generated vertices
        base::Array<V> m_vertices;

        // search entries
        struct Entry
        {
            uint32_t m_index;
            int m_next;
        };

        // search table
        base::Array<Entry> m_hashEntries;
        int m_hashBuckets[MAX_BUCKETS];

        // limits
        uint32_t m_firstVertex;
        uint32_t m_maxVerticesInGroup;
        bool m_groupLimitReached;

        //--

        // add vertex to cache, returns false if the vertex could not be added
        INLINE bool addVertex(const V& v, uint32_t& outV)
        {
            // compute position only hash
            auto posHash = v.calcPositionHash();
            auto bucketIndex = (posHash % MAX_BUCKETS);

            // test vertices in the range
            auto entryIndex = m_hashBuckets[bucketIndex];
            while (entryIndex != INDEX_NONE)
            {
                auto& entry = m_hashEntries[entryIndex];
                auto& vertex = m_vertices[m_firstVertex + entry.m_index];
                if (vertex.equals(v))
                {
                    outV = entry.m_index;
                    return true;
                }

                entryIndex = entry.m_next;
            }

            // make sure we have space for the new vertex
            if (m_hashEntries.size() >= m_maxVerticesInGroup)
                return false;

            // insert new vertex
            entryIndex = m_hashEntries.size();
            auto& entry = m_hashEntries.emplaceBack();
            entry.m_index = m_vertices.size() - m_firstVertex;
            m_vertices.pushBack(v);

            // link
            entry.m_next = m_hashBuckets[bucketIndex];
            m_hashBuckets[bucketIndex] = entryIndex;

            // return vertex index
            outV = entryIndex;
            return true;
        }
    };

} // wavefront
