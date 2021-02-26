/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "renderingStaticTexture.h"
#include "core/resource/include/resourceTags.h"
#include "gpu/device/include/renderingDeviceApi.h"
#include "gpu/device/include/renderingDeviceService.h"
#include "gpu/device/include/renderingDeviceGlobalObjects.h"
#include "gpu/device/include/renderingResources.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_STRUCT(StaticTextureMip)
    RTTI_PROPERTY(dataOffset);
    RTTI_PROPERTY(dataSize);
    RTTI_PROPERTY(width);
    RTTI_PROPERTY(height);
    RTTI_PROPERTY(depth);
    RTTI_PROPERTY(rowPitch);
    RTTI_PROPERTY(slicePitch);
    RTTI_PROPERTY(compressed);
    RTTI_PROPERTY(streamed);
    RTTI_END_TYPE();

//---

RTTI_BEGIN_TYPE_CLASS(StaticTexture);
    RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4stex");
    RTTI_METADATA(res::ResourceDescriptionMetadata).description("Static Texture");
    RTTI_METADATA(res::ResourceTagColorMetadata).color(0xa8, 0xd6, 0xe2);
    RTTI_PROPERTY(m_persistentPayload);
    RTTI_PROPERTY(m_streamingPayload);
    RTTI_PROPERTY(m_mips);
RTTI_END_TYPE();

StaticTexture::StaticTexture()
    : ITexture(TextureInfo())
{
}

StaticTexture::StaticTexture(Buffer&& data, res::AsyncBuffer&& asyncData, Array<StaticTextureMip>&& mips, const TextureInfo& info)
    : ITexture(info)
    , m_persistentPayload(std::move(data))
    , m_streamingPayload(std::move(asyncData))
    , m_mips(std::move(mips))
{
    createDeviceResources();
}

StaticTexture::StaticTexture(const image::ImageView& image)
    : ITexture(TextureInfo())
{
    DEBUG_CHECK(!"Not implemented yet");
}

StaticTexture::~StaticTexture()
{
    destroyDeviceResources();
}

void StaticTexture::onPostLoad()
{
    TBaseClass::onPostLoad();

    const auto& format = GetImageFormatInfo(m_info.format);

    for (const auto& mip : m_mips)
    {
        const auto minSize = format.compressed ? 4 : 1;
        const auto alignedWidth = Align<uint32_t>(mip.width, minSize);
        const auto alignedHeight = Align<uint32_t>(mip.height, minSize);
        const auto expectedSize = ((alignedWidth * alignedHeight * mip.depth) * format.bitsPerPixel) / 8;

        DEBUG_CHECK_EX(expectedSize == mip.dataSize, TempString("Static texture '{}' mipmap [{}x{}x{}] has unexpected size {} (expected {}), format {}",
            path(), mip.width, mip.height, mip.depth, mip.dataSize, expectedSize, m_info.format));
    }

    createDeviceResources();
}

gpu::ImageSampledViewPtr StaticTexture::view() const
{
    return m_mainView ? m_mainView : gpu::Globals().TextureGray;
}

//--

class StaticTextureSourceDataProvider : public gpu::ISourceDataProvider
{
public:
	StaticTextureSourceDataProvider(Buffer data, const Array<StaticTextureMip>& mips, StringBuf path, ImageFormat format, uint32_t numMipsPerSlice)
		: m_data(data)
		, m_mips(mips)
        , m_numMipsPerSlice(numMipsPerSlice)
		, m_path(path)
		, m_format(format)
	{}

	virtual void print(IFormatStream& f) const override final
	{
		f.appendf("StaticTexture '{}'", m_path);
	}

    virtual CAN_YIELD void fetchSourceData(Array<SourceAtom>& outAtoms) const override final
	{
        for (auto index : m_mips.indexRange())
        {
            const auto& mip = m_mips[index];

            auto& atom = outAtoms.emplaceBack();
            atom.buffer = m_data;
            atom.mip = index % m_numMipsPerSlice;
            atom.slice = index / m_numMipsPerSlice;
            atom.sourceDataSize = mip.dataSize;
            atom.sourceData = m_data.data() + mip.dataOffset;
        }
	}

private:
	Buffer m_data;
	Array<StaticTextureMip> m_mips; // mip map data
	StringBuf m_path;

	ImageFormat m_format;
    uint32_t m_numMipsPerSlice;

	fibers::WaitCounter m_fence;
};

void StaticTexture::createDeviceResources()
{
    DEBUG_CHECK_EX(m_info.slices * m_info.mips == m_mips.size(), "Slice/Mip count mismatch");

    if (auto service = GetService<gpu::DeviceService>())
    {
        if (auto device = service->device())
        {
            if (!m_mainView)
            {
                gpu::ImageCreationInfo info;
                info.width = m_info.width;
                info.height = m_info.height;
                info.depth = m_info.depth;
                info.numSlices = m_info.slices;
                info.numMips = m_info.mips;
                info.view = m_info.type;
                info.format = m_info.format;
                info.allowShaderReads = true;
                info.label = StringBuf(TempString("{}", path()));

				auto data = RefNew<StaticTextureSourceDataProvider>(m_persistentPayload, m_mips, path().str(), m_info.format, info.numMips);
				if (m_object = device->createImage(info, data))
					m_mainView = m_object->createSampledView();
            }
        }
    }
}

void StaticTexture::destroyDeviceResources()
{
    m_object.reset();
	m_mainView.reset();
}

//--

END_BOOMER_NAMESPACE()

