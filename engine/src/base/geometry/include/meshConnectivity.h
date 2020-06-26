/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

namespace base
{
    namespace mesh
    {

        /// helper class to build connectivity between faces, edges and points
        class BASE_GEOMETRY_API MeshConnectivity
        {
        public:
            typedef int EdgeID;
            typedef int PointID;
            typedef int FaceID;

            struct Point
            {
                base::Vector3 point;
                uint32_t hash = 0;
                EdgeID edgeList = -1;
                PointID nextPoint = -1; // in hashmap
            };

            struct Edge
            {
                PointID startPoint = -1; // root point of the edge
                PointID endPoint = -1; // target point of the edge
                FaceID face = -1; // face this edge belongs to
                EdgeID oppositeEdge = -1; // if we are the "A->B" this is the "B->A" vertex, can be empty
                EdgeID nextPointEdge = -1; // next edge while iterating point edges
                EdgeID nextFaceEdge = -1;  // next edge while iterating face edges
                EdgeID nextDuplicatedEdge = -1; // if there's more than once edge that is the same
                EdgeID prevDuplicatedEdge = -1; // if there's more than once edge that is the same
            };

            struct Face
            {
                EdgeID firstEdge = -1;
            };

            typedef base::Array<Point> TPoints;
            typedef base::Array<Edge> TEdges;
            typedef base::Array<Face> TFace;

            TPoints m_points;
            TEdges m_edges;
            TFace m_faces;

            //--

            base::Array<PointID> m_pointHashMap;

            MeshConnectivity();

            static INLINE uint32_t ComputePointHash(const base::Vector3& pos)
            {
                uint32_t c[3] = { *(const uint32_t*)&pos.x, *(const uint32_t*)&pos.y, *(const uint32_t*)&pos.z };
                return base::FastHash32(c, sizeof(c));
            }

            // reset data
            void clear();

            // map a single 3d position to a point in a mesh, returns unique ID
            PointID mapPoint(const base::Vector3& pos);

            // map edge between two points that belong to a given face
            EdgeID mapEdge(PointID a, PointID b, FaceID face);

            // map a face (triangle)
            FaceID mapFace(PointID a, PointID b, PointID c);

            //--

            struct FacesAtPointIterator
            {
            public:
                INLINE FacesAtPointIterator()
                {}

                INLINE FacesAtPointIterator(EdgeID edgeId, const Edge* edges)
                    : currentEdge(edgeId)
                    , edges(edges)
                {}

                INLINE FacesAtPointIterator(const FacesAtPointIterator& other) = default;

                INLINE bool valid() const
                {
                    return (currentEdge != INVALID_ID);
                }

                INLINE FaceID faceID() const
                {
                    return edges[currentEdge].face;
                }

                INLINE void operator++()
                {
                    currentEdge = edges[currentEdge].nextPointEdge;
                }

            private:
                EdgeID currentEdge = -1;
                const Edge* edges = nullptr;
            };

            FacesAtPointIterator collectFacesAtPoint(PointID a, FaceID originalFace) const;

            void rehash();
        };

    } // mesh
} // base