/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\trimesh #]
***/

#include "build.h"
#include "triMesh.h"
#include "triMeshKdTree.h"
#include "mathShapes.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(TriMesh);
    RTTI_PROPERTY(m_nodes);
    RTTI_PROPERTY(m_triangles);
    RTTI_PROPERTY(m_bounds);
RTTI_END_TYPE();

//---

namespace helper
{

    struct QuantizationParams
    {
        QuantizationParams(const Box& quantizationBox)
        {
            m_offset = quantizationBox.min;

            auto scale  = quantizationBox.max - quantizationBox.min;
            m_scale.x = scale.x > 0.0f ? 65535.0f / scale.x : 0.0f;
            m_scale.y = scale.y > 0.0f ? 65535.0f / scale.y : 0.0f;
            m_scale.z = scale.z > 0.0f ? 65535.0f / scale.z : 0.0f;
        }

        INLINE void quantize(const Vector3& pos, uint16_t* outValues) const
        {
            outValues[0] = (uint16_t)std::clamp((pos.x - m_offset.x) * m_scale.x, 0.0f, 65535.0f);
            outValues[1] = (uint16_t)std::clamp((pos.y - m_offset.y) * m_scale.y, 0.0f, 65535.0f);
            outValues[2] = (uint16_t)std::clamp((pos.z - m_offset.z) * m_scale.z, 0.0f, 65535.0f);
        }

    private:
        Vector3 m_offset;
        Vector3 m_scale;
    };

    static void CopyNode(const QuantizationParams& q, const kd::TreeNode* sourceNodes, const TriMeshBuildTriangle* sourceTriangles, TriMeshNode* destNodes, int nodeIndex, Box& outBox)
    {
        auto& sourceNode = sourceNodes[nodeIndex];
        auto& destNode = destNodes[nodeIndex];

        Box localBox;
        if (sourceNode.childIndex == -1)
        {
            destNode.childIndex = sourceNode.firstTriangle;
            destNode.triangleCount = sourceNode.numTriangles;

            // compute bounding box of the node
            for (uint32_t i=0; i<sourceNode.numTriangles; ++i)
            {
                auto& tri = sourceTriangles[i + sourceNode.firstTriangle];
                localBox.merge(tri.v0);
                localBox.merge(tri.v1);
                localBox.merge(tri.v2);
            }
        }
        else
        {
            destNode.childIndex = sourceNode.childIndex;
            destNode.triangleCount = 0;

            // recurse to children
            CopyNode(q, sourceNodes, sourceTriangles, destNodes, sourceNode.childIndex + 0, localBox);
            CopyNode(q, sourceNodes, sourceTriangles, destNodes, sourceNode.childIndex + 1, localBox);
        }

        // write the quantized box
        q.quantize(localBox.min, destNode.boxMin);
        q.quantize(localBox.max, destNode.boxMax);

        // merge box into parent
        outBox.merge(localBox);
    }

    static void CopyTriangles(const TriMeshBuildTriangle* sourceTriangles, TriMeshTriangle* destTriangles, uint32_t firstTriangles, uint32_t numTriangles)
    {
        Box localBox;
        for (uint32_t i=0; i<numTriangles; ++i)
        {
            auto &tri = sourceTriangles[firstTriangles + i];
            localBox.merge(tri.v0);
            localBox.merge(tri.v1);
            localBox.merge(tri.v2);
        }

        QuantizationParams q(localBox);

        for (uint32_t i=0; i<numTriangles; ++i)
        {
            auto& src = sourceTriangles[firstTriangles+i];
            auto& dest = destTriangles[firstTriangles+i];

            q.quantize(src.v0, dest.v0);
            q.quantize(src.v1, dest.v1);
            q.quantize(src.v2, dest.v2);
            //dest.v0 = src.v0;
            //dest.v1 = src.v1;
            //dest.v2 = src.v2;
            dest.userData = src.userData;
        }
    }

    static void CopyNodeTriangles(const kd::TreeNode* sourceNodes, const TriMeshBuildTriangle* sourceTriangles, uint32_t numNodes, TriMeshTriangle* destTriangles)
    {
        for (uint32_t i=0; i<numNodes; ++i)
        {
            auto &src = sourceNodes[i];
            if (src.childIndex == -1)
            {
                CopyTriangles(sourceTriangles, destTriangles, src.firstTriangle, src.numTriangles);
            }
        }
    }

} // helper

//---

TriMesh::TriMesh()
{}

void TriMesh::build(TriMeshBuildTriangle* triangles, uint32_t numTriangles)
{
    // compute vertex bounds
    Box bounds;
    for (uint32_t i = 0; i < numTriangles; ++i)
    {
        bounds.merge(triangles[i].v0);
        bounds.merge(triangles[i].v1);
        bounds.merge(triangles[i].v2);
    }

    // prepare builder
    Array<kd::TreeNode> nodes;
    kd::TreeBuilder builder(triangles, numTriangles, nodes);

    // build
    kd::TreeBuildingSetup setup;
    setup.numTrianglesPerNode = 16;
    builder.build(setup);

    // setup quantization helper
    helper::QuantizationParams quantizer(bounds);

    // copy nodes
    if (auto nodeData = Buffer::Create(POOL_TRIMESH, sizeof(TriMeshNode) * nodes.size()))
    {
        auto writeNode  = (TriMeshNode *) nodeData.data();
        Box localBox;
        helper::CopyNode(quantizer, nodes.typedData(), triangles, writeNode, 0, localBox);
        m_nodes = std::move(nodeData);
    }

    // copy triangles, use local quantization within the nodes
    if (auto triangleData = Buffer::Create(POOL_TRIMESH, sizeof(TriMeshTriangle) * numTriangles))
    {
        auto writeTri  = (TriMeshTriangle *) triangleData.data();
        helper::CopyNodeTriangles(nodes.typedData(), triangles, nodes.size(), writeTri);
        m_triangles = std::move(triangleData);
    }

    // write data
    m_bounds = bounds;
    m_quantizationScale.x = (bounds.max.x - bounds.min.x) / 65536.0f;
    m_quantizationScale.y = (bounds.max.y - bounds.min.y) / 65536.0f;
    m_quantizationScale.z = (bounds.max.z - bounds.min.z) / 65536.0f;
}

//---

bool TriMesh::contains(const Vector3& point) const
{
    return false;
}

struct Dequantizer
{
    Dequantizer(const Box& bounds, const Vector3& scale)
        : m_offset(bounds.min)
        , m_scale(scale)
    {}

    Dequantizer(const Vector3& boxMin, const Vector3& boxMax)
        : m_offset(boxMin)
    {
        m_scale.x = (boxMax.x - boxMin.x) / 65536.0f;
        m_scale.y = (boxMax.y - boxMin.y) / 65536.0f;
        m_scale.z = (boxMax.z - boxMin.z) / 65536.0f;
    }

    INLINE Vector3 dequantize(const uint16_t* v) const
    {
        return Vector3(
            m_offset.x + (float)v[0] * m_scale.x,
            m_offset.y + (float)v[1] * m_scale.y,
            m_offset.z + (float)v[2] * m_scale.z);
    }

private:
    Vector3 m_offset;
    Vector3 m_scale;
};

bool TriMesh::intersect(const Vector3& origin, const Vector3& direction, float maxLength, float* outEnterDistFromOrigin, Vector3* outEntryPoint, Vector3* outEntryNormal) const
{
    Dequantizer treeDQ(m_bounds, m_quantizationScale);

    Vector3 invDir;
    invDir.x = 1.0f / direction.x;
    invDir.y = 1.0f / direction.y;
    invDir.z = 1.0f / direction.z;

    auto localMaxLength  = maxLength;
    auto localNormal  = Vector3::ZERO();
    auto ret  = false;

    auto numNodes  = this->numNodes();
    auto node  = (const TriMeshNode*) m_nodes.data();

    // TODO: actual tree
    for (uint32_t j=0; j<numNodes; ++j, ++node)
    {
        if (node->triangleCount == 0)
            continue;

        auto boxMin  = treeDQ.dequantize(node->boxMin);
        auto boxMax  = treeDQ.dequantize(node->boxMax);

        if (IntersectBoxRay(origin, direction, invDir, localMaxLength, boxMin, boxMax))
        {
            Dequantizer nodeDQ(boxMin, boxMax);

            auto numTriangles  = node->triangleCount;
            auto tri  = (const TriMeshTriangle*) m_triangles.data() + node->childIndex;
            for (uint32_t i=0; i<numTriangles; ++i, ++tri)
            {
                auto v0  = nodeDQ.dequantize(tri->v0);
                auto v1  = nodeDQ.dequantize(tri->v1);
                auto v2  = nodeDQ.dequantize(tri->v2);

                if (IntersectTriangleRay(origin, direction, v0, v1, v2, localMaxLength, &localMaxLength))
                {
                    localNormal = TriangleNormal(v0, v1, v2);
                    ret = true;
                }
            }
        }
    }

    if (ret)
    {
        if (outEnterDistFromOrigin)
            *outEnterDistFromOrigin = localMaxLength;

        if (outEntryPoint)
            *outEntryPoint = origin + localMaxLength*direction;

        if (outEntryNormal)
            *outEntryNormal = localNormal;

        return true;
    }

    return false;
}

//---

END_BOOMER_NAMESPACE()
