/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "imagePreviewPanelElements.h"
#include "imagePreviewPanel.h"

#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/descriptor.h"
#include "gpu/device/include/globalObjects.h"
#include "gpu/device/include/shader.h"

#include "engine/canvas/include/batchRenderer.h"
#include "engine/canvas/include/geometryBuilder.h"
#include "engine/canvas/include/canvas.h"
#include "engine/ui/include/uiElement.h"
#include "engine/ui/include/uiTextLabel.h"

#include "core/image/include/imageView.h"
#include "gpu/device/include/shaderSelector.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

//StaticResource<ShaderLibrary> resCanvasCustomHandlerChecker("/editor/shaders/canvas_checkers.csl");

/*struct CanvasCheckersData
{
    Rect validRect;
    Vector4 colorA;
    Vector4 colorB;
};

/// custom rendering handler
class CanvasCheckersHandler : public canvas::ICanvasRendererCustomBatchHandler
{
    RTTI_DECLARE_VIRTUAL_CLASS(CanvasCheckersHandler, canvas::ICanvasRendererCustomBatchHandler);

public:
    virtual void initialize(IDevice* drv) override final
    {
    }

    virtual void render(gpu::CommandWriter& cmd, const canvas::Canvas& canvas, const canvas::CanvasRenderingParams& params, uint32_t firstIndex, uint32_t numIndices, uint32_t numPayloads, const canvas::CanvasCustomBatchPayload* payloads) override
    {
        if (auto shader = resCanvasCustomHandlerChecker.loadAndGet())
        {
            for (uint32_t i = 0; i < numPayloads; ++i)
            {
                const auto& payload = payloads[i];

                const auto& config = *(const CanvasCheckersData*)payload.data;

                struct
                {
                    ConstantsView constants;
                } params;

                params.constants = cmd.opUploadConstants(config);

                cmd.opBindParametersInline("CavasCheckerData"_id, params);
                cmd.opDrawIndexed(shader, 0, payload.firstIndex, payload.numIndices);
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(CanvasCheckersHandler);
RTTI_END_TYPE();*/

//--

/// custom rendering handler
class ICanvasImagePreviewHandler : public canvas::ICanvasSimpleBatchRenderer
{
    RTTI_DECLARE_VIRTUAL_CLASS(ICanvasImagePreviewHandler, canvas::ICanvasSimpleBatchRenderer);

public:
    struct PrivateData
    {
        gpu::ImageSampledView* texture = nullptr;
        bool showRed = true;
        bool showGreen = true;
        bool showBlue = true;
        bool showAlpha = true;
        bool pointFilter = false;
        int mip = 0;
        int slice = 0;
        int colorSpace = 0; // 0-texture is in sRGB mode - output directly, 1-texture is linear, take gamma, 2-HDR with no tone mapping, 3-HDR with tonemapping
        float colorSpaceScale = 1.0f;
        int toneMapMode = 0;
    };

    virtual void render(gpu::CommandWriter& cmd, const RenderData& data, uint32_t firstVertex, uint32_t numVertices) const override
    {
        const auto& srcData = *(const PrivateData*)data.customData;

        struct
        {
            Vector4 colorSelector;
            int mip = 0;
            int slice = 0;
            int colorSpace = 0; // 0-texture is in sRGB mode - output directly, 1-texture is linear, take gamma, 2-HDR with no tone mapping, 3-HDR with tonemapping
            float colorSpaceScale = 1.0f;
            int toneMapMode = 0;
        } constants;

        constants.colorSelector.x = srcData.showRed ? 1.0f : 0.0f;
        constants.colorSelector.y = srcData.showGreen ? 1.0f : 0.0f;
        constants.colorSelector.z = srcData.showBlue ? 1.0f : 0.0f;
        constants.colorSelector.w = srcData.showAlpha ? 1.0f : 0.0f;
        constants.mip = srcData.mip;
        constants.slice = srcData.slice;
        constants.colorSpace = srcData.colorSpace;
        constants.colorSpaceScale = srcData.colorSpaceScale;
        constants.toneMapMode = srcData.toneMapMode;

        gpu::DescriptorEntry desc[3];
        desc[0].constants(constants);
        desc[1] = srcData.pointFilter ? gpu::Globals().SamplerClampPoint : gpu::Globals().SamplerClampBiLinear;
        desc[2] = srcData.texture ? srcData.texture : gpu::Globals().TextureWhite;

        cmd.opBindDescriptor("ExtraParams"_id, desc);

        TBaseClass::render(cmd, data, firstVertex, numVertices);
    }
};

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICanvasImagePreviewHandler);
RTTI_END_TYPE();

//--

/// custom rendering handler
class CanvasImagePreviewHandler2D : public ICanvasImagePreviewHandler
{
    RTTI_DECLARE_VIRTUAL_CLASS(CanvasImagePreviewHandler2D, ICanvasImagePreviewHandler);

public:
    virtual gpu::ShaderObjectPtr loadMainShaderFile() override final
    {
        gpu::ShaderSelector selector;
        selector.set("USE_TEXTURE_2D"_id, 1);
        return gpu::LoadStaticShaderDeviceObject("editor/canvas_image_preview.fx", selector);
    }
};

RTTI_BEGIN_TYPE_CLASS(CanvasImagePreviewHandler2D);
RTTI_END_TYPE();

//--

/// custom rendering handler
class CanvasImagePreviewHandlerCube : public ICanvasImagePreviewHandler
{
    RTTI_DECLARE_VIRTUAL_CLASS(CanvasImagePreviewHandlerCube, ICanvasImagePreviewHandler);

public:
    virtual gpu::ShaderObjectPtr loadMainShaderFile() override final
    {
        gpu::ShaderSelector selector;
        selector.set("USE_TEXTURE_ARRAY"_id, 1);
        return gpu::LoadStaticShaderDeviceObject("editor/canvas_image_preview.fx", selector);
    }
};

RTTI_BEGIN_TYPE_CLASS(CanvasImagePreviewHandlerCube);
RTTI_END_TYPE();

//--

/*StaticResource<ShaderLibrary> resCanvasCustomCompressedTexturePreview("/editor/shaders/canvas_texture_compression_preview.csl");

struct CanvasTextureCompressionPreviewParams
{
        
    ImageView texture;
    ImageView texture2;
};

/// custom rendering handler
class CanvasTextureCompressionPreviewHandler : public canvas::ICanvasRendererCustomBatchHandler
{
    RTTI_DECLARE_VIRTUAL_CLASS(CanvasTextureCompressionPreviewHandler, canvas::ICanvasRendererCustomBatchHandler);

public:
    virtual void initialize(IDevice* drv) override final
    {
    }

    virtual void render(gpu::CommandWriter& cmd, const canvas::Canvas& canvas, const canvas::CanvasRenderingParams& params, uint32_t firstIndex, uint32_t numIndices, uint32_t numPayloads, const canvas::CanvasCustomBatchPayload* payloads) override
    {
        if (auto shader = resCanvasCustomCompressedTexturePreview.loadAndGet())
        {
            for (uint32_t i = 0; i < numPayloads; ++i)
            {
                const auto& payload = payloads[i];

                const auto& config = *(const CanvasTextureCompressionPreviewParams*)payload.data;

                struct
                {
                    ConstantsView constants;
                    ImageView texture;
                    ImageView texture2;
                } params;

                params.constants = cmd.opUploadConstants(config.constants);
                params.texture = config.texture;
                params.texture2 = config.texture2;

                cmd.opBindParametersInline("CanvasTextureCompressionPreviewParams"_id, params);
                cmd.opDrawIndexed(shader, 0, payload.firstIndex, payload.numIndices);
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(CanvasTextureCompressionPreviewHandler);
RTTI_END_TYPE();*/

//--

/*StaticResource<ShaderLibrary> resCanvasCustomHandlerPixelGrid("/editor/shaders/canvas_pixel_grid.csl");

/// custom rendering handler
class CanvasImagePixelGridHandler : public canvas::ICanvasRendererCustomBatchHandler
{
    RTTI_DECLARE_VIRTUAL_CLASS(CanvasImagePixelGridHandler, canvas::ICanvasRendererCustomBatchHandler);

public:
    virtual void initialize(IDevice* drv) override final
    {
    }

    virtual void render(gpu::CommandWriter& cmd, const canvas::Canvas& canvas, const canvas::CanvasRenderingParams& params, uint32_t firstIndex, uint32_t numIndices, uint32_t numPayloads, const canvas::CanvasCustomBatchPayload* payloads) override
    {
        if (auto shader = resCanvasCustomHandlerPixelGrid.loadAndGet())
        {
            for (uint32_t i = 0; i < numPayloads; ++i)
            {
                const auto& payload = payloads[i];

                const auto& config = *(const Rect*)payload.data;

                struct
                {
                    ConstantsView constants;
                } params;

                params.constants = cmd.opUploadConstants(config);

                cmd.opBindParametersInline("CanvasPixelGridParams"_id, params);
                cmd.opDrawIndexed(shader, 0, payload.firstIndex, payload.numIndices);
            }
        }
    }
};

RTTI_BEGIN_TYPE_CLASS(CanvasImagePixelGridHandler);
RTTI_END_TYPE();*/

//--

static ImageFormat BestPreviewFormat(const image::Image& image)
{
    switch (image.format())
    {
    case image::PixelFormat::Uint8_Norm:
    {
        switch (image.channels())
        {
        case 1: return ImageFormat::R8_UNORM;
        case 2: return ImageFormat::RG8_UNORM;
        case 3: return ImageFormat::RGB8_UNORM;
        case 4: return ImageFormat::RGBA8_UNORM;
        }
        break;
    }

    case image::PixelFormat::Uint16_Norm:
    {
        switch (image.channels())
        {
        case 1: return ImageFormat::R16_UNORM;
        case 2: return ImageFormat::RG16_UNORM;
        case 3: return ImageFormat::RGBA16_UNORM;
        case 4: return ImageFormat::RGBA16_UNORM;
        }
        break;
    }

    case image::PixelFormat::Float16_Raw:
    {
        switch (image.channels())
        {
        case 1: return ImageFormat::R16F;
        case 2: return ImageFormat::RG16F;
        case 3: return ImageFormat::RGBA16F;
        case 4: return ImageFormat::RGBA16F;
        }
        break;
    }

    case image::PixelFormat::Float32_Raw:
    {
        switch (image.channels())
        {
        case 1: return ImageFormat::R32F;
        case 2: return ImageFormat::RG32F;
        case 3: return ImageFormat::RGB32F;
        case 4: return ImageFormat::RGBA32F;
        }
        break;
    }
    }

    DEBUG_CHECK(!"Invalid format");
    return ImageFormat::UNKNOWN;
}

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IImagePreviewElement);
RTTI_END_TYPE();

IImagePreviewElement::~IImagePreviewElement()
{}

bool IImagePreviewElement::queryColor(float x, float y, ImagePreviewPixel& outPixel) const
{
    return false;
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(ImagePreviewElement);
RTTI_END_TYPE();

ImagePreviewElement::ImagePreviewElement(const gpu::ImageSampledView* view, int mipIndex /*= 0*/, int sliceIndex /*= 0*/)
    : m_view(AddRef(view))
    , m_mipIndex(mipIndex)
    , m_sliceIndex(sliceIndex)
{
    DEBUG_CHECK(view);
}    

ImagePreviewElement::~ImagePreviewElement()
{
}

void ImagePreviewElement::prepareGeometry(ui::CanvasArea* owner, float sx, float sy, ui::Size& outCanvasSizeAtCurrentScale)
{
    //auto width = std::max<uint32_t>(1, m_view.width() >> m_mip);
    //auto height = std::max<uint32_t>(1, m_view.height() >> m_mip);
    auto width = m_view->image()->width();
    auto height = m_view->image()->height();

    outCanvasSizeAtCurrentScale.x = width * sx;
    outCanvasSizeAtCurrentScale.y = height * sy;
}

void ImagePreviewElement::configure(const ImagePreviewPanelSettings& settings)
{
    m_settings.create(settings);
}

void ImagePreviewElement::render(ui::CanvasArea* owner, float x, float y, float sx, float sy, canvas::Canvas& canvas, float mergedOpacity)
{
    //auto width = std::max<uint32_t>(1, m_view.width() >> m_mip);
    //auto height = std::max<uint32_t>(1, m_view.height() >> m_mip);
    auto width = m_view->width();
    auto height = m_view->height();

    canvas::Canvas::QuadSetup quad;
    quad.x1 = sx * width;
    quad.y1 = sy * height;
    quad.op = m_settings->premultiply ? canvas::BlendOp::AlphaPremultiplied : canvas::BlendOp::AlphaBlend;
    quad.wrap = false;

    ICanvasImagePreviewHandler::PrivateData setup;
    setup.texture = m_view;
    setup.pointFilter = m_settings->pointFilter;
    setup.showRed = m_settings->showRed;
    setup.showGreen = m_settings->showGreen;
    setup.showBlue = m_settings->showBlue;
    setup.showAlpha = m_settings->showAlpha;
    setup.mip = m_mipIndex;
    setup.slice = m_sliceIndex;

    if (m_settings->colorSpace == assets::ImageContentColorSpace::SRGB)
    {
        setup.colorSpace = 1;
        setup.colorSpaceScale = 1.0f;
    }
    else if (m_settings->colorSpace == assets::ImageContentColorSpace::Linear)
    {
        setup.colorSpace = 0;
        setup.colorSpaceScale = 1.0f;
    }
    else if (m_settings->colorSpace == assets::ImageContentColorSpace::HDR)
    {
        setup.colorSpace = 2;
        setup.toneMapMode = m_settings->toneMapMode;
        setup.colorSpaceScale = m_settings->exposureAdj;
    }

    if (m_view->imageViewType() == ImageViewType::View2D)
        canvas.quadEx<CanvasImagePreviewHandler2D>(canvas::Placement(x, y), quad, setup);
    else if (m_view->imageViewType() == ImageViewType::View2DArray)
        canvas.quadEx<CanvasImagePreviewHandlerCube>(canvas::Placement(x, y), quad, setup);
}

//--

void RenderPixelBackground(canvas::Canvas& canvas, const ui::Position& tl, const ui::Position& br, const ui::ElementArea& drawArea, const Rect& activeImageArea, float colorFrac)
{
    /*if (!activeImageArea.empty())
    {
        Color color = Color::GRAY;
        color.a = FloatTo255(colorFrac);

        canvas::Canvas::RawVertex v[4];
        v[0].uv.x = tl.x;
        v[0].uv.y = tl.y;
        v[1].uv.x = br.x;
        v[1].uv.y = tl.y;
        v[2].uv.x = br.x;
        v[2].uv.y = br.y;
        v[3].uv.x = tl.x;
        v[3].uv.y = br.y;
        v[0].color = color;
        v[1].color = color;
        v[2].color = color;
        v[3].color = color;
        v[0].pos.x = 0.0f;
        v[0].pos.y = 0.0f;
        v[1].pos.x = drawArea.size().x;
        v[1].pos.y = 0.0f;
        v[2].pos.x = drawArea.size().x;
        v[2].pos.y = drawArea.size().y;
        v[3].pos.x = 0.0f;
        v[3].pos.y = drawArea.size().y;

        uint16_t i[6];
        i[0] = 0;
        i[1] = 1;
        i[2] = 2;
        i[3] = 0;
        i[4] = 2;
        i[5] = 3;

        canvas::Canvas::RawGeometry geom;
        geom.indices = i;
        geom.vertices = v;
        geom.numIndices = 6;

        static const auto customDrawerId = canvas::GetHandlerIndex<CanvasImagePixelGridHandler>();

        const auto style = canvas::SolidColor(Color::WHITE);
        const auto op = canvas::CompositeOperation::Blend;
        canvas.place(style, geom, customDrawerId, canvas.uploadCustomPayloadData(activeImageArea), op);
    }*/
}

//--

END_BOOMER_NAMESPACE_EX(ed)
