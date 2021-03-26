/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#pragma once

#include "core/containers/include/pagedBuffer.h"
#include "core/object/include/objectSelection.h"
#include "core/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE()

///---

class DebugDrawer;

///---

/// simple rendering vertex, used in the simple rendering :)
/// NOTE: we don't have and don't want tangent space here as most of this crap has no lighting any way
struct DebugVertex
{
    static const uint32_t FLAG_SHADE = FLAG(0);
    static const uint32_t FLAG_EDGES = FLAG(1);
    static const uint32_t FLAG_SPRITE = FLAG(2);
    static const uint32_t FLAG_LINE = FLAG(3);
    static const uint32_t FLAG_SCREEN = FLAG(4);

    Vector3 p = Vector3::ZERO();
    float w = 0.0f;
    uint32_t f = 0; 
    Vector2 t = Vector2::ZERO();
    Selectable s = Selectable();
    Color c = Color::WHITE;
};

///----

/// rendering layer for frame geometry
enum class DebugGeometryLayer : uint8_t
{
    SceneSolid, // scene fragment, rendering amongst normal geometry
    SceneTransparent, // pre-multiplied alpha transparent MESH (no lines, no sprites here)
    Overlay, // overlay fragment, rendered on top of 3D geometry (gizmos)
    Screen, // screen fragments, mostly text 

    MAX,
};

//--

struct DebugGeometryElement
{
    uint32_t firstIndex = 0;
    uint32_t numIndices = 0;
    uint32_t firstVertex = 0;
    uint32_t numVeritices = 0;
};

//---

/// single geometry chunk
class ENGINE_RENDERING_API DebugGeometryChunk : public IReferencable
{
    RTTI_DECLARE_POOL(POOL_DEBUG_GEOMETRY);

public:
    INLINE DebugGeometryLayer layer() const { return m_layer; }

    INLINE uint32_t numVertices() const { return m_numVertices; }
    INLINE DebugVertex* vertices() { return m_vertices; }
    INLINE const DebugVertex* vertices() const { return m_vertices; }

    INLINE uint32_t numIndices() const { return m_numIndices; }
    INLINE uint32_t* indices() { return m_indices; }
    INLINE const uint32_t* indices() const { return m_indices; }

    //--

    static DebugGeometryChunkPtr Create(DebugGeometryLayer layer, uint32_t numVertices, uint32_t numIndices);

    //--

private:
    DebugGeometryChunk();
    virtual ~DebugGeometryChunk();

    DebugGeometryLayer m_layer = DebugGeometryLayer::SceneSolid;

    uint32_t m_numVertices = 0;
    DebugVertex* m_vertices = nullptr;

    uint32_t m_numIndices = 0;
    uint32_t* m_indices  = nullptr;

    friend class DebugGeometryCollector;
};

//---

/// debug geometry collector
class ENGINE_RENDERING_API DebugGeometryCollector : public NoCopy
{
public:
    DebugGeometryCollector(uint32_t viewportWidth, uint32_t viewportHight, const Camera& camera);
    ~DebugGeometryCollector();

    //--

    INLINE uint32_t viewportWidth() const { return m_viewportWidth; }
    INLINE uint32_t viewportHeight() const { return m_viewportHight; }

    INLINE const Camera& camera() const { return m_camera; }

    //--

    struct Element
    {
        DebugGeometryChunkPtr chunk;
        Matrix localToWorld;
        Selectable selectableOverride;
        Element* next = nullptr;
        DebugGeometryChunk* localChunk = nullptr;
    };

    // get total counts
    void queryLimits(uint32_t& outMaxVertices, uint32_t& outMaxIndices) const;

    // list of debug elements for given layer
    void get(DebugGeometryLayer layer, const Element*& outElement, uint32_t& outCount, uint32_t& outNumVertices, uint32_t& outNumIndices) const;

    // add chunk to given layer
    void push(const DebugGeometryChunk* chunk, const Matrix& placement, const Selectable& selectable = Selectable());

    // add chunk to given layer
    void push(const DebugGeometryBuilderBase& data, const Matrix& placement = Matrix::IDENTITY(), const Selectable& selectable = Selectable());

    //--

private:
    static PageAllocator st_PageAllocator;

    LinearAllocator m_allocator;

    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHight = 0;
    Camera m_camera;

    void clear();

    struct Layer
    {
        Element* head = nullptr;
        Element* tail = nullptr;
        uint32_t totalVertices = 0;
        uint32_t totalIndices = 0;
        uint32_t count = 0;
    };

    static const auto MAX_LAYERS = (int)DebugGeometryLayer::MAX;
    Layer m_layers[MAX_LAYERS];
};

//---

END_BOOMER_NAMESPACE()
