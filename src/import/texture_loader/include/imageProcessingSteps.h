/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(assets)

//--

class IMPORT_TEXTURE_LOADER_API IImageProcessingStep : public base::IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IImageProcessingStep, base::IObject);

public:
    virtual ~IImageProcessingStep();
    virtual bool validate(base::IFormatStream& outErrors) const { return true; }
    virtual void process(base::image::ImageView& view, base::image::ImagePtr& tempImagePtr) const = 0;
};

//--

enum class ImageChannelOperation : uint8_t
{
    CopyR=0, // copy "red" channel into this channel
    CopyG, // copy "green" channel into this channel
    CopyB, // copy "blue" channel into this channel
    CopyA, // copy "alpha" channel into this channel
    SetZero, // set to zeros
    SetOne, // set to white/all ones
};

/// channel remapping step
class IMPORT_TEXTURE_LOADER_API ImageProcessingStep_ChannelRemap : public IImageProcessingStep
{
    RTTI_DECLARE_VIRTUAL_CLASS(ImageProcessingStep_ChannelRemap, IImageProcessingStep);

public:
    ImageProcessingStep_ChannelRemap();

    ImageChannelOperation m_red = ImageChannelOperation::CopyR;
    ImageChannelOperation m_green = ImageChannelOperation::CopyG;
    ImageChannelOperation m_blue = ImageChannelOperation::CopyB;
    ImageChannelOperation m_alpha = ImageChannelOperation::CopyA;

    uint8_t m_numOutputChannels = 0;

protected:
    virtual bool validate(base::IFormatStream& outErrors) const override;
    virtual void process(base::image::ImageView& view, base::image::ImagePtr& tempImagePtr) const override;
};

//--

END_BOOMER_NAMESPACE(assets)