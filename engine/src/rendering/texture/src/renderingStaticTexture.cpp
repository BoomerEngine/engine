/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "renderingStaticTexture.h"
#include "base/resource/include/resourceTags.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingDeviceGlobalObjects.h"
#include "rendering/device/include/renderingResources.h"

namespace rendering
{

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
        RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4stex");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Static Texture");
        RTTI_METADATA(base::res::ResourceTagColorMetadata).color(0xa8, 0xd6, 0xe2);
        RTTI_PROPERTY(m_persistentPayload);
        RTTI_PROPERTY(m_streamingPayload);
        RTTI_PROPERTY(m_mips);
    RTTI_END_TYPE();

    StaticTexture::StaticTexture()
        : ITexture(TextureInfo())
    {
    }

    StaticTexture::StaticTexture(base::Buffer&& data, base::res::AsyncBuffer&& asyncData, base::Array<StaticTextureMip>&& mips, const TextureInfo& info)
        : ITexture(info)
        , m_persistentPayload(std::move(data))
        , m_streamingPayload(std::move(asyncData))
        , m_mips(std::move(mips))
    {
        createDeviceResources();
    }

    StaticTexture::StaticTexture(const base::image::ImageView& image)
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
        createDeviceResources();
    }

    rendering::ImageSampledViewPtr StaticTexture::view() const
    {
        return m_mainView ? m_mainView : rendering::Globals().TextureGray;
    }

    //--

	class StaticTextureSourceDataProvider : public rendering::ISourceDataProvider
	{
	public:
		StaticTextureSourceDataProvider(base::Buffer data, const base::Array<StaticTextureMip>& mips, base::StringBuf path, ImageFormat format, base::fibers::WaitCounter loadingFence = base::fibers::WaitCounter())
			: m_data(data)
			, m_mips(mips)
			, m_path(path)
			, m_format(format)
			, m_fence(loadingFence)
		{}

		virtual void notifyFinshed(bool success) override final
		{
			if (!m_fence.empty())
			{
				Fibers::GetInstance().signalCounter(m_fence);
				m_fence = base::fibers::WaitCounter();
			}
		}

		virtual void print(base::IFormatStream& f) const override final
		{
			f.appendf("StaticTexture '{}'", m_path);
		}

		virtual void writeSourceData(const base::Array<WriteAtom>& atoms) const override final
		{
			DEBUG_CHECK(m_mips.size() == atoms.size());

			const auto numCopiableAtoms = std::min<uint32_t>(atoms.size(), m_mips.size());
			for (uint32_t i = 0; i<numCopiableAtoms; ++i)
			{
				const auto& mip = m_mips[i];
				const auto& atom = atoms[i];

				//DEBUG_CHECK(atom.targetDataSize == mip.dataSize);
				const auto copySize = std::min<uint32_t>(atom.targetDataSize, mip.dataSize);
				memcpy(atom.targetDataPtr, m_data.data() + mip.dataOffset, copySize);
			}
		}

	private:
		base::Buffer m_data;
		base::Array<StaticTextureMip> m_mips; // mip map data
		base::StringBuf m_path;
		ImageFormat m_format;

		base::fibers::WaitCounter m_fence;
	};

    void StaticTexture::createDeviceResources()
    {
        DEBUG_CHECK_EX(m_info.slices * m_info.mips == m_mips.size(), "Slice/Mip count mismatch");

        if (auto service = base::GetService<rendering::DeviceService>())
        {
            if (auto device = service->device())
            {
                if (!m_mainView)
                {
                    rendering::ImageCreationInfo info;
                    info.width = m_info.width;
                    info.height = m_info.height;
                    info.depth = m_info.depth;
                    info.numSlices = m_info.slices;
                    info.numMips = m_info.mips;
                    info.view = m_info.type;
                    info.format = m_info.format;
                    info.allowShaderReads = true;
                    info.label = base::StringBuf(base::TempString("{}", path()));

					auto fence = Fibers::GetInstance().createCounter("StaticTextureInit", 1);
					auto data = base::RefNew<StaticTextureSourceDataProvider>(m_persistentPayload, m_mips, path(), m_info.format, fence);

					if (m_object = device->createImage(info, data))
						m_mainView = m_object->createSampledView();

					Fibers::GetInstance().waitForCounterAndRelease(fence);
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

} // rendering

