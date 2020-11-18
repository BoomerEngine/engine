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

    rendering::ImageView StaticTexture::view() const
    {
        return m_mainView ? m_mainView : ImageView::DefaultGrayLinear();
    }

    //--

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

                    base::InplaceArray<rendering::SourceData, 128> sourceData;
                    sourceData.resize(info.numMips * info.numSlices);

                    auto* writePtr = sourceData.typedData();
                    for (uint32_t i = 0; i < info.numSlices; ++i)
                    {
                        for (uint32_t j = 0; j < info.numMips; ++j, ++writePtr)
                        {
                            auto sourceMipIndex = (i * m_info.mips) + j;
                            if (sourceMipIndex < m_mips.size())
                            {
                                const auto& sourceMip = m_mips[sourceMipIndex];
                                DEBUG_CHECK_EX(!sourceMip.streamed, "Streaming not yet supported");

                                const auto* dataPtr = m_persistentPayload.data() + sourceMip.dataOffset;
                                writePtr->data = m_persistentPayload;
                                writePtr->offset = sourceMip.dataOffset;
                                writePtr->size = sourceMip.dataSize;
                            }
                        }
                    }

                    if (m_object = device->createImage(info, sourceData.typedData()))
                        m_mainView = m_object->view().createSampledView(ObjectID::DefaultTrilinearSampler(false));
                }
            }
        }
    }

    void StaticTexture::destroyDeviceResources()
    {
        m_object.reset();
        m_mainView = ImageView();
    }

    //--

} // rendering

