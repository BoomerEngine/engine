/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "renderingTest.h"

#include "core/image/include/image.h"
#include "core/image/include/imageUtils.h"
#include "core/image/include/imageView.h"
#include "core/io/include/ioSystem.h"

#include "core/resource/include/resourceTags.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/buffer.h"
#include "gpu/device/include/image.h"
#include "gpu/device/include/shader.h"
#include "gpu/device/include/shaderData.h"
#include "gpu/device/include/descriptor.h"
#include "gpu/device/include/shader.h"
#include "gpu/device/include/pipeline.h"
#include "gpu/device/include/output.h"
#include "gpu/device/include/globalObjects.h"
#include "core/resource/include/depotService.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

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
}

void IRenderingTest::queryInitialCamera(Vector3& outPosition, Angles& outRotation)
{
	outPosition = Vector3(-1.5f, 0.0f, 1.0f);
	outRotation = Angles(15.0f, 0.0f, 0.0f);
}

void IRenderingTest::updateCamera(const Vector3& position, const Angles& rotation)
{
	m_cameraPosition = position;
	m_cameraAngles = rotation;
}

void IRenderingTest::describeSubtest(IFormatStream& f)
{
	f.appendf("SubTest {}", subTestIndex());
}

bool IRenderingTest::reportError(StringView txt)
{
    TRACE_ERROR("Rendering test error: {}", txt);
    m_hasErrors = true;
    return false;
}

bool IRenderingTest::prepareAndInitialize(IDevice* drv, uint32_t subTestIndex, IOutputObject* output)
{
	m_subTestIndex = subTestIndex;
	m_device = drv;

	m_quadVertices = createVertexBuffer(sizeof(Simple3DVertex) * 6, nullptr);

	initialize();

    return !m_hasErrors;
}

GraphicsPipelineObjectPtr IRenderingTest::loadGraphicsShader(StringView partialPath, const GraphicsRenderStatesSetup* states /*= nullptr*/, const ShaderSelector& extraSelectors /*= ShaderSelector()*/)
{
    auto selectors = extraSelectors;
    //selectors.set("YFLIP"_id, 1);

	// load shader
	auto res = LoadStaticShader(TempString("tests/{}", partialPath), selectors);
	if (!res)
	{
		reportError(TempString("Failed to load shaders from '{}'", partialPath));
		return nullptr;
	}

	// assemble final selectors
	if (!res->deviceShader())
	{
		reportError(TempString("Failed to find permutation '{}' in shader '{}'", selectors, partialPath));
		return nullptr;
	}

	// map render states
	GraphicsRenderStatesObjectPtr renderStates;
	if (states)
		renderStates = createRenderStates(*states);

	// create the PSO
    return res->deviceShader()->createGraphicsPipeline(renderStates);
}

ComputePipelineObjectPtr IRenderingTest::loadComputeShader(StringView partialPath, const ShaderSelector& extraSelectors /*= ShaderSelector()*/)
{
	// load shader
    auto res = LoadStaticShader(TempString("tests/{}", partialPath), extraSelectors);
	if (!res)
	{
		reportError(TempString("Failed to load shaders from '{}'", partialPath));
		return nullptr;
	}

	// assemble final selectors
	if (!res->deviceShader())
	{
		reportError(TempString("Failed to find permutation '{}' in shader '{}'", extraSelectors, partialPath));
		return nullptr;
	}


	// create the compute PSO
	return res->deviceShader()->createComputePipeline();
}

GraphicsRenderStatesObjectPtr IRenderingTest::createRenderStates(const GraphicsRenderStatesSetup& setup)
{
	const auto key = setup.key();

	GraphicsRenderStatesObjectPtr ret;
	if (m_renderStatesMap.find(key, ret))
	{
		DEBUG_CHECK(ret->states() == setup);
		return ret;
	}

	if (auto ret = m_device->createGraphicsRenderStates(setup))
	{
		m_renderStatesMap[key] = ret;
		return ret;
	}

	reportError("Failed to create render states");
	return nullptr;
}

BufferObjectPtr IRenderingTest::createBuffer(const BufferCreationInfo& info, const ISourceDataProvider* initializationData)
{
    if (auto ret = m_device->createBuffer(info, initializationData))
		return ret;

    reportError("Failed to create data buffer");
	return nullptr;
}

BufferObjectPtr IRenderingTest::createIndirectBuffer(uint32_t size, uint32_t stride)
{
	BufferCreationInfo info;
	info.allowUAV = true;
	info.allowIndirect = true;
	info.size = size;
	info.stride = stride;
	info.label = "IndirectBuffer";

	return createBuffer(info);
}

BufferObjectPtr IRenderingTest::createFormatBuffer(ImageFormat format, uint32_t size, bool allowUAV)
{
	BufferCreationInfo info;
	info.allowCostantReads = false;
	info.allowShaderReads = true;
	info.allowUAV = allowUAV;
	info.size = size;
	info.format = format;
	info.label = "FormatBuffer";

	return createBuffer(info);
}

BufferObjectPtr IRenderingTest::createConstantBuffer(uint32_t size, const void* sourceData /*= nullptr*/, bool dynamic /*= false*/, bool allowUav /*= false*/)
{
	BufferCreationInfo info;
	info.allowCostantReads = true;
	info.allowUAV = allowUav;
	info.allowDynamicUpdate = dynamic;
	info.size = size;
	info.label = "ConstantBuffer";

	if (sourceData)
	{
		auto buf = Buffer::Create(POOL_TEMP, size, 16, sourceData);
		auto data = RefNew<SourceDataProviderBuffer>(buf);
		return createBuffer(info, data);
	}
	else
	{
		return createBuffer(info);
	}
}

BufferObjectPtr IRenderingTest::createVertexBuffer(uint32_t size, const void* sourceData)
{
    BufferCreationInfo info;
    info.allowVertex = true;
    info.allowDynamicUpdate = sourceData == nullptr;
    info.size = size;
    info.label = "VertexBuffer";

	if (sourceData)
	{
		auto buf = Buffer::Create(POOL_TEMP, size, 16, sourceData);
		auto data = RefNew<SourceDataProviderBuffer>(buf);
		return createBuffer(info, data);
	}
	else
	{ 
		return createBuffer(info);
	}
}

BufferObjectPtr IRenderingTest::createIndexBuffer(uint32_t size, const void* sourceData)
{
    BufferCreationInfo info;
    info.allowIndex = true;
    info.allowDynamicUpdate = sourceData == nullptr;
    info.size = size;
    info.label = "VertexBuffer";

	if (sourceData)
	{
		auto buf = Buffer::Create(POOL_TEMP, size, 16, sourceData);
		auto data = RefNew<SourceDataProviderBuffer>(buf);
		return createBuffer(info, data);
	}
	else
	{
		return createBuffer(info);
	}
}

BufferObjectPtr IRenderingTest::createStorageBuffer(uint32_t size, uint32_t stride /*= 0*/, bool dynamic /*= false*/, bool allowVertex /*= false*/, bool allowIndex /*= false*/)
{
    BufferCreationInfo info;
    info.allowShaderReads = true;
    info.allowDynamicUpdate = dynamic;
    info.allowVertex = allowVertex;
    info.allowIndex = allowIndex;
    info.allowUAV = true;
    info.size = size;
    info.stride = stride;
    info.label = "StorageBuffer";
    return createBuffer(info);
}

ImageObjectPtr IRenderingTest::createImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData /*= nullptr*/)
{
	if (auto ret = m_device->createImage(info, sourceData))
		return ret;

    reportError("Failed to create image");
    return AddRef(Globals().TextureWhite->image());
}

SamplerObjectPtr IRenderingTest::createSampler(const SamplerState& info)
{
	if (auto ret = m_device->createSampler(info))
		return ret;

    reportError("Failed to sampler");
	return Globals().SamplerClampPoint;
}

//--

using namespace image;

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

static image::ImagePtr CreateMissingAlphaChannel(const image::ImagePtr& source)
{
    if (source->channels() != 4)
    {
        auto biggerImage = RefNew<image::Image>(source->format(), 4, source->width(), source->height());
        image::ConvertChannels(source->view(), biggerImage->view(), &Color::WHITE);
        return biggerImage;
    }
    else
    {
        return source;
    }
}

void TextureSlice::generateMipmaps()
{
    auto curImg = mipmaps.back();
    auto width = curImg->width();
    auto height = curImg->height();
    auto depth = curImg->depth();
    while (width > 1 || height > 1 || depth > 1)
    {
        width = std::max<uint16_t>(1, width / 2);
        height = std::max<uint16_t>(1, height / 2);
        depth = std::max<uint16_t>(1, depth / 2);

        auto newImage = RefNew<image::Image>(curImg->format(), curImg->channels(), width, height, depth);
        image::Downsample(curImg->view(), newImage->view(), image::DownsampleMode::Average, image::ColorSpace::Linear);

        mipmaps.pushBack(newImage);
        curImg = newImage;
    }
}

class RenderingTestImageSlicesDataProvider : public ISourceDataProvider
{
public:
	RenderingTestImageSlicesDataProvider(const Array<TextureSlice>& data, uint32_t numMips)
		: m_data(data)
		, m_numMips(numMips)
	{}

    virtual CAN_YIELD void fetchSourceData(Array<SourceAtom>& outAtoms) const override final
	{
        const auto numTotalAtoms = m_numMips * m_data.size();

        for (uint32_t index=0; index< numTotalAtoms; ++index)
        {
            auto& atom = outAtoms.emplaceBack();
            atom.mip = index % m_numMips;
            atom.slice = index / m_numMips;

            const auto& sliceData = m_data[atom.slice];
            const auto* mipImage = sliceData.mipmaps[atom.mip].get();

            atom.buffer = mipImage->view().toBuffer();
            atom.sourceDataSize = atom.buffer.size();
            atom.sourceData = atom.buffer.data();
        }
	}

private:
	Array<TextureSlice> m_data;
	uint32_t m_numMips;
};

ImageObjectPtr IRenderingTest::createImage(const Array<TextureSlice>& slices, ImageViewType viewType)
{
    // no slices or no mips
    if (slices.empty() || slices[0].mipmaps.empty())
    {
        reportError("Failed to create image from empty list of slices");
        return AddRef(Globals().TextureWhite->image());
    }

    // get the image format from the first mip of the first slice
    auto rootImage = slices[0].mipmaps[0];
    auto imageFormat = ConvertImageFormat(rootImage->format(), rootImage->channels());
    if (imageFormat == ImageFormat::UNKNOWN)
    {
        reportError("Failed to create image from unknown pixel format");
        return AddRef(Globals().TextureWhite->image());
    }

    // create source data blocks
	const auto numMips = slices[0].mipmaps.size();
	const auto sourceData = RefNew< RenderingTestImageSlicesDataProvider>(slices, numMips);

    // setup image information
    auto* imagePtr = rootImage.get();
    ImageCreationInfo imageInfo;
    imageInfo.view = viewType;
    imageInfo.numMips = range_cast<uint8_t>(numMips);
    imageInfo.numSlices = range_cast<uint16_t>(slices.size());
    imageInfo.allowShaderReads = true;
	imageInfo.allowCopies = true;
    imageInfo.width = rootImage->width();
    imageInfo.height = rootImage->height();
    imageInfo.depth = rootImage->depth();
    imageInfo.format = imageFormat;

    // create the image
	return createImage(imageInfo, sourceData);
}

ImageObjectPtr IRenderingTest::loadImage2D(StringView assetFile, bool createMipmaps /*= false*/, bool forceAlpha)
{
    auto imagePtr = LoadImageFromDepotPath(TempString("/engine/tests/textures/{}", assetFile));
    if (!imagePtr)
    {
        reportError(TempString("Failed to load image '{}'", assetFile));
        return AddRef(Globals().TextureWhite->image());
    }

    // update 3 components to 4 components by adding opaque alpha
    if (forceAlpha)
        imagePtr = CreateMissingAlphaChannel(imagePtr);

    // create simple slice
    Array<TextureSlice> slices;
    slices.emplaceBack().mipmaps.pushBack(imagePtr);

    if (createMipmaps)
        slices.back().generateMipmaps();

	return createImage(slices, ImageViewType::View2D);
}

ImageObjectPtr IRenderingTest::createMipmapTest2D(uint16_t initialSize, bool markers /*= false*/)
{
    Array<TextureSlice> slices;
    slices.emplaceBack();

    // color table
    static Color colors[] =
    {
        Color(255, 50, 50, 255),
        Color(50, 255, 50, 255),
        Color(50, 50, 255, 255),
        Color(255, 50, 255, 255),
        Color(50, 255, 255, 255),
        Color(255, 255, 50, 255),
        Color(255, 255, 255, 255),
        Color(255, 127, 127, 255),
        Color(127, 255, 127, 255),
        Color(127, 127, 255, 255),
        Color(255, 50, 127, 255),
        Color(50, 255, 127, 255),
        Color(127, 50, 1.5f, 255),
    };

    // create image slices
    uint16_t size = initialSize;
    uint16_t level = 0;
    while (size > 0)
    {
        auto mipImage = RefNew<image::Image>(PixelFormat::Uint8_Norm, 4, size, size);
        image::Fill(mipImage->view(), &colors[level % ARRAY_COUNT(colors)]);
        slices.back().mipmaps.pushBack(mipImage);

        size = size / 2;
        level += 1;
    }

    return createImage(slices, ImageViewType::View2D);
}

ImageObjectPtr IRenderingTest::createChecker2D(uint16_t initialSize, uint32_t checkerSize, bool generateMipmaps /*= true*/, Color colorA /*= Color::WHITE*/, Color colorB /*= Color::BLACK*/)
{
    auto mipImage = RefNew<image::Image>(PixelFormat::Uint8_Norm, 4, initialSize, initialSize);
    image::Fill(mipImage->view(), &colorA);

    // create checker
    uint32_t flag = 0;
    for (uint32_t y = 0; y < initialSize; y += checkerSize)
    {
        for (uint32_t x = 0; x < initialSize; x += checkerSize, flag = !flag)
        {
            if (flag)
            {
                auto checkerView = mipImage->view().subView(x, y, checkerSize, checkerSize);
                image::Fill(checkerView, &colorB);
            }
        }
        flag = !flag;
    }

    // create slice and autogenerate mipmaps for it
    Array<TextureSlice> slices;
    slices.emplaceBack().mipmaps.pushBack(mipImage);;
    slices.back().generateMipmaps();

    return createImage(slices, ImageViewType::View2D);
}

static image::ImagePtr CreateFilledImage(uint16_t size, Color color)
{
    auto mipImage = RefNew<image::Image>(PixelFormat::Uint8_Norm, 4, size, size);
    image::Fill(mipImage->view(), &color);
    return mipImage;
}

ImageObjectPtr IRenderingTest::createFlatCubemap(uint16_t size)
{
    Array<TextureSlice> slices;
    slices.emplaceBack().mipmaps.pushBack(CreateFilledImage(size, Color(255, 128, 128, 255))); // X+
    slices.emplaceBack().mipmaps.pushBack(CreateFilledImage(size, Color(0, 128, 128, 255))); // X-
    slices.emplaceBack().mipmaps.pushBack(CreateFilledImage(size, Color(128, 255, 128, 255))); // Y+
    slices.emplaceBack().mipmaps.pushBack(CreateFilledImage(size, Color(128, 0, 128, 255))); // Y-
    slices.emplaceBack().mipmaps.pushBack(CreateFilledImage(size, Color(128, 128, 255, 255))); // Z+
    slices.emplaceBack().mipmaps.pushBack(CreateFilledImage(size, Color(128, 128, 0, 255))); // Z-

    return createImage(slices, ImageViewType::ViewCube);
}

static image::ImagePtr CreateCubeSide(uint16_t size, const Vector3& n, const Vector3& u, const Vector3& v)
{
    auto mipImage = RefNew<image::Image>(PixelFormat::Uint8_Norm, 4, size, size);

    // fill image
    /*CreateCubeKernel::Params params;
    params.invSize = 1.0f / (float)size;
    params.n = n;
    params.v = v;
    params.u = u;

    PixelAccess access(*mipImage);
    image::ProcessImageInplace<CreateCubeKernel>(access, mipImage->range(), &params);*/

    return mipImage;
}

ImageObjectPtr IRenderingTest::createColorCubemap(uint16_t size)
{
    Array<TextureSlice> slices;
    slices.emplaceBack().mipmaps.pushBack(CreateCubeSide(size, Vector3::EX(), -Vector3::EZ(), -Vector3::EY())); // X+
    slices.emplaceBack().mipmaps.pushBack(CreateCubeSide(size, -Vector3::EX(), Vector3::EZ(), -Vector3::EY())); // X-

    slices.emplaceBack().mipmaps.pushBack(CreateCubeSide(size, Vector3::EY(), Vector3::EX(), Vector3::EZ())); // Y+
    slices.emplaceBack().mipmaps.pushBack(CreateCubeSide(size, -Vector3::EY(), Vector3::EX(), -Vector3::EZ())); // Y-

    slices.emplaceBack().mipmaps.pushBack(CreateCubeSide(size, Vector3::EZ(), Vector3::EX(), -Vector3::EY())); // Z+
    slices.emplaceBack().mipmaps.pushBack(CreateCubeSide(size, -Vector3::EZ(), -Vector3::EX(), -Vector3::EY())); // Z-

    return createImage(slices, ImageViewType::ViewCube);
}

bool IRenderingTest::loadCubemapSide(Array<TextureSlice>& outSlices, StringView assetFile, bool createMipmaps /*= false*/)
{
    auto imagePtr = LoadImageFromDepotPath(TempString("/engine/tests/textures/{}", assetFile));
    if (!imagePtr)
    {
        reportError(TempString("Failed to load image '{}'", assetFile));
        return false;
    }

    // update 3 components to 4 components by adding opaque alpha
    imagePtr = CreateMissingAlphaChannel(imagePtr);
    outSlices.emplaceBack().mipmaps.pushBack(imagePtr);

    if (createMipmaps)
        outSlices.back().generateMipmaps();

    return true;
}

ImageObjectPtr IRenderingTest::loadCubemap(StringView assetFile, bool createMipmaps /*= false*/)
{
    Array<TextureSlice> slices;
    if (!loadCubemapSide(slices, TempString("{}_right.png", assetFile), createMipmaps)) return ImageObjectPtr();
    if (!loadCubemapSide(slices, TempString("{}_left.png", assetFile), createMipmaps)) return ImageObjectPtr();
    if (!loadCubemapSide(slices, TempString("{}_top.png", assetFile), createMipmaps)) return ImageObjectPtr();
    if (!loadCubemapSide(slices, TempString("{}_bottom.png", assetFile), createMipmaps)) return ImageObjectPtr();
    if (!loadCubemapSide(slices, TempString("{}_front.png", assetFile), createMipmaps)) return ImageObjectPtr();
    if (!loadCubemapSide(slices, TempString("{}_back.png", assetFile), createMipmaps)) return ImageObjectPtr();

    return createImage(slices, ImageViewType::ViewCube);
}

//--

void IRenderingTest::drawQuad(CommandWriter& cmd, const GraphicsPipelineObject* func, float x, float y, float w, float h, float u0, float v0, float u1, float v1, Color color)
{
	cmd.opTransitionLayout(m_quadVertices, ResourceLayout::VertexBuffer, ResourceLayout::CopyDest);
    auto* v = cmd.opUpdateDynamicBufferPtrN<Simple3DVertex>(m_quadVertices, 0, 6);
	cmd.opTransitionLayout(m_quadVertices, ResourceLayout::CopyDest, ResourceLayout::VertexBuffer);

    v[0].set(x, y, 0.5f, u0, v0, color);
    v[1].set(x + w, y, 0.5f, u1, v0), color;
    v[2].set(x + w, y + h, 0.5f, u1, v1, color);

    v[3].set(x, y, 0.5f, u0, v0, color);
    v[4].set(x + w, y + h, 0.5f, u1, v1, color);
    v[5].set(x, y + h, 0.5f, u0, v1, color);

    //cmd.opSetPrimitiveType(PrimitiveTopology::TriangleList);
    cmd.opBindVertexBuffer("Simple3DVertex"_id, m_quadVertices);
    cmd.opDraw(func, 0, 6);
}

void IRenderingTest::setQuadParams(CommandWriter& cmd, float x, float y, float w, float h)
{
    Vector4 rectCoords;
    rectCoords.x = -1.0f + (x) * 2.0f;
    rectCoords.y = -1.0f + (y) * 2.0f;
    rectCoords.z = -1.0f + (x + w) * 2.0f;
    rectCoords.w = -1.0f + (y + h) * 2.0f;

	DescriptorEntry desc[1];
	desc[0].constants(rectCoords);

	cmd.opBindDescriptor("QuadParams"_id, desc);
}

void IRenderingTest::setQuadParams(CommandWriter& cmd, const RenderTargetView* rt, const Rect& rect)
{
    const auto invWidth = rt ? 1.0f / rt->width() : 1.0f;
    const auto invHeight = rt ? 1.0f / rt->height() : 1.0f;

    Vector4 rectCoords;
    rectCoords.x = -1.0f + (rect.min.x * invWidth) * 2.0f;
    rectCoords.y = -1.0f + (rect.min.y * invHeight) * 2.0f;
    rectCoords.z = -1.0f + (rect.max.x * invWidth) * 2.0f;
    rectCoords.w = -1.0f + (rect.max.y * invHeight) * 2.0f;
            
	DescriptorEntry desc[1];
	desc[0].constants(rectCoords);

	cmd.opBindDescriptor("QuadParams"_id, desc);
}

//--

SimpleMesh::SimpleMesh()
{}

static uint64_t CalcVertexHash(const Mesh3DVertex& v)
{
    CRC64 crc;
    crc.append(&v, sizeof(v));
    return crc.crc();
}

//--

RefPtr<SimpleMesh> LoadSimpleMeshFromDepotPath(StringView path)
{
    Buffer text;
    if (!GetService<DepotService>()->loadFileToBuffer(path, text))
        return nullptr;

    auto ret = RefNew<SimpleMesh>();

    Array<Vector2> uvVertices;
    Array<Vector3> posVertices;
    Array<Vector3> nVertices;

    HashMap<uint64_t, uint16_t> mappedVertices;
    uint32_t chunkBaseVertex = 0;

    SimpleChunk info;
    info.material = "default"_id;
    info.firstIndex = 0;
    info.firstVertex = 0;
    info.numIndices = 0;
    info.numVerties = 0;
    ret->m_chunks.pushBack(info);

    StringParser str(text);
    while (str.parseWhitespaces())
    {
        if (str.parseKeyword("vn"))
        {
            Vector3 data;
            str.parseFloat(data.x);
            str.parseFloat(data.y);
            str.parseFloat(data.z);
            /*if (setup.m_swapYZ)
                std::swap(data.y, data.z);*/
            nVertices.pushBack(data);
        }
        else if (str.parseKeyword("vt"))
        {
            Vector2 data;
            str.parseFloat(data.x);
            str.parseFloat(data.y);
            uvVertices.pushBack(data);
        }
        else if (str.parseKeyword("v"))
        {
            Vector3 data;
            str.parseFloat(data.x);
            str.parseFloat(data.y);
            str.parseFloat(data.z);
            /*if (setup.m_swapYZ)
                std::swap(data.y, data.z);*/
            posVertices.pushBack(data);
        }
        else if (str.parseKeyword("g"))
        {
            StringView name;
            str.parseString(name);

            auto curNumVertices = ret->m_allVertices.size();
            auto curNumIndices = ret->m_allIndices.size();
            if (!ret->m_chunks.empty())
            {
                auto& curChunk = ret->m_chunks.back();
                if (curNumVertices > curChunk.firstVertex && curNumIndices > curChunk.firstIndex)
                {
                    curChunk.numIndices = curNumIndices - curChunk.firstIndex;
                    curChunk.numVerties = curNumVertices - curChunk.firstVertex;
                }
                else
                {
                    ret->m_chunks.popBack(); // empty chunk
                }
            }

            SimpleChunk info;
            info.material = StringID(name);
            info.firstIndex = curNumIndices;
            info.firstVertex = curNumVertices;
            info.numIndices = 0;
            info.numVerties = 0;
            ret->m_chunks.pushBack(info);

            chunkBaseVertex = curNumVertices;
            mappedVertices.clear();
        }
        else if (str.parseKeyword("f"))
        {
            // gather vertices
            bool hasNormals = false;
            InplaceArray<Mesh3DVertex, 10> vertices;
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
                            {
                                v.VertexNormal = nVertices[nIndex - 1];
                                hasNormals |= !v.VertexNormal.isNearZero();
                            }
                    }
                }
            }

            // map vertices
            InplaceArray<uint16_t, 10> mappedIndices;
            if (vertices.size() >= 3)
            {
                // compute normal
                Vector3 normal;
                if (!hasNormals)
                    SafeTriangleNormal(vertices[0].VertexPosition, vertices[1].VertexPosition, vertices[2].VertexPosition, normal);

                // map vertices
                for (auto& v : vertices)
                {
                    if (!hasNormals)
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

            for (uint32_t i = 2; i < mappedIndices.size(); ++i)
            {
                ret->m_allIndices.pushBack(mappedIndices[0]);
                ret->m_allIndices.pushBack(mappedIndices[i - 1]);
                ret->m_allIndices.pushBack(mappedIndices[i]);
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
        if (curNumVertices > curChunk.firstVertex && curNumIndices > curChunk.firstIndex)
        {
            curChunk.numIndices = curNumIndices - curChunk.firstIndex;
            curChunk.numVerties = curNumVertices - curChunk.firstVertex;
        }
        else
        {
            ret->m_chunks.popBack(); // empty chunk
        }
    }

    return ret;
}

//--

void SimpleRenderMesh::drawChunk(CommandWriter& cmd, const GraphicsPipelineObject* func, uint32_t chunkIndex) const
{
    auto& chunk = m_chunks[chunkIndex];

    cmd.opBindIndexBuffer(m_indexBuffer);
    cmd.opBindVertexBuffer("Mesh3DVertex"_id, m_vertexBuffer);
    cmd.opDrawIndexed(func, chunk.firstVertex, chunk.firstIndex, chunk.numIndices);
}

void SimpleRenderMesh::drawMesh(CommandWriter& cmd, const GraphicsPipelineObject* func) const
{
    for (uint32_t i = 0; i < m_chunks.size(); ++i)
        drawChunk(cmd, func, i);
}

SimpleRenderMeshPtr IRenderingTest::loadMesh(StringView assetFile, const MeshSetup& setup)
{
    auto loadedAsset = LoadSimpleMeshFromDepotPath(TempString("/engine/tests/meshes/{}", assetFile));
    if (!loadedAsset)
    {
        reportError(TempString("Failed to load mesh '{}'", assetFile));
        return nullptr;
    }

    auto vertices = loadedAsset->m_allVertices;
    if (setup.m_swapYZ)
    {
        for (auto& v : vertices)
        {
            std::swap(v.VertexPosition.y, v.VertexPosition.z);
            std::swap(v.VertexNormal.y, v.VertexNormal.z);
        }
    }

    if (setup.m_flipNormal ^ setup.m_flipFaces)
        for (auto& v : vertices)
            v.VertexNormal = -v.VertexNormal;

    if (setup.m_loadTransform != Matrix::IDENTITY())
    {
        for (auto& v : vertices)
        {
            v.VertexPosition = setup.m_loadTransform.transformPoint(v.VertexPosition);
            v.VertexNormal = setup.m_loadTransform.transformVector(v.VertexNormal).normalized();
        }
    }

    Box bounds;
    for (const auto& v : vertices)
        bounds.merge(v.VertexPosition);

    auto indices = loadedAsset->m_allIndices;
    if (setup.m_flipFaces)
    {
        auto* ptr = indices.typedData();
        auto* endPtr = ptr + indices.size();

        while (ptr < endPtr)
        {
            std::swap(ptr[0], ptr[2]);
            ptr += 3;
        }
    }

    auto ret = RefNew<SimpleRenderMesh>();
    ret->m_bounds = bounds;
    ret->m_chunks = loadedAsset->m_chunks;

    ret->m_vertexBuffer = createVertexBuffer(vertices);
    if (!ret->m_vertexBuffer)
        return nullptr;

    ret->m_indexBuffer = createIndexBuffer(indices);
    if (!ret->m_indexBuffer)
        return nullptr;

    return ret;
}

//--

END_BOOMER_NAMESPACE_EX(gpu::test)
