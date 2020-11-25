/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingShaderLibrary.h"
#include "rendering/device/include/renderingFramebuffer.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingImage.h"
#include "rendering/device/include/renderingImage.h"

#include "base/object/include/rttiMetadata.h"

namespace rendering
{
    namespace test
    {
        //---

        /// create buffer upload data from an array
        template< typename T >
        INLINE static SourceDataProviderPtr CreateSourceData(const base::Array<T>& sourceData)
        {
            auto buf = base::Buffer::Create(POOL_TEMP, sourceData.dataSize(), 1, sourceData.data());
			return base::RefNew<SourceDataProviderBuffer>(buf);;
        }

        /// create buffer upload data from an array
        template< typename T >
        INLINE static SourceDataProviderPtr CreateSourceDataRaw(const T& sourceData)
        {
            auto buf = base::Buffer::Create(POOL_TEMP, sizeof(sourceData), 1, &sourceData);
			return base::RefNew<SourceDataProviderBuffer>(buf);;
        }

        //---

        struct Simple2DVertex
        {
            base::Vector2 VertexPosition;
            base::Vector2 VertexUV;
            base::Color VertexColor;

            INLINE void set(float x, float y, float u, float v, base::Color _color)
            {
                VertexPosition = base::Vector2(x, y);
                VertexUV = base::Vector2(u, v);
                VertexColor = _color;
            }
        };

        struct Simple3DVertex
        {
            base::Vector3 VertexPosition;
            base::Vector2 VertexUV;
            base::Color VertexColor;

            INLINE Simple3DVertex()
            {}

            INLINE Simple3DVertex(float x, float y, float z, float u = 0.0f, float v = 0.0f, base::Color _color = base::Color::BLACK)
            {
                VertexPosition = base::Vector3(x, y, z);
                VertexUV = base::Vector2(u, v);
                VertexColor = _color;
            }

            INLINE void set(float x, float y, float z, float u = 0.0f, float v = 0.0f, base::Color _color = base::Color::BLACK)
            {
                VertexPosition = base::Vector3(x, y, z);
                VertexUV = base::Vector2(u, v);
                VertexColor = _color;
            }
        };

        struct Mesh3DVertex
        {
            base::Vector3 VertexPosition;
            base::Vector3 VertexNormal;
            base::Vector2 VertexUV;
            base::Color VertexColor;

            INLINE Mesh3DVertex()
                : VertexPosition(0, 0, 0)
                , VertexNormal(0, 0, 1)
                , VertexUV(0, 0)
                , VertexColor(base::Color::WHITE)
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
            typedef base::HashMap<base::StringBuf, base::StringBuf> TParamTable;
            TParamTable m_paramTable;
        };

        //---

        /// order of test
        class RenderingTestOrderMetadata : public base::rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTestOrderMetadata, base::rtti::IMetadata);

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
        class RenderingTestSubtestCountMetadata : public base::rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTestSubtestCountMetadata, base::rtti::IMetadata);

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
            base::Array<base::image::ImagePtr> m_mipmaps;
            void generateMipmaps();
        };

        //---

        struct SimpleChunk
        {
            base::StringID material;
            uint32_t firstVertex = 0;
            uint32_t firstIndex = 0;
            uint32_t numVerties = 0;
            uint32_t numIndices = 0;
        };

        /// a test mesh - group of triangles
        class SimpleMesh : public base::res::IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SimpleMesh, base::res::IResource);

        public:
            base::Array<Mesh3DVertex> m_allVertices;
            base::Array<uint16_t> m_allIndices;
            base::Array<SimpleChunk> m_chunks;

            SimpleMesh();
            ~SimpleMesh();
        };


        // render mesh 
        struct SimpleRenderMesh : public base::IReferencable
        {
        public:
            base::Box m_bounds;
            base::Array<SimpleChunk> m_chunks;

            BufferObjectPtr m_vertexBuffer;
			BufferObjectPtr m_indexBuffer;

            void drawChunk(command::CommandWriter& cmd, const ShaderLibrary* func, uint32_t chunkIndex) const;
            void drawMesh(command::CommandWriter& cmd, const ShaderLibrary* func) const;
        };

        typedef base::RefPtr<SimpleRenderMesh> SimpleRenderMeshPtr;

        // mesh import setup
        struct MeshSetup
        {
            base::Matrix m_loadTransform;
            bool m_swapYZ = false;
            bool m_flipFaces = false;
            bool m_flipNormal = false;

            INLINE MeshSetup()
                : m_loadTransform(base::Matrix::IDENTITY())
            {}
        };

        //---

        /// a basic rendering test
        class IRenderingTest : public base::IReferencable
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
            bool prepareAndInitialize(IDevice* drv, uint32_t subTestIndex);

            // initialize test
            virtual void initialize() = 0;

            // tear down test
            virtual void shutdown();

            // render test via the provided interface, in the interactive mode the frame index is animated
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) = 0;

            //--

            // load shaders, NOTE: uses short path based in the engine/test/shaders/ directory
            ShaderLibraryPtr loadShader(base::StringView partialPath);

            // load a simple mesh (obj file) from disk, file should be in the engine/assets/tests/ directory
			ImageObjectPtr loadImage2D(base::StringView assetFile, bool createMipmaps = false, bool uavCapable = false, bool forceAlpha = false);
            
            // load a custom cubemap from 6 images
			ImageObjectPtr loadCubemap(base::StringView assetFile, bool createMipmaps = false);

            // load a simple mesh (obj file) from disk, file should be in the engine/assets/tests/ directory
            SimpleRenderMeshPtr loadMesh(base::StringView assetFile, const MeshSetup& setup = MeshSetup());

            //--

            // create buffer
            BufferObjectPtr createBuffer(const BufferCreationInfo& info, const ISourceDataProvider* initializationData = nullptr);

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

            // create image
            ImageObjectPtr createImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData = nullptr, bool uavCapable = false);

            // create texture from list of slices
			ImageObjectPtr createImage(const base::Array<TextureSlice>& slices, ImageViewType viewType, bool uavCapable = false);

            // create a 2D mipmap test, each mipmap has different color
            ImageObjectPtr createMipmapTest2D(uint16_t initialSize, bool markers = false);

            // create a 2D checker image
			ImageObjectPtr createChecker2D(uint16_t initialSize, uint32_t checkerSize, bool generateMipmaps = true, base::Color colorA = base::Color::WHITE, base::Color colorB = base::Color::BLACK);

            // create a simple flat-color test cubemap
			ImageObjectPtr createFlatCubemap(uint16_t size = 64);

            // create a simple normal color test cubemap
			ImageObjectPtr createColorCubemap(uint16_t size = 64);

            // create sampler
            SamplerObjectPtr createSampler(const SamplerState& info);

            //--

            // draw a simple quad using given shaders
            void drawQuad(command::CommandWriter& cmd, const ShaderLibrary* func, float x, float y, float w, float h, float u0=0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f, base::Color color = base::Color::WHITE);

            // configure quad params
            void setQuadParams(command::CommandWriter& cmd, float x, float y, float w, float h);
            void setQuadParams(command::CommandWriter& cmd, const RenderTargetView* rt, const base::Rect& rect);

            //--

            // report loading error
            bool reportError(base::StringView txt);

        private:
            base::SpinLock m_allLoadedResourcesLock;
            base::Array<base::res::BaseReference> m_allLoadedResources;
            
            bool m_hasErrors;
            uint32_t m_subTestIndex;
            IDevice* m_device;

            BufferObjectPtr m_quadVertices;

            bool loadCubemapSide(base::Array<TextureSlice>& outSlices, base::StringView assetFile, bool createMipmaps /*= false*/);
        };

        ///---

        template< typename VT = Simple3DVertex, typename IT = uint16_t >
        struct VertexIndexBunch
        {
            base::Array<VT> m_vertices;
            base::Array<IT> m_indices;

			BufferObjectPtr m_vertexBuffer;
			BufferObjectPtr m_indexBuffer;

            void createBuffers(IRenderingTest& owner)
            {
                if (!m_vertices.empty())
                {
                    rendering::BufferCreationInfo vertexInfo;
                    vertexInfo.allowVertex = true;
                    vertexInfo.size = m_vertices.dataSize();

                    auto sourceData = CreateSourceData(m_vertices);
                    m_vertexBuffer = owner.createBuffer(vertexInfo, sourceData);
                }

                if (!m_indices.empty())
                {
                    rendering::BufferCreationInfo indexInfo;
                    indexInfo.allowIndex = true;
                    indexInfo.size = m_indices.dataSize();

                    auto sourceData = CreateSourceData(m_indices);
                    m_indexBuffer = owner.createBuffer(indexInfo, sourceData);
                }
            }

            void draw(command::CommandWriter& cmd, const ShaderLibrary* func, uint16_t firstInstance = 0, uint16_t numInstances = 1) const
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

    } // test
} // rendering