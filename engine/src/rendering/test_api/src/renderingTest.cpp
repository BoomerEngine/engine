/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "renderingTest.h"

#include "rendering/driver/include/renderingDriver.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "base/image/include/imageView.h"
#include "base/io/include/ioSystem.h"

namespace rendering
{
    namespace test
    {

        ///---

        RTTI_BEGIN_TYPE_CLASS(RenderingTestOrderMetadata);
        RTTI_END_TYPE();

        RTTI_BEGIN_TYPE_CLASS(RenderingTestSubtestCountMetadata);
        RTTI_END_TYPE();

        ///---

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IRenderingTest);
        RTTI_END_TYPE();

        IRenderingTest::IRenderingTest()
            : m_hasErrors(false)
            , m_device(nullptr)
        {}

        IRenderingTest::~IRenderingTest()
        {}

        void IRenderingTest::shutdown()
        {
            if (m_device)
            {
                for (auto& obj : m_driverObjects)
                    m_device->releaseObject(obj);
            }
        }

        bool IRenderingTest::reportError(base::StringView<char> txt)
        {
            TRACE_ERROR("Rendering test error: {}", txt);
            m_hasErrors = true;
            return false;
        }

        bool IRenderingTest::prepareAndInitialize(IDriver* drv, uint32_t subTestIndex)
        {
            m_subTestIndex = subTestIndex;
            m_device = drv;
            initialize();
            return !m_hasErrors;
        }

        const ShaderLibrary* IRenderingTest::loadShader(base::StringView<char> partialPath)
        {
            if (auto res = base::LoadResource<ShaderLibrary>(base::TempString("/engine/tests/shaders/{}", partialPath)))
            {
                auto lock = base::CreateLock(m_allLoadedResourcesLock);
                m_allLoadedResources.pushBack(res);
                return res.acquire();
            }

            reportError(base::TempString("Failed to load shaders from '{}'", partialPath));
            return nullptr;
        }

        BufferView IRenderingTest::createBuffer(const BufferCreationInfo& info, const SourceData* initializationData)
        {
            auto ret = m_device->createBuffer(info, initializationData);
            if (ret.empty())
            {
                reportError("Failed to create data buffer");
                return BufferView();
            }

            m_driverObjects.pushBack(ret.id());
            return ret;
        }

        ImageView IRenderingTest::createImage(const ImageCreationInfo& info, const SourceData* sourceData /*= nullptr*/, bool uavCapable /*= false*/)
        {
            auto ret = m_device->createImage(info, sourceData);
            if (ret.empty())
            {
                reportError("Failed to create image");
                return ImageView();
            }

            m_driverObjects.pushBack(ret.id());
            return ret;
        }

        ObjectID IRenderingTest::createSampler(const SamplerState& info)
        {
            auto ret = m_device->createSampler(info);
            if (ret.empty())
            {
                reportError("Failed to sampler");
                return ObjectID();
            }

            m_driverObjects.pushBack(ret);
            return ret;
        }

        //--

        using namespace base::image;

        static ImageFormat ConvertImageFormat(PixelFormat pixelFormat, uint32_t numChannels)
        {
            if (pixelFormat == PixelFormat::Uint8_Norm)
            {
                switch (numChannels)
                {
                case 1: return ImageFormat::R8_UNORM;
                case 2: return ImageFormat::RG8_UNORM;
                case 3: return ImageFormat::RGB8_UNORM;
                case 4: return ImageFormat::RGBA8_UNORM;
                }
            }
            else if (pixelFormat == PixelFormat::Float32_Raw)
            {
                switch (numChannels)
                {
                case 1: return ImageFormat::R32F;
                case 2: return ImageFormat::RG32F;
                case 4: return ImageFormat::RGBA32F;
                }
            }

            ASSERT(!"Unsupported test image format");
            return ImageFormat::UNKNOWN;
        }

        static base::image::ImagePtr CreateMissingAlphaChannel(const base::image::ImagePtr& source)
        {
            if (source->channels() != 4)
            {
                auto biggerImage = base::CreateSharedPtr<base::image::Image>(source->format(), 4, source->width(), source->height());
                base::image::ConvertChannels(source->view(), biggerImage->view(), &base::Color::WHITE);
                return biggerImage;
            }
            else
            {
                return source;
            }
        }

        void TextureSlice::generateMipmaps()
        {
            auto curImg = m_mipmaps.back();
            auto width = curImg->width();
            auto height = curImg->height();
            auto depth = curImg->depth();
            while (width > 1 || height > 1 || depth > 1)
            {
                width = std::max<uint16_t>(1, width / 2);
                height = std::max<uint16_t>(1, height / 2);
                depth = std::max<uint16_t>(1, depth / 2);

                auto newImage = base::CreateSharedPtr<base::image::Image>(curImg->format(), curImg->channels(), width, height, depth);
                base::image::Downsample(curImg->view(), newImage->view(), base::image::DownsampleMode::Average, base::image::ColorSpace::Linear);

                m_mipmaps.pushBack(curImg);
                curImg = newImage;
            }
        }

        ImageView IRenderingTest::createImage(const base::Array<TextureSlice>& slices, ImageViewType viewType, bool uavCapable /*= false*/)
        {
            // no slices or no mips
            if (slices.empty() || slices[0].m_mipmaps.empty())
            {
                reportError("Failed to create image from empty list of slices");
                return ImageView();
            }

            // get the image format from the first mip of the first slice
            auto rootImage = slices[0].m_mipmaps[0];
            auto imageFormat = ConvertImageFormat(rootImage->format(), rootImage->channels());
            if (imageFormat == ImageFormat::UNKNOWN)
            {
                reportError("Failed to create image from unknown pixel format");
                return ImageView();
            }

            // create source data blocks
            base::Array<SourceData> sourceData;
            auto numMips = slices[0].m_mipmaps.size();
            //sourceData.reserve(slices.size() * numMips);

            // prepare structures describing the data location
            for (uint32_t i = 0; i < slices.size(); ++i)
            {
                auto& slice = slices[i];
                for (uint32_t j = 0; j < slice.m_mipmaps.size(); ++j)
                {
                    auto& mip = slice.m_mipmaps[j];
                    auto view = mip->view();

                    SourceData data;
                    data.size = view.dataSize();
                    data.offset = 0;
                    data.data = base::Buffer::Create(POOL_TEMP, view.dataSize(), 1, mip->data());
                    //data.m_rowPitch = mip->layout().m_rowPitch;
                    //data.m_slicePitch = mip->layout().m_layerPitch;
                    sourceData.pushBack(data);
                }
            }

            // setup image information
            auto* imagePtr = rootImage.get();
            ImageCreationInfo imageInfo;
            imageInfo.view = viewType;
            imageInfo.numMips = range_cast<uint8_t>(numMips);
            imageInfo.numSlices = range_cast<uint16_t>(slices.size());
            imageInfo.allowShaderReads = true;
            imageInfo.allowUAV = uavCapable;
            imageInfo.width = rootImage->width();
            imageInfo.height = rootImage->height();
            imageInfo.depth = rootImage->depth();
            imageInfo.format = imageFormat;

            // create the image
            return createImage(imageInfo, sourceData.typedData());
        }

        ImageView IRenderingTest::loadImage2D(const base::StringBuf& assetFile, bool createMipmaps /*= false*/, bool uavCapable /*= false*/, bool forceAlpha)
        {
            auto imagePtr = base::LoadResource<base::image::Image>(base::TempString("/engine/tests/textures/{}", assetFile)).acquire();
            if (!imagePtr)
            {
                reportError(base::TempString("Failed to load image '{}'", assetFile));
                return ImageView();
            }

            // update 3 components to 4 components by adding opaque alpha
            if (forceAlpha)
                imagePtr = CreateMissingAlphaChannel(imagePtr);

            // create simple slice
            base::Array<TextureSlice> slices;
            slices.emplaceBack().m_mipmaps.pushBack(imagePtr);

            if (createMipmaps)
                slices.back().generateMipmaps();

            return createImage(slices, ImageViewType::View2D, uavCapable);
        }

        ImageView IRenderingTest::createMipmapTest2D(uint16_t initialSize, bool markers /*= false*/)
        {
            base::Array<TextureSlice> slices;
            slices.emplaceBack();

            // color table
            static base::Color colors[] =
            {
                base::Color(255, 50, 50, 255),
                base::Color(50, 255, 50, 255),
                base::Color(50, 50, 255, 255),
                base::Color(255, 50, 255, 255),
                base::Color(50, 255, 255, 255),
                base::Color(255, 255, 50, 255),
                base::Color(255, 255, 255, 255),
                base::Color(255, 127, 127, 255),
                base::Color(127, 255, 127, 255),
                base::Color(127, 127, 255, 255),
                base::Color(255, 50, 127, 255),
                base::Color(50, 255, 127, 255),
                base::Color(127, 50, 1.5f, 255),
            };

            // create image slices
            uint16_t size = initialSize;
            uint16_t level = 0;
            while (size > 0)
            {
                auto mipImage = base::CreateSharedPtr<base::image::Image>(PixelFormat::Uint8_Norm, 4, size, size);
                base::image::Fill(mipImage->view(), &colors[level % ARRAY_COUNT(colors)]);
                slices.back().m_mipmaps.pushBack(mipImage);

                size = size / 2;
                level += 1;
            }

            return createImage(slices, ImageViewType::View2D);
        }

        ImageView IRenderingTest::createChecker2D(uint16_t initialSize, uint32_t checkerSize, bool generateMipmaps /*= true*/, base::Color colorA /*= base::Color::WHITE*/, base::Color colorB /*= base::Color::BLACK*/)
        {
            auto mipImage = base::CreateSharedPtr<base::image::Image>(PixelFormat::Uint8_Norm, 4, initialSize, initialSize);
            base::image::Fill(mipImage->view(), &colorA);

            // create checker
            uint32_t flag = 0;
            for (uint32_t y = 0; y < initialSize; y += checkerSize)
            {
                for (uint32_t x = 0; x < initialSize; x += checkerSize, flag = !flag)
                {
                    if (flag)
                    {
                        auto checkerView = mipImage->view().subView(x, y, checkerSize, checkerSize);
                        base::image::Fill(checkerView, &colorB);
                    }
                }
                flag = !flag;
            }

            // create slice and autogenerate mipmaps for it
            base::Array<TextureSlice> slices;
            slices.emplaceBack().m_mipmaps.pushBack(mipImage);;
            slices.back().generateMipmaps();

            return createImage(slices, ImageViewType::View2D);
        }

        static base::image::ImagePtr CreateFilledImage(uint16_t size, base::Color color)
        {
            auto mipImage = base::CreateSharedPtr<base::image::Image>(PixelFormat::Uint8_Norm, 4, size, size);
            base::image::Fill(mipImage->view(), &color);
            return mipImage;
        }

        ImageView IRenderingTest::createFlatCubemap(uint16_t size)
        {
            base::Array<TextureSlice> slices;
            slices.emplaceBack().m_mipmaps.pushBack(CreateFilledImage(size, base::Color(255, 128, 128, 255))); // X+
            slices.emplaceBack().m_mipmaps.pushBack(CreateFilledImage(size, base::Color(0, 128, 128, 255))); // X-
            slices.emplaceBack().m_mipmaps.pushBack(CreateFilledImage(size, base::Color(128, 255, 128, 255))); // Y+
            slices.emplaceBack().m_mipmaps.pushBack(CreateFilledImage(size, base::Color(128, 0, 128, 255))); // Y-
            slices.emplaceBack().m_mipmaps.pushBack(CreateFilledImage(size, base::Color(128, 128, 255, 255))); // Z+
            slices.emplaceBack().m_mipmaps.pushBack(CreateFilledImage(size, base::Color(128, 128, 0, 255))); // Z-

            return createImage(slices, ImageViewType::ViewCube);
        }

        static base::image::ImagePtr CreateCubeSide(uint16_t size, const base::Vector3& n, const base::Vector3& u, const base::Vector3& v)
        {
            auto mipImage = base::CreateSharedPtr<base::image::Image>(PixelFormat::Uint8_Norm, 4, size, size);

            // fill image
            /*CreateCubeKernel::Params params;
            params.invSize = 1.0f / (float)size;
            params.n = n;
            params.v = v;
            params.u = u;

            PixelAccess access(*mipImage);
            base::image::ProcessImageInplace<CreateCubeKernel>(access, mipImage->range(), &params);*/

            return mipImage;
        }

        ImageView IRenderingTest::createColorCubemap(uint16_t size)
        {
            base::Array<TextureSlice> slices;
            slices.emplaceBack().m_mipmaps.pushBack(CreateCubeSide(size, base::Vector3::EX(), -base::Vector3::EZ(), -base::Vector3::EY())); // X+
            slices.emplaceBack().m_mipmaps.pushBack(CreateCubeSide(size, -base::Vector3::EX(), base::Vector3::EZ(), -base::Vector3::EY())); // X-

            slices.emplaceBack().m_mipmaps.pushBack(CreateCubeSide(size, base::Vector3::EY(), base::Vector3::EX(), base::Vector3::EZ())); // Y+
            slices.emplaceBack().m_mipmaps.pushBack(CreateCubeSide(size, -base::Vector3::EY(), base::Vector3::EX(), -base::Vector3::EZ())); // Y-

            slices.emplaceBack().m_mipmaps.pushBack(CreateCubeSide(size, base::Vector3::EZ(), base::Vector3::EX(), -base::Vector3::EY())); // Z+
            slices.emplaceBack().m_mipmaps.pushBack(CreateCubeSide(size, -base::Vector3::EZ(), -base::Vector3::EX(), -base::Vector3::EY())); // Z-

            return createImage(slices, ImageViewType::ViewCube);
        }

        bool IRenderingTest::loadCubemapSide(base::Array<TextureSlice>& outSlices, const base::StringBuf& assetFile, bool createMipmaps /*= false*/)
        {
            auto imagePtr = base::LoadResource<base::image::Image>(base::TempString("/engine/tests/textures/{}", assetFile)).acquire();
            if (!imagePtr)
            {
                reportError(base::TempString("Failed to load image '{}'", assetFile));
                return false;
            }

            // update 3 components to 4 components by adding opaque alpha
            imagePtr = CreateMissingAlphaChannel(imagePtr);
            outSlices.emplaceBack().m_mipmaps.pushBack(imagePtr);

            if (createMipmaps)
                outSlices.back().generateMipmaps();

            return true;
        }

        ImageView IRenderingTest::loadCubemap(const base::StringBuf& assetFile, bool createMipmaps /*= false*/)
        {
            base::Array<TextureSlice> slices;
            if (!loadCubemapSide(slices, base::TempString("{}_right.png", assetFile), createMipmaps)) return ImageView();
            if (!loadCubemapSide(slices, base::TempString("{}_left.png", assetFile), createMipmaps)) return ImageView();
            if (!loadCubemapSide(slices, base::TempString("{}_top.png", assetFile), createMipmaps)) return ImageView();
            if (!loadCubemapSide(slices, base::TempString("{}_bottom.png", assetFile), createMipmaps)) return ImageView();
            if (!loadCubemapSide(slices, base::TempString("{}_front.png", assetFile), createMipmaps)) return ImageView();
            if (!loadCubemapSide(slices, base::TempString("{}_back.png", assetFile), createMipmaps)) return ImageView();

            return createImage(slices, ImageViewType::ViewCube);
        }

        //--

          //--

        SimpleMesh::SimpleMesh()
        {}

        SimpleMesh::~SimpleMesh()
        {
        }

        static uint64_t CalcVertexHash(const Mesh3DVertex& v)
        {
            base::CRC64 crc;
            crc.append(&v, sizeof(v));
            return crc.crc();
        }

        MeshSetup::MeshSetup()
            : m_loadTransform(base::Matrix::IDENTITY())
            , m_swapYZ(true)
            , m_flipFaces(false)
            , m_computeFaceNormals(false)
        {}

        void SimpleMesh::drawChunk(command::CommandWriter& cmd, const ShaderLibrary* func, uint32_t chunkIndex) const
        {
            auto& chunk = m_chunks[chunkIndex];

            cmd.opBindIndexBuffer(m_indexBuffer);
            cmd.opBindVertexBuffer("Mesh3DVertex"_id, m_vertexBuffer);
            cmd.opDrawIndexed(func, chunk.m_firstVertex, chunk.m_firstIndex, chunk.m_numIndices);
        }

        void SimpleMesh::drawMesh(command::CommandWriter& cmd, const ShaderLibrary* func) const
        {
            for (uint32_t i = 0; i < m_chunks.size(); ++i)
                drawChunk(cmd, func, i);
        }

        //--

        SimpleMeshPtr IRenderingTest::loadMesh(const base::StringBuf& assetFile, const MeshSetup& setup)
        {
            return nullptr;
            /*// assemble full path
            auto fullMeshPath = base::res::ResourcePath(base::TempString("/engine/tests/meshes/{}", assetFile));
            auto loadedAsset = base::LoadResource<base::res::RawTextData>(fullMeshPath).acquire();
            if (!loadedAsset)
                return nullptr;

            // load asset to buffer
            const auto assetData = loadedAsset->data();

            auto ret = base::CreateSharedPtr<SimpleMesh>();

            base::Array<base::Vector2> uvVertices;
            base::Array<base::Vector3> posVertices;
            base::Array<base::Vector3> nVertices;

            base::HashMap<uint64_t, uint16_t> mappedVertices;
            uint32_t chunkBaseVertex = 0;

            SimpleMesh::Chunk info;
            info.m_material = "default"_id;
            info.m_firstIndex = 0;
            info.m_firstVertex = 0;
            info.m_numIndices = 0;
            info.m_numVerties = 0;
            ret->m_chunks.pushBack(info);
            ret->m_bounds.clear();

            base::StringParser str(assetData);
            while (str.parseWhitespaces())
            {
                if (str.parseKeyword("vn"))
                {
                    base::Vector3 data;
                    str.parseFloat(data.x);
                    str.parseFloat(data.y);
                    str.parseFloat(data.z);
                    if (setup.m_swapYZ)
                        std::swap(data.y, data.z);
                    nVertices.pushBack(data);
                }
                else if (str.parseKeyword("vt"))
                {
                    base::Vector2 data;
                    str.parseFloat(data.x);
                    str.parseFloat(data.y);
                    uvVertices.pushBack(data);
                }
                else if (str.parseKeyword("v"))
                {
                    base::Vector3 data;
                    str.parseFloat(data.x);
                    str.parseFloat(data.y);
                    str.parseFloat(data.z);
                    if (setup.m_swapYZ)
                        std::swap(data.y, data.z);
                    posVertices.pushBack(data);
                }
                else if (str.parseKeyword("g"))
                {
                    base::StringView<char> name;
                    str.parseString(name);

                    auto curNumVertices = ret->m_allVertices.size();
                    auto curNumIndices = ret->m_allIndices.size();
                    if (!ret->m_chunks.empty())
                    {
                        auto& curChunk = ret->m_chunks.back();
                        if (curNumVertices > curChunk.m_firstVertex&& curNumIndices > curChunk.m_firstIndex)
                        {
                            curChunk.m_numIndices = curNumIndices - curChunk.m_firstIndex;
                            curChunk.m_numVerties = curNumVertices - curChunk.m_firstVertex;
                        }
                        else
                        {
                            ret->m_chunks.popBack(); // empty chunk
                        }
                    }

                    SimpleMesh::Chunk info;
                    info.m_material = base::StringID(name);
                    info.m_firstIndex = curNumIndices;
                    info.m_firstVertex = curNumVertices;
                    info.m_numIndices = 0;
                    info.m_numVerties = 0;
                    ret->m_chunks.pushBack(info);

                    chunkBaseVertex = curNumVertices;
                    mappedVertices.clear();
                }
                else if (str.parseKeyword("f"))
                {
                    // gather vertices
                    base::InplaceArray<Mesh3DVertex, 10> vertices;
                    for (uint32_t i = 0; i < 10; ++i)
                    {
                        uint32_t posIndex = 0;
                        if (!str.parseUint32(posIndex))
                            break;

                        auto& v = vertices.emplaceBack();

                        if (posIndex > 0 && posIndex <= posVertices.size())
                            v.VertexPosition = posVertices[posIndex - 1];

                        if (str.parseKeyword("/"))
                        {
                            uint32_t uvIndex = 0;
                            if (str.parseUint32(uvIndex))
                                if (uvIndex > 0 && uvIndex <= uvVertices.size())
                                    v.VertexUV = uvVertices[uvIndex - 1];

                            if (str.parseKeyword("/"))
                            {
                                uint32_t nIndex = 0;
                                if (str.parseUint32(nIndex))
                                    if (nIndex > 0 && nIndex <= nVertices.size())
                                        v.VertexNormal = nVertices[nIndex - 1];
                            }
                        }

                        // transform and initialize
                        v.VertexPosition = setup.m_loadTransform.transformPoint(v.VertexPosition);
                        v.VertexNormal = setup.m_loadTransform.transformVector(v.VertexNormal);//.normalized();
                        ret->m_bounds.merge(v.VertexPosition);
                    }

                    // map vertices
                    base::InplaceArray<uint16_t, 10> mappedIndices;
                    if (vertices.size() >= 3)
                    {
                        // compute normal
                        base::Vector3 normal(0, 0, 1);
                        if (setup.m_computeFaceNormals)
                            base::SafeTriangleNormal(vertices[0].VertexPosition, vertices[1].VertexPosition, vertices[2].VertexPosition, normal);
                        normal = -normal;

                        // map vertices
                        for (auto& v : vertices)
                        {
                            if (setup.m_computeFaceNormals)
                                v.VertexNormal = normal;

                            auto hash = CalcVertexHash(v);
                            uint16_t mappedIndex = 0;
                            if (!mappedVertices.find(hash, mappedIndex))
                            {
                                mappedIndex = range_cast<uint16_t>(ret->m_allVertices.size() - chunkBaseVertex);
                                ret->m_allVertices.pushBack(v);
                                mappedVertices.set(hash, mappedIndex);
                            }

                            mappedIndices.pushBack(mappedIndex);
                        }
                    }

                    if (setup.m_flipFaces)
                    {
                        for (uint32_t i = 2; i < mappedIndices.size(); ++i)
                        {
                            ret->m_allIndices.pushBack(mappedIndices[i]);
                            ret->m_allIndices.pushBack(mappedIndices[i - 1]);
                            ret->m_allIndices.pushBack(mappedIndices[0]);
                        }
                    }
                    else
                    {
                        for (uint32_t i = 2; i < mappedIndices.size(); ++i)
                        {
                            ret->m_allIndices.pushBack(mappedIndices[0]);
                            ret->m_allIndices.pushBack(mappedIndices[i - 1]);
                            ret->m_allIndices.pushBack(mappedIndices[i]);
                        }
                    }
                }
                else
                {
                    str.parseTillTheEndOfTheLine();
                }
            }

            // finish chunk
            auto curNumVertices = ret->m_allVertices.size();
            auto curNumIndices = ret->m_allIndices.size();
            if (!ret->m_chunks.empty())
            {
                auto& curChunk = ret->m_chunks.back();
                if (curNumVertices > curChunk.m_firstVertex&& curNumIndices > curChunk.m_firstIndex)
                {
                    curChunk.m_numIndices = curNumIndices - curChunk.m_firstIndex;
                    curChunk.m_numVerties = curNumVertices - curChunk.m_firstVertex;
                }
                else
                {
                    ret->m_chunks.popBack(); // empty chunk
                }
            }

            // create vertex buffer
            {
                BufferCreationInfo info;
                info.allowVertex = true;
                info.size = ret->m_allVertices.dataSize();

                auto sourceData = CreateSourceData(ret->m_allVertices);
                ret->m_vertexBuffer = createBuffer(info, &sourceData);
                if (!ret->m_vertexBuffer)
                    return nullptr;
            }

            // create index buffer
            {
                BufferCreationInfo info;
                info.allowIndex = true;
                info.size = ret->m_allIndices.dataSize();

                auto sourceData = CreateSourceData(ret->m_allIndices);
                ret->m_indexBuffer = createBuffer(info, &sourceData);
                if (!ret->m_indexBuffer)
                    return nullptr;
            }

            return ret;*/
        }

        //--

    } // test
} // rendering