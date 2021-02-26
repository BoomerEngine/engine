/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "gpu/device/include/renderingDeviceApi.h"
#include "gpu/device/include/renderingCommandWriter.h"
#include "gpu/device/include/renderingShader.h"
#include "gpu/device/include/renderingGraphicsStates.h"
#include "gpu/device/include/renderingFramebuffer.h"
#include "gpu/device/include/renderingDescriptor.h"
#include "gpu/device/include/renderingImage.h"
#include "gpu/device/include/renderingImage.h"
#include "gpu/device/include/renderingShaderSelector.h"

#include "core/object/include/rttiMetadata.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

//---

/// create buffer upload data from an array
template< typename T >
INLINE static SourceDataProviderPtr CreateSourceData(const Array<T>& sourceData)
{
    auto buf = Buffer::Create(POOL_TEMP, sourceData.dataSize(), 1, sourceData.data());
	return RefNew<SourceDataProviderBuffer>(buf);;
}

/// create buffer upload data from an array
template< typename T >
INLINE static SourceDataProviderPtr CreateSourceDataRaw(const T& sourceData)
{
    auto buf = Buffer::Create(POOL_TEMP, sizeof(sourceData), 1, &sourceData);
	return RefNew<SourceDataProviderBuffer>(buf);;
}

//---

struct Simple2DVertex
{
    Vector2 VertexPosition;
    Vector2 VertexUV;
    Color VertexColor;

    INLINE void set(float x, float y, float u, float v, Color _color)
    {
        VertexPosition = Vector2(x, y);
        VertexUV = Vector2(u, v);
        VertexColor = _color;
    }
};

struct Simple3DVertex
{
    Vector3 VertexPosition;
    Vector2 VertexUV;
    Color VertexColor;

    INLINE Simple3DVertex()
    {}

    INLINE Simple3DVertex(float x, float y, float z, float u = 0.0f, float v = 0.0f, Color _color = Color::BLACK)
    {
        VertexPosition = Vector3(x, y, z);
        VertexUV = Vector2(u, v);
        VertexColor = _color;
    }

    INLINE void set(float x, float y, float z, float u = 0.0f, float v = 0.0f, Color _color = Color::BLACK)
    {
        VertexPosition = Vector3(x, y, z);
        VertexUV = Vector2(u, v);
        VertexColor = _color;
    }
};

struct Mesh3DVertex
{
    Vector3 VertexPosition;
    Vector3 VertexNormal;
    Vector2 VertexUV;
    Color VertexColor;

    INLINE Mesh3DVertex()
        : VertexPosition(0, 0, 0)
        , VertexNormal(0, 0, 1)
        , VertexUV(0, 0)
        , VertexColor(Color::WHITE)
    {}
};

//---

/// test parametrization
class ParamTable
{
public:
    INLINE ParamTable()
    {}

    INLINE void set(const char* name, const char* val)
    {
        m_paramTable.set(name, val);
    }

    template< typename T >
    INLINE T get(const char* name, const T& defaultValue = T()) const
    {
        auto valPtr  = m_paramTable.find(name);
        if (valPtr == nullptr)
            return defaultValue;

        typename std::remove_cv<T>::type ret = defaultValue;
        valPtr->view().match(ret);
        return ret;
    }

private:
    typedef HashMap<StringBuf, StringBuf> TParamTable;
    TParamTable m_paramTable;
};

//---

/// order of test
class RenderingTestOrderMetadata : public rtti::IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTestOrderMetadata, rtti::IMetadata);

public:
    INLINE RenderingTestOrderMetadata()
        : m_order(-1)
    {}

    RenderingTestOrderMetadata& order(int val)
    {
        m_order = val;
        return *this;
    }

    int m_order;
};

//---

/// number of sub-tests in the test
class RenderingTestSubtestCountMetadata : public rtti::IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTestSubtestCountMetadata, rtti::IMetadata);

public:
    INLINE RenderingTestSubtestCountMetadata()
        : m_count(1)
    {}

    RenderingTestSubtestCountMetadata& count(uint32_t count)
    {
        m_count = count;
        return *this;
    }

    uint32_t m_count;
};

//---

struct TextureSlice
{
    Array<image::ImagePtr> mipmaps;
    void generateMipmaps();
};

//---

struct SimpleChunk
{
    StringID material;
    uint32_t firstVertex = 0;
    uint32_t firstIndex = 0;
    uint32_t numVerties = 0;
    uint32_t numIndices = 0;
};

/// a test mesh - group of triangles
class SimpleMesh : public IReferencable
{
public:
    Array<Mesh3DVertex> m_allVertices;
    Array<uint16_t> m_allIndices;
    Array<SimpleChunk> m_chunks;

    SimpleMesh();
};

extern RefPtr<SimpleMesh> LoadSimpleMeshFromDepotPath(StringView path);

// render mesh 
struct SimpleRenderMesh : public IReferencable
{
public:
    Box m_bounds;
    Array<SimpleChunk> m_chunks;

    BufferObjectPtr m_vertexBuffer;
	BufferObjectPtr m_indexBuffer;

    void drawChunk(CommandWriter& cmd, const GraphicsPipelineObject* func, uint32_t chunkIndex) const;
    void drawMesh(CommandWriter& cmd, const GraphicsPipelineObject* func) const;
};

typedef RefPtr<SimpleRenderMesh> SimpleRenderMeshPtr;

// mesh import setup
struct MeshSetup
{
    Matrix m_loadTransform;
    bool m_swapYZ = false;
    bool m_flipFaces = false;
    bool m_flipNormal = false;

    INLINE MeshSetup()
        : m_loadTransform(Matrix::IDENTITY())
    {}
};

//---

/// a basic rendering test
class IRenderingTest : public IReferencable
{
    RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IRenderingTest);

public:
    IRenderingTest();
    virtual ~IRenderingTest();

    //--

    // did loading of any resource failed ?
    INLINE bool hasErrors() const { return m_hasErrors; }

    // rendering
    INLINE IDevice* device() const { return m_device; }

    // sub test
    INLINE uint32_t subTestIndex() const { return m_subTestIndex; }

    //--

    // prepare test
    bool prepareAndInitialize(IDevice* drv, uint32_t subTestIndex, IOutputObject* output);

    // initialize test
    virtual void initialize() = 0;

    // tear down test
    virtual void shutdown();

    // render test via the provided interface, in the interactive mode the frame index is animated
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) = 0;

	// query initial camera position
	virtual void queryInitialCamera(Vector3& outPosition, Angles& outRotation);

	// update camera 
	virtual void updateCamera(const Vector3& position, const Angles& rotation);

	// describe the current subtest 
	virtual void describeSubtest(IFormatStream& f);

    //--

	// load shaders, NOTE: uses short path based in the engine/test/shaders/ directory
    GraphicsPipelineObjectPtr loadGraphicsShader(StringView partialPath, const GraphicsRenderStatesSetup* states = nullptr, const ShaderSelector& extraSelectors = ShaderSelector());

	// load shaders, NOTE: uses short path based in the engine/test/shaders/ directory
	ComputePipelineObjectPtr loadComputeShader(StringView partialPath, const ShaderSelector& extraSelectors = ShaderSelector());

    // load a simple mesh (obj file) from disk, file should be in the engine/assets/tests/ directory
	ImageObjectPtr loadImage2D(StringView assetFile, bool createMipmaps = false, bool forceAlpha = false);
            
    // load a custom cubemap from 6 images
	ImageObjectPtr loadCubemap(StringView assetFile, bool createMipmaps = false);

    // load a simple mesh (obj file) from disk, file should be in the engine/assets/tests/ directory
    SimpleRenderMeshPtr loadMesh(StringView assetFile, const MeshSetup& setup = MeshSetup());

    //--

	// create render states
	GraphicsRenderStatesObjectPtr createRenderStates(const GraphicsRenderStatesSetup& setup);

    // create buffer
    BufferObjectPtr createBuffer(const BufferCreationInfo& info, const ISourceDataProvider* initializationData = nullptr);

	// create formatted buffer
	BufferObjectPtr createFormatBuffer(ImageFormat format, uint32_t size, bool allowUAV);

    // create vertex buffer
	BufferObjectPtr createVertexBuffer(uint32_t size, const void* sourceData); // pass NULL data to create dynamic buffer

    // create index buffer
	BufferObjectPtr createIndexBuffer(uint32_t size, const void* sourceData); // pass NULL data to create dynamic buffer

    // create storage buffer
	BufferObjectPtr createStorageBuffer(uint32_t size, uint32_t stride = 0, bool dynamic=false, bool allowVertex=false, bool allowIndex=false);

	// create constant buffer
	BufferObjectPtr createConstantBuffer(uint32_t size, const void* sourceData=nullptr, bool dynamic = false, bool allowUav = false);

    // create vertex buffer from array
    template< typename T >
    INLINE BufferObjectPtr createVertexBuffer(const Array<T>& data) { return createVertexBuffer(data.dataSize(), data.data()); }

    // create index buffer from array
    template< typename T >
    INLINE BufferObjectPtr createIndexBuffer(const Array<T> & data) { return createIndexBuffer(data.dataSize(), data.data()); }

	// create indirect buffer, accessible from compute via UAV
	BufferObjectPtr createIndirectBuffer(uint32_t size, uint32_t stride);

	//--

    // create image
    ImageObjectPtr createImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData = nullptr);

    // create texture from list of slices
	ImageObjectPtr createImage(const Array<TextureSlice>& slices, ImageViewType viewType);

    // create a 2D mipmap test, each mipmap has different color
    ImageObjectPtr createMipmapTest2D(uint16_t initialSize, bool markers = false);

    // create a 2D checker image
	ImageObjectPtr createChecker2D(uint16_t initialSize, uint32_t checkerSize, bool generateMipmaps = true, Color colorA = Color::WHITE, Color colorB = Color::BLACK);

    // create a simple flat-color test cubemap
	ImageObjectPtr createFlatCubemap(uint16_t size = 64);

    // create a simple normal color test cubemap
	ImageObjectPtr createColorCubemap(uint16_t size = 64);

    // create sampler
    SamplerObjectPtr createSampler(const SamplerState& info);

    //--

    // draw a simple quad using given shaders
    void drawQuad(CommandWriter& cmd, const GraphicsPipelineObject* func, float x=-1.0f, float y = -1.0f, float w=2.0f, float h = 2.0f, float u0=0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f, Color color = Color::WHITE);

    // configure quad params
    void setQuadParams(CommandWriter& cmd, float x, float y, float w, float h);
    void setQuadParams(CommandWriter& cmd, const RenderTargetView* rt, const Rect& rect);

    //--

    // report loading error
    bool reportError(StringView txt);

protected:
	Vector3 m_cameraPosition;
	Angles m_cameraAngles;

private:
    SpinLock m_allLoadedResourcesLock;
    Array<res::ResourcePtr> m_allLoadedResources;
            
	HashMap<uint64_t, GraphicsRenderStatesObjectPtr> m_renderStatesMap;

    bool m_hasErrors;
    uint32_t m_subTestIndex;
    IDevice* m_device;

    BufferObjectPtr m_quadVertices;

    bool loadCubemapSide(Array<TextureSlice>& outSlices, StringView assetFile, bool createMipmaps /*= false*/);
};

///---

template< typename VT = Simple3DVertex, typename IT = uint16_t >
struct VertexIndexBunch
{
    Array<VT> m_vertices;
    Array<IT> m_indices;

	BufferObjectPtr m_vertexBuffer;
	BufferObjectPtr m_indexBuffer;

    void createBuffers(IRenderingTest& owner)
    {
        if (!m_vertices.empty())
        {
            gpu::BufferCreationInfo vertexInfo;
            vertexInfo.allowVertex = true;
            vertexInfo.size = m_vertices.dataSize();

            auto sourceData = CreateSourceData(m_vertices);
            m_vertexBuffer = owner.createBuffer(vertexInfo, sourceData);
        }

        if (!m_indices.empty())
        {
            gpu::BufferCreationInfo indexInfo;
            indexInfo.allowIndex = true;
            indexInfo.size = m_indices.dataSize();

            auto sourceData = CreateSourceData(m_indices);
            m_indexBuffer = owner.createBuffer(indexInfo, sourceData);
        }
    }

    void draw(CommandWriter& cmd, const GraphicsPipelineObject* func, uint16_t firstInstance = 0, uint16_t numInstances = 1) const
    {
        if (m_indexBuffer)
        {
            cmd.opBindIndexBuffer(m_indexBuffer, ImageFormat::R16_UINT);
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
            cmd.opDrawIndexedInstanced(func, 0, 0, m_indices.size(), firstInstance, numInstances);
        }
        else
        {
            cmd.opBindVertexBuffer("Simple3DVertex"_id,  m_vertexBuffer);
            cmd.opDrawInstanced(func, 0, m_vertices.size(), firstInstance, numInstances);
        }
    }
};

///---

END_BOOMER_NAMESPACE_EX(gpu::test)
