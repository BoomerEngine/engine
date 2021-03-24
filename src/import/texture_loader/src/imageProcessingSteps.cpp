/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "imageProcessingSteps.h"
#include "core/image/include/imageUtils.h"
#include "core/image/include/imageView.h"
#include "core/image/include/image.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IImageProcessingStep);
RTTI_END_TYPE();

IImageProcessingStep::~IImageProcessingStep()
{}

//--

RTTI_BEGIN_TYPE_ENUM(ImageChannelOperation);
RTTI_ENUM_OPTION(CopyR);
RTTI_ENUM_OPTION(CopyG);
RTTI_ENUM_OPTION(CopyB);
RTTI_ENUM_OPTION(CopyA);
RTTI_ENUM_OPTION(SetZero);
RTTI_ENUM_OPTION(SetOne);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_CLASS(ImageProcessingStep_ChannelRemap);
    RTTI_CATEGORY("Image parameters");
    RTTI_PROPERTY(m_numOutputChannels).editable();
    RTTI_CATEGORY("Channel mapping");
    RTTI_PROPERTY(m_red).editable();
    RTTI_PROPERTY(m_green).editable();
    RTTI_PROPERTY(m_blue).editable();
    RTTI_PROPERTY(m_alpha).editable();
RTTI_END_TYPE();

ImageProcessingStep_ChannelRemap::ImageProcessingStep_ChannelRemap()
{}

bool ImageProcessingStep_ChannelRemap::validate(IFormatStream& outErrors) const
{
    return true;
}

union ColorDefaults
{
    uint8_t FormatUint8[4];
    uint16_t FormatUint16[4];
    uint16_t FormatFloat16[4];
    float FormatFloat32[4];
};

void ImageProcessingStep_ChannelRemap::process(ImageView& view, ImagePtr& tempImagePtr) const
{
    auto sourceView = view;

    auto channelCount = m_numOutputChannels;
    if (channelCount == 0 || channelCount >= 4)
        channelCount = view.channels();

    if (channelCount != sourceView.channels())
    {
        tempImagePtr = RefNew<Image>(view.format(), channelCount, sourceView.width(), sourceView.height(), sourceView.depth());
        ConvertChannels(sourceView, tempImagePtr->view());
        view = tempImagePtr->view();
    }

    ColorDefaults colorDefaults;

    switch (view.format())
    {
        case ImagePixelFormat::Uint8_Norm:
            colorDefaults.FormatUint8[0] = 0;
            colorDefaults.FormatUint8[1] = 0;
            colorDefaults.FormatUint8[1] = 0;
            colorDefaults.FormatUint8[2] = 255;
            break;

        case ImagePixelFormat::Uint16_Norm:
            colorDefaults.FormatUint16[0] = 0;
            colorDefaults.FormatUint16[1] = 0;
            colorDefaults.FormatUint16[1] = 0;
            colorDefaults.FormatUint16[2] = 65535;
            break;

        case ImagePixelFormat::Float16_Raw:
            colorDefaults.FormatFloat16[0] = Float16Helper::Compress(0.0f);
            colorDefaults.FormatFloat16[1] = Float16Helper::Compress(0.0f);
            colorDefaults.FormatFloat16[1] = Float16Helper::Compress(0.0f);
            colorDefaults.FormatFloat16[2] = Float16Helper::Compress(1.0f);
            break;

        case ImagePixelFormat::Float32_Raw:
            colorDefaults.FormatFloat32[0] = 0.0f;
            colorDefaults.FormatFloat32[1] = 0.0f;
            colorDefaults.FormatFloat32[1] = 0.0f;
            colorDefaults.FormatFloat32[2] = 1.0f;
            break;
    }

    uint8_t channelMappingValues[6];
    channelMappingValues[4] = channelCount; // zero
    channelMappingValues[5] = channelCount + 3; // one
    channelMappingValues[0] = 0;
    channelMappingValues[1] = (channelCount >= 2) ? 1 : channelCount;
    channelMappingValues[2] = (channelCount >= 3) ? 2 : channelCount;
    channelMappingValues[3] = (channelCount >= 4) ? 3 : channelCount+3;

    uint8_t channelMapping[4];
    channelMapping[0] = channelMappingValues[(int)m_red];
    channelMapping[1] = channelMappingValues[(int)m_green];
    channelMapping[2] = channelMappingValues[(int)m_blue];
    channelMapping[3] = channelMappingValues[(int)m_alpha];

    CopyChannels(view, channelMapping, &colorDefaults);
}

//--

END_BOOMER_NAMESPACE_EX(assets)
