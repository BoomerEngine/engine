/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh\trimesh #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

/// collision triangle
#pragma pack( push, 4 )
struct TriMeshTriangle
{
    uint16_t v0[3]; // quantized using node bounds
    uint16_t v1[3]; // quantized using node bounds
    uint16_t v2[3]; // quantized using node bounds
    uint16_t userData; // user payload (usually material + flags)
};

/// collision node
struct TriMeshNode
{
    uint16_t boxMin[3]; // quantized using original bounds
    uint16_t boxMax[3]; // quantized using original bounds
    uint32_t childIndex;
    uint32_t triangleCount;
};
#pragma pack(pop)

//---

struct TriMeshBuildTriangle
{
    Vector3 v0;
    Vector3 v1;
    Vector3 v2;
    uint16_t userData = 0;

    Vector3 center; // computed
};

//---

/// triangle mesh shape, owns all the data
class CORE_MATH_API TriMesh
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(TriMesh);

public:
    TriMesh();

    //--

    // get the internal bounds of the triangles
    INLINE const Box& bounds() const { return m_bounds; }

    // get size of all data in this shape
    INLINE uint32_t datasize() const { return m_nodes.size() + m_triangles.size(); }

    // get number of triangles
    INLINE uint32_t numTriangles() const { return m_triangles.size() / sizeof(TriMeshTriangle); }

    // get number of nodes
    INLINE uint32_t numNodes() const { return m_nodes.size() / sizeof(TriMeshNode); }

    //--

    // check if this mesh contains a given point
    // NOTE: this is EXPENSIVE check and assumes that the mesh is WATER TIGHT
    // NOTE: can give BS results for points around the very boundary of the mesh
    bool contains(const Vector3& point) const;

    // intersect this convex shape with ray, returns distance to point of entry
    bool intersect(const Vector3& origin, const Vector3& direction, float maxLength = VERY_LARGE_FLOAT, float* outEnterDistFromOrigin = nullptr, Vector3* outEntryPoint = nullptr, Vector3* outEntryNormal = nullptr) const;

    //--

    // build from gathered triangle list
    // NOTE: may take some time, data is copied internally
    // NOTE: the data buffer must be writable for the duration of the build (some operations are done in-place)
    void build(TriMeshBuildTriangle* triangles, uint32_t numTriangles);

public:
    Buffer m_nodes;
    Buffer m_triangles;
    Box m_bounds;
    Vector3 m_quantizationScale;
};

//---

END_BOOMER_NAMESPACE()
