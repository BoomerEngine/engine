/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#include "build.h"
#include "meshConnectivity.h"

namespace base
{
    namespace mesh
    {

        MeshConnectivity::MeshConnectivity()
        {
            m_points.reserve(64 * 1024);
            m_edges.reserve(256 * 1024);
            m_faces.reserve(128 * 1024);

            m_pointHashMap.resizeWith(65536, -1);
            ASSERT(base::IsPow2(m_pointHashMap.size()));
        }

        void MeshConnectivity::clear()
        {
            m_points.reset();
            m_edges.reset();
            m_faces.reset();

            for (auto& it : m_pointHashMap)
                it = -1;
        }

        void MeshConnectivity::rehash()
        {
            // recreate the bucket list
            auto newSize = m_pointHashMap.size() * 2;
            ASSERT(base::IsPow2(newSize));

            m_pointHashMap.clear();
            m_pointHashMap.resizeWith(newSize, -1);
            auto buckets  = m_pointHashMap.typedData();

            // reinsert points
            uint32_t numPoints = m_points.size();
            auto point  = m_points.typedData();
            for (uint32_t i=0; i<numPoints; ++i, ++point)
            {
                auto bucketIndex = point->hash & (newSize - 1);
                point->nextPoint = buckets[bucketIndex];
                buckets[bucketIndex] = i;
            }
        }

        MeshConnectivity::PointID MeshConnectivity::mapPoint(const base::Vector3& pos)
        {
            // rehash ?
            if (m_points.size() >= m_pointHashMap.size())
                rehash();

            // add to map
            auto hash = ComputePointHash(pos);
            auto bucketIndex = hash & (m_pointHashMap.size() - 1);

            // try to use existing one
			auto pointId = m_pointHashMap.typedData()[bucketIndex];
            while (pointId != -1)
            {
				auto& point = m_points.typedData()[pointId];
                if (point.point == pos)
                    return pointId;

                pointId = point.nextPoint;
            }

            // create new point
            pointId = m_points.size();
            auto& point = m_points.emplaceBack();
            point.hash = hash;
            point.point = pos;
            point.edgeList = -1;
			point.nextPoint = m_pointHashMap.typedData()[bucketIndex];
            m_pointHashMap[bucketIndex] = pointId;

            return pointId;
        }

        MeshConnectivity::EdgeID MeshConnectivity::mapEdge(PointID a, PointID b, FaceID face)
        {
            // look for edges from A to B, if found it means it's a duplicated edge
            EdgeID existingEdgeId = -1;
            auto curEdgeId = m_points[a].edgeList; // iterate edges at point A
            while (curEdgeId != -1)
            {
                auto& edge = m_edges[curEdgeId];
                ASSERT(edge.startPoint == a);
                if (edge.endPoint == b)
                {
                    // the A->B edge already exists, duplicate it
                    existingEdgeId = curEdgeId;
                    break;
                }

                curEdgeId = edge.nextPointEdge; // go to next edge at point
            }

            // create new edge
            auto edgeId = m_edges.size();
            auto& edge = m_edges.emplaceBack();
            edge.startPoint = a;
            edge.endPoint = b;
            edge.face = face;

            // register edge in point
            edge.nextPointEdge = m_points[a].edgeList;
            m_points[a].edgeList = edgeId;

            // register duplicate, all duplicated edges form a double linked list
            if (existingEdgeId != -1)
            {
                edge.nextDuplicatedEdge = existingEdgeId;
                m_edges[existingEdgeId].prevDuplicatedEdge = edgeId;
            }

            // look for any a reverse edge in the point B
            {
                auto& otherPoint = m_points[b];
                auto otherEdgeId = otherPoint.edgeList;
                while (otherEdgeId != -1)
                {
                    auto& otherEdge = m_edges[otherEdgeId];
                    ASSERT(otherEdge.startPoint == b);
                    if (otherEdge.endPoint == a)
                    {
                        // if the opposite edge already has a partner than we are dealing with a duplicate
                        if (otherEdge.oppositeEdge == -1)
                        {
                            edge.oppositeEdge = otherEdgeId;
                            otherEdge.oppositeEdge = edgeId;
                            break;
                        }
                    }
                    otherEdgeId = otherEdge.nextPointEdge;
                }
            }

            // return edge
            return edgeId;
        }

        MeshConnectivity::FacesAtPointIterator MeshConnectivity::collectFacesAtPoint(PointID a, FaceID originalFace) const
        {
            auto& point = m_points[a];
            auto edgeId = point.edgeList;
            return FacesAtPointIterator(edgeId, m_edges.typedData());
        }

        MeshConnectivity::FaceID MeshConnectivity::mapFace(PointID a, PointID b, PointID c)
        {
            // don't map degenerated faces
            if (a == b || b == c || c == a)
                return -1;

            // create face
            FaceID faceId = m_faces.size();
            auto& face = m_faces.emplaceBack();

            // create edges
            EdgeID edgeAB = mapEdge(a, b, faceId);
            EdgeID edgeBC = mapEdge(b, c, faceId);
            EdgeID edgeCA = mapEdge(c, a, faceId);

            // link edges in face
            face.firstEdge = edgeAB;
            m_edges[edgeAB].nextFaceEdge = edgeBC;
            m_edges[edgeBC].nextFaceEdge = edgeCA;
            m_edges[edgeCA].nextFaceEdge = edgeAB;

            return faceId;
        }

    } // content
} // rendering