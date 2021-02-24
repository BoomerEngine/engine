/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: math\convex #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//---

#pragma pack( push, 4 )
/// internal edge descriptor for 3D convex hull
struct ConvexEdge
{
    short next = 0;
    short reverse = 0;
    short targetVertexIndex = 0;
    short padding = 0; // NEEDED

    //! get source vertex of this edge
    INLINE int sourceVertex() const { return (this + reverse)->targetVertexIndex; }

    //! get target vertex of this edge
    INLINE int targetVertex() const { return targetVertexIndex; }

    //! Counter-clockwise list of all edges of a vertex
    INLINE const ConvexEdge* nextEdgeOfVertex() const { return this + next; }

    //! Clockwise list of all Edge of a face
    INLINE const ConvexEdge* nextEdgeOfFace() const { return (this + reverse)->nextEdgeOfVertex(); }

    //! Get the same edge but from the other side
    INLINE const ConvexEdge* reverseEdge() const { return this + reverse; }
};
#pragma pack(pop)

//---

/// 3D convex hull
class BASE_MATH_API Convex 
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Convex);

public:
    Convex();
    Convex(const Convex& other) = default;
    Convex(Convex&& other) = default;
    Convex& operator=(const Convex& other) = default;
    Convex& operator=(Convex&& other) = default;

    //--

    /// vertices are stored as 3D points
    typedef Vector3 Vertex;

    /// plane is just a normal plane
    typedef Plane Plane;

    /// faces are stored as the indices to first edges
    typedef uint16_t Face;

    //--

    /// is this a empty convex shape ?
    INLINE bool empty() const { return m_data.empty(); }

    /// is this a valid convex shape ?
    INLINE bool valid() const { return !m_data.empty(); }

    /// get size of the data stored in the convex hull
    INLINE uint32_t dataSize() const { return m_data.size(); }

    /// get number of vertices
    INLINE uint32_t numVertices() const { return valid() ? header().numVertices : 0; }

    /// get number of edged
    INLINE uint32_t numEdges() const { return valid() ? header().numEdges : 0; }

    /// get number of faces
    INLINE uint32_t numFaces() const { return valid() ? header().numFaces : 0; }

    /// get number of planes
    INLINE uint32_t numPlanes() const { return valid() ? header().numPlanes : 0; }

    /// get raw vertices
    INLINE const Vertex* vertices() const { return valid() ? (const Vertex*)base::OffsetPtr(m_data.data(), header().vertexOffset) : nullptr; }

    /// get raw edges
    INLINE const ConvexEdge* edges() const { return valid() ? (const ConvexEdge*)base::OffsetPtr(m_data.data(), header().edgeOffset) : nullptr; }

    /// get raw planes
    INLINE const Plane* planes() const { return valid() ? (const Plane*)base::OffsetPtr(m_data.data(), header().planeOffset) : nullptr; }

    /// get raw faces
    INLINE const Face* faces() const { return valid() ? (const Face*)base::OffsetPtr(m_data.data(), header().faceOffset) : nullptr; }

    //--

    // compute volume of the shape
    float volume() const;

    // compute bounding box of the shape in the coordinates of the shape
    Box bounds() const;

    // check if this convex hull contains a given point
    bool contains(const Vector3& point) const;

    // intersect this convex shape with ray, returns distance to point of entry
    bool intersect(const Vector3& origin, const Vector3& direction, float maxLength = VERY_LARGE_FLOAT, float* outEnterDistFromOrigin = nullptr, Vector3* outEntryPoint = nullptr, Vector3* outEntryNormal = nullptr) const;

    // build from list of points
    bool build(const float* points, uint32_t numPoints, uint32_t stride, float shinkBy = 0.0f);

    //--

private:
    Buffer m_data;

    struct Header
    {
        uint16_t numVertices;
        uint16_t vertexOffset;
        uint16_t numEdges;
        uint16_t edgeOffset;
        uint16_t numFaces;
        uint16_t faceOffset;
        uint16_t numPlanes;
        uint16_t planeOffset;
    };

    INLINE const Header& header() const { return *(const Header*)m_data.data(); }
};

//---

END_BOOMER_NAMESPACE(base)