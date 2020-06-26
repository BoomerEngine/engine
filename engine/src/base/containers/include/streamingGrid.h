/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: streaming #]
***/

#pragma once

#include "base/containers/include/bitPool.h"

namespace base
{
    namespace containers
    {

        //---

        /// Grid debug information
        struct StreamingGridDebugInfo
        {
            //--

            struct ElemInfo
            {
                float x,y,r;
            };

            //--

            // number of used buckets on this level
            uint32_t numBuckets;

            // number of used cells on this level
            uint32_t numCells;

            // number of elements in the cells
            uint32_t numElements;

            // number of elements that are not allocated (not including the last bucket)
            uint32_t m_numWastedElements;

            // all elements
            Array<ElemInfo> m_elems;

            // occupancy histogram
            uint32_t m_gridSize;
            uint32_t m_gridMaxElemCount;
            Array<uint32_t> m_gridElemCount;

            INLINE StreamingGridDebugInfo()
                : numBuckets(0)
                , numCells(0)
                , numElements(0)
                , m_numWastedElements(0)
                , m_gridSize(0)
                , m_gridMaxElemCount(0)
            {}
        };

        //---

        /// data collector for streaming grid
        class BASE_CONTAINERS_API StreamingGridCollector
        {
        public:
            struct Elem
            {
                uint32_t m_elem; // Id of the elmenet
                uint32_t m_distanceKey; // Distance estimation
            };

            StreamingGridCollector(Elem* elements, uint32_t capacity);

            // sort object by distance
            void sort();

            INLINE void add(uint32_t element, uint64_t distance)
            {
                if (m_numElements < m_maxElemenets)
                {
                    m_elements[m_numElements].m_elem = element;
                    m_elements[m_numElements].m_distanceKey = (uint32_t)(distance >> 1);
                    m_numElements += 1;
                }
            }

            INLINE uint32_t size() const
            {
                return m_numElements;
            }

            INLINE uint32_t operator[](uint32_t index) const
            {
                return m_elements[index].m_elem;
            }

        private:
            Elem* m_elements;
            uint32_t m_numElements;
            uint32_t m_maxElemenets;
        };

        //---

        // object identification inside the grid
        typedef uint32_t StreamingGridObjectID;

        /// hierarchical (multi-level) streaming grid
        class BASE_CONTAINERS_API StreamingGrid : public NoCopy
        {
        public:
            StreamingGrid(uint32_t numLevels, uint32_t numBuckets);
            ~StreamingGrid();

            //--

            //! get number of levels in the grid
            INLINE uint32_t numLevels() const { return m_numLevels; }

            //! get the maximum number of buckets in the grid
            INLINE uint32_t maxBuckets() const { return m_bucketIDs.capacity(); }

            //! get number of allocated buckets in the grid (occupancy)
            INLINE uint32_t numBuckets() const { return m_bucketIDs.size(); }


            //! get debug information about streaming grid at given level
            void debugInfo(uint32_t level, StreamingGridDebugInfo& outData) const;

            //--

            //! register object in the grid
            StreamingGridObjectID registerObject(uint16_t x, uint16_t y, uint16_t z, uint16_t radius, uint32_t data);

            //! move object to a new position in streaming grid
            //! NOTE: returns new ID (the ID changes when object is moved)
            StreamingGridObjectID moveObject(StreamingGridObjectID hash, uint16_t newX, uint16_t newY, uint16_t newZ, uint32_t data);

            //! unregister object from the grid based on it's ID and data
            void unregisterObject(StreamingGridObjectID id, uint32_t data);

            //! unregister object from the grid using its position
            void unregisterObject(uint16_t x, uint16_t y, uint16_t z, uint16_t radius, uint32_t data);

            //! Collect entries in streaming range for given position
            //! Entries are returned using provided collector
            void collectForPoint(uint16_t x, uint16_t y, uint16_t z, class StreamingGridCollector& outCollector) const;

            //! Collect entries in 2D area
            //! Entries are returned using provided collector
            void collectForArea(uint16_t minX, uint16_t minY, uint16_t maxX, uint16_t maxY, class StreamingGridCollector& outCollector) const;

            //! Collect all registered object
            void collectAll(class StreamingGridCollector& outCollector) const;

        private:
            typedef uint32_t  TBucketIndex;

#pragma pack(push)
#pragma pack(1)
            struct GridElement // 10 bytes
            {
                uint16_t m_x; // quantized world space position (X)
                uint16_t m_y; // quantized world space position (Y)
                uint16_t m_z; // quantized world space position (Z)
                uint16_t m_radius; // quantized radius
                uint32_t m_data; // user data
            };

            struct GridBucket // 64 bytes - cache line
            {
                static const uint32_t MAX = 5;

                GridElement m_elems[MAX]; // elements
                uint32_t m_nextBucket : 24; // next bucket index
                uint32_t m_elemCount : 8; // number of elements in this bucket
            };

            struct GridNode
            {
                TBucketIndex m_bucket; // first bucket index
                uint16_t m_bucketCount; // number of buckets in this node
            };

            struct GridLevel
            {
                GridNode* m_nodes;
                uint16_t m_levelSize;
                uint32_t m_nodeCount;
            };

            static_assert(64 == sizeof(GridBucket), "Structure is expected to have a specific size");
#pragma pack(pop)

            // grid levels
            uint32_t m_numLevels;
            GridLevel* m_levels;

            // all grid cells
            uint32_t m_numNodes;
            GridNode* m_nodes;

            // grid buckets
            GridBucket* m_buckets;
            BitPool<> m_bucketIDs;
            uint32_t m_numBuckets;

            // initialize data structure
            void createBuckets(uint32_t numBuckets);
            void createGrid(uint32_t numLevels);
        };

    } // streaming
} // base