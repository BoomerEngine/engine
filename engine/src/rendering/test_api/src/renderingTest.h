/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingShaderLibrary.h"
#include "rendering/driver/include/renderingFramebuffer.h"

#include "base/object/include/rttiMetadata.h"

#include "renderingTestShared.h"


namespace rendering
{
    namespace test
    {
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

        /// a test mesh - group of triangles
        class SimpleMesh : public base::IReferencable
        {
        public:
            struct Chunk
            {
                base::StringID m_material;
                uint32_t m_firstVertex;
                uint32_t m_firstIndex;
                uint32_t m_numVerties;
                uint32_t m_numIndices;
            };

            base::Array<Mesh3DVertex> m_allVertices;
            base::Array<uint16_t> m_allIndices;
            base::Array<Chunk> m_chunks;

            BufferView m_vertexBuffer;
            BufferView m_indexBuffer;

            base::Box m_bounds;
            //

            void drawChunk(command::CommandWriter& cmd, const ShaderLibrary* func, uint32_t chunkIndex) const;
            void drawMesh(command::CommandWriter& cmd, const ShaderLibrary* func) const;

            SimpleMesh();
            ~SimpleMesh();
        };

        typedef base::RefPtr<SimpleMesh> SimpleMeshPtr;

        // mesh import setup
        struct MeshSetup
        {
            base::Matrix m_loadTransform;
            bool m_swapYZ;
            bool m_flipFaces;
            bool m_computeFaceNormals;

            MeshSetup();
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

            // device
            INLINE IDriver* device() const { return m_device; }

            // sub test
            INLINE uint32_t subTestIndex() const { return m_subTestIndex; }

            //--

            // prepare test
            bool prepareAndInitialize(IDriver* drv, uint32_t subTestIndex);

            // initialize test
            virtual void initialize() = 0;

            // tear down test
            virtual void shutdown();

            // render test via the provided interface, in the interactive mode the frame index is animated
            virtual void render(command::CommandWriter& cmd, float time, const ImageView& backBufferView, const ImageView& backBufferDepthView ) = 0;

            //--

            // load shaders, NOTE: uses short path based in the engine/test/shaders/ directory
            const ShaderLibrary* loadShader(base::StringView partialPath);

            // load a simple mesh (obj file) from disk, file should be in the engine/assets/tests/ directory
            ImageView loadImage2D(const base::StringBuf& assetFile, bool createMipmaps = false, bool uavCapable = false, bool forceAlpha = false);
            
            // load a custom cubemap from 6 images
            ImageView loadCubemap(const base::StringBuf& assetFile, bool createMipmaps = false);

            // load a simple mesh (obj file) from disk, file should be in the engine/assets/tests/ directory
            SimpleMeshPtr loadMesh(const base::StringBuf& assetFile, const MeshSetup& setup = MeshSetup());

            //--

            // create buffer
            BufferView createBuffer(const BufferCreationInfo& info, const SourceData* initializationData = nullptr);

            // create image
            ImageView createImage(const ImageCreationInfo& info, const SourceData* sourceData = nullptr, bool uavCapable = false);

            // create texture from list of slices
            ImageView createImage(const base::Array<TextureSlice>& slices, ImageViewType viewType, bool uavCapable = false);

            // create a 2D mipmap test, each mipmap has different color
            ImageView createMipmapTest2D(uint16_t initialSize, bool markers = false);

            // create a 2D checker image
            ImageView createChecker2D(uint16_t initialSize, uint32_t checkerSize, bool generateMipmaps = true, base::Color colorA = base::Color::WHITE, base::Color colorB = base::Color::BLACK);

            // create a simple flat-color test cubemap
            ImageView createFlatCubemap(uint16_t size = 64);

            // create a simple normal color test cubemap
            ImageView createColorCubemap(uint16_t size = 64);

            // create sampler
            ObjectID createSampler(const SamplerState& info);

            //--

            // report loading error
            bool reportError(base::StringView txt);

        private:
            base::SpinLock m_allLoadedResourcesLock;
            base::Array<base::res::BaseReference> m_allLoadedResources;
            base::Array<ObjectID> m_driverObjects;
            bool m_hasErrors;
            uint32_t m_subTestIndex;
            IDriver* m_device;

            bool loadCubemapSide(base::Array<TextureSlice>& outSlices, const base::StringBuf& assetFile, bool createMipmaps /*= false*/);
        };

        ///---

        template< typename VT = Simple3DVertex, typename IT = uint16_t >
        struct VertexIndexBunch
        {
            base::Array<VT> m_vertices;
            base::Array<IT> m_indices;

            BufferView m_vertexBuffer;
            BufferView m_indexBuffer;

            void createBuffers(IRenderingTest& owner)
            {
                if (!m_vertices.empty())
                {
                    rendering::BufferCreationInfo vertexInfo;
                    vertexInfo.allowVertex = true;
                    vertexInfo.size = m_vertices.dataSize();

                    auto sourceData = CreateSourceData(m_vertices);
                    m_vertexBuffer = owner.createBuffer(vertexInfo, &sourceData);
                }

                if (!m_indices.empty())
                {
                    rendering::BufferCreationInfo indexInfo;
                    indexInfo.allowIndex = true;
                    indexInfo.size = m_indices.dataSize();

                    auto sourceData = CreateSourceData(m_indices);
                    m_indexBuffer = owner.createBuffer(indexInfo, &sourceData);
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