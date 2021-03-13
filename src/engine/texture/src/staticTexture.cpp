/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "staticTexture.h"

#include "core/resource/include/tags.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/globalObjects.h"
#include "gpu/device/include/resources.h"

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

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IStaticTexture);
    RTTI_PROPERTY(m_persistentPayload);
    RTTI_PROPERTY(m_streamingPayload);
    RTTI_PROPERTY(m_mips);
RTTI_END_TYPE();

IStaticTexture::IStaticTexture()
    : ITexture(TextureInfo())
{}

IStaticTexture::IStaticTexture(Setup&& setup)
    : ITexture(setup.info)
    , m_persistentPayload(std::move(setup.data))
    , m_streamingPayload(std::move(setup.asyncData))
    , m_mips(std::move(setup.mips))
{
    createDeviceResources();
}

IStaticTexture::~IStaticTexture()
{
    destroyDeviceResources();
}

void IStaticTexture::onPostLoad()
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
            loadPath(), mip.width, mip.height, mip.depth, mip.dataSize, expectedSize, m_info.format));
    }

    createDeviceResources();
}

gpu::ImageSampledViewPtr IStaticTexture::view() const
{
    return m_mainView ? m_mainView : gpu::Globals().TextureGray;
}

//--

class StaticTextureSourceDataProvider : public gpu::ISourceDataProvider
{
public:
	StaticTextureSourceDataProvider(CompressedBufer data, const Array<StaticTextureMip>& mips, StringBuf path, ImageFormat format, uint32_t numMipsPerSlice)
		: m_data(data)
		, m_mips(mips)
        , m_numMipsPerSlice(numMipsPerSlice)
		, m_path(path)
		, m_format(format)
	{}

	virtual void print(IFormatStream& f) const override final
	{
		f.appendf("IStaticTexture '{}'", m_path);
	}

    virtual CAN_YIELD void fetchSourceData(Array<SourceAtom>& outAtoms) const override final
	{
        const auto decompressed = m_data.decompress();

        for (auto index : m_mips.indexRange())
        {
            const auto& mip = m_mips[index];
            
            auto& atom = outAtoms.emplaceBack();
            atom.buffer = decompressed;
            atom.mip = index % m_numMipsPerSlice;
            atom.slice = index / m_numMipsPerSlice;
            atom.sourceDataSize = mip.dataSize;
            atom.sourceData = decompressed.data() + mip.dataOffset;
        }
	}

private:
    CompressedBufer m_data;
	Array<StaticTextureMip> m_mips; // mip map data
	StringBuf m_path;

	ImageFormat m_format;
    uint32_t m_numMipsPerSlice;

	FiberSemaphore m_fence;
};

void IStaticTexture::createDeviceResources()
{
    DEBUG_CHECK_RETURN_EX(m_info.slices * m_info.mips == m_mips.size(), "Slice/Mip count mismatch");

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
                info.label = StringBuf(TempString("{}", loadPath()));

				auto data = RefNew<StaticTextureSourceDataProvider>(m_persistentPayload, m_mips, loadPath(), m_info.format, info.numMips);
				if (m_object = device->createImage(info, data))
					m_mainView = m_object->createSampledView();
            }
        }
    }
}

void IStaticTexture::destroyDeviceResources()
{
    m_object.reset();
	m_mainView.reset();
}

//--

END_BOOMER_NAMESPACE()

