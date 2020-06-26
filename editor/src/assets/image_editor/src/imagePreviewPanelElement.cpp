/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "imagePreviewPanelElements.h"

#include "rendering/driver/include/renderingShaderLibrary.h"
#include "rendering/canvas/include/renderingCanvasRendererCustomBatchHandler.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingDeviceService.h"
#include "rendering/driver/include/renderingDriver.h"
#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/image/include/imageView.h"
#include "base/ui/include/uiElement.h"
#include "base/ui/include/uiTextLabel.h"
#include "imagePreviewPanel.h"

namespace ed
{

    //--

    base::res::StaticResource<rendering::ShaderLibrary> resCanvasCustomHandlerChecker("editor/shaders/canvas_checkers.csl");

    struct CanvasCheckersData
    {
        base::Rect validRect;
        base::Vector4 colorA;
        base::Vector4 colorB;
    };

    /// custom rendering handler
    class CanvasCheckersHandler : public rendering::canvas::ICanvasRendererCustomBatchHandler
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CanvasCheckersHandler, rendering::canvas::ICanvasRendererCustomBatchHandler);

    public:
        virtual void initialize(rendering::IDriver* drv) override final
        {
        }

        virtual void render(rendering::command::CommandWriter& cmd, const base::canvas::Canvas& canvas, const rendering::canvas::CanvasRenderingParams& params, uint32_t firstIndex, uint32_t numIndices, uint32_t numPayloads, const rendering::canvas::CanvasCustomBatchPayload* payloads) override
        {
            if (auto shader = resCanvasCustomHandlerChecker.loadAndGet())
            {
                for (uint32_t i = 0; i < numPayloads; ++i)
                {
                    const auto& payload = payloads[i];

                    const auto& config = *(const CanvasCheckersData*)payload.data;

                    struct
                    {
                        rendering::ConstantsView constants;
                    } params;

                    params.constants = cmd.opUploadConstants(config);

                    cmd.opBindParametersInline("CavasCheckerData"_id, params);
                    cmd.opDrawIndexed(shader, 0, payload.firstIndex, payload.numIndices);
                }
            }
        }
    };

    RTTI_BEGIN_TYPE_CLASS(CanvasCheckersHandler);
    RTTI_END_TYPE();

    //--

    base::res::StaticResource<rendering::ShaderLibrary> resCanvasCustomHandlerPreview("editor/shaders/canvas_image_preview.csl");

    struct CanvasImagePreviewData
    {
        base::Vector4 colorSelector;
        rendering::ImageView texture;
    };

    /// custom rendering handler
    class CanvasImagePreviewHandler : public rendering::canvas::ICanvasRendererCustomBatchHandler
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CanvasImagePreviewHandler, rendering::canvas::ICanvasRendererCustomBatchHandler);

    public:
        virtual void initialize(rendering::IDriver* drv) override final
        {
        }

        virtual void render(rendering::command::CommandWriter& cmd, const base::canvas::Canvas& canvas, const rendering::canvas::CanvasRenderingParams& params, uint32_t firstIndex, uint32_t numIndices, uint32_t numPayloads, const rendering::canvas::CanvasCustomBatchPayload* payloads) override
        {
            if (auto shader = resCanvasCustomHandlerPreview.loadAndGet())
            {
                for (uint32_t i = 0; i < numPayloads; ++i)
                {
                    const auto& payload = payloads[i];

                    const auto* config = (const CanvasImagePreviewData*)payload.data;

                    struct
                    {
                        rendering::ConstantsView constants;
                        rendering::ImageView texture;
                    } params;

                    params.constants = cmd.opUploadConstants(&config->colorSelector, 16);
                    params.texture = config->texture;

                    cmd.opBindParametersInline("CanvasImagePreviewParams"_id, params);
                    cmd.opDrawIndexed(shader, 0, payload.firstIndex, payload.numIndices);
                }
            }
        }
    };

    RTTI_BEGIN_TYPE_CLASS(CanvasImagePreviewHandler);
    RTTI_END_TYPE();

    //--

    base::res::StaticResource<rendering::ShaderLibrary> resCanvasCustomCompressedTexturePreview("editor/shaders/canvas_texture_compression_preview.csl");

    struct CanvasTextureCompressionPreviewParams
    {
        struct
        {
            base::Vector4 colorSelector;
            int mip = 0;
            int compareMode = 0;
            float compareScale = 1.0f;
            int colorSpace = 0; // 0-texture is in sRGB mode - output directly, 1-texture is linear, take gamma, 2-HDR with no tone mapping, 3-HDR with tonemapping
            float colorSpaceScale = 1.0f;
            int toneMapMode = 0;
        } constants;

        rendering::ImageView texture;
        rendering::ImageView texture2;
    };

    /// custom rendering handler
    class CanvasTextureCompressionPreviewHandler : public rendering::canvas::ICanvasRendererCustomBatchHandler
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CanvasTextureCompressionPreviewHandler, rendering::canvas::ICanvasRendererCustomBatchHandler);

    public:
        virtual void initialize(rendering::IDriver* drv) override final
        {
        }

        virtual void render(rendering::command::CommandWriter& cmd, const base::canvas::Canvas& canvas, const rendering::canvas::CanvasRenderingParams& params, uint32_t firstIndex, uint32_t numIndices, uint32_t numPayloads, const rendering::canvas::CanvasCustomBatchPayload* payloads) override
        {
            if (auto shader = resCanvasCustomCompressedTexturePreview.loadAndGet())
            {
                for (uint32_t i = 0; i < numPayloads; ++i)
                {
                    const auto& payload = payloads[i];

                    const auto& config = *(const CanvasTextureCompressionPreviewParams*)payload.data;

                    struct
                    {
                        rendering::ConstantsView constants;
                        rendering::ImageView texture;
                        rendering::ImageView texture2;
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
    RTTI_END_TYPE();

    //--

    base::res::StaticResource<rendering::ShaderLibrary> resCanvasCustomHandlerPixelGrid("editor/shaders/canvas_pixel_grid.csl");

    /// custom rendering handler
    class CanvasImagePixelGridHandler : public rendering::canvas::ICanvasRendererCustomBatchHandler
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CanvasImagePixelGridHandler, rendering::canvas::ICanvasRendererCustomBatchHandler);

    public:
        virtual void initialize(rendering::IDriver* drv) override final
        {
        }

        virtual void render(rendering::command::CommandWriter& cmd, const base::canvas::Canvas& canvas, const rendering::canvas::CanvasRenderingParams& params, uint32_t firstIndex, uint32_t numIndices, uint32_t numPayloads, const rendering::canvas::CanvasCustomBatchPayload* payloads) override
        {
            if (auto shader = resCanvasCustomHandlerPixelGrid.loadAndGet())
            {
                for (uint32_t i = 0; i < numPayloads; ++i)
                {
                    const auto& payload = payloads[i];

                    const auto& config = *(const base::Rect*)payload.data;

                    struct
                    {
                        rendering::ConstantsView constants;
                    } params;

                    params.constants = cmd.opUploadConstants(config);

                    cmd.opBindParametersInline("CanvasPixelGridParams"_id, params);
                    cmd.opDrawIndexed(shader, 0, payload.firstIndex, payload.numIndices);
                }
            }
        }
    };

    RTTI_BEGIN_TYPE_CLASS(CanvasImagePixelGridHandler);
    RTTI_END_TYPE();


    //--

    static rendering::ImageFormat BestPreviewFormat(const base::image::Image& image)
    {
        switch (image.format())
        {
        case base::image::PixelFormat::Uint8_Norm:
        {
            switch (image.channels())
            {
            case 1: return rendering::ImageFormat::R8_UNORM;
            case 2: return rendering::ImageFormat::RG8_UNORM;
            case 3: return rendering::ImageFormat::RGB8_UNORM;
            case 4: return rendering::ImageFormat::RGBA8_UNORM;
            }
            break;
        }

        case base::image::PixelFormat::Uint16_Norm:
        {
            switch (image.channels())
            {
            case 1: return rendering::ImageFormat::R16_UNORM;
            case 2: return rendering::ImageFormat::RG16_UNORM;
            case 3: return rendering::ImageFormat::RGBA16_UNORM;
            case 4: return rendering::ImageFormat::RGBA16_UNORM;
            }
            break;
        }

        case base::image::PixelFormat::Float16_Raw:
        {
            switch (image.channels())
            {
            case 1: return rendering::ImageFormat::R16F;
            case 2: return rendering::ImageFormat::RG16F;
            case 3: return rendering::ImageFormat::RGBA16F;
            case 4: return rendering::ImageFormat::RGBA16F;
            }
            break;
        }

        case base::image::PixelFormat::Float32_Raw:
        {
            switch (image.channels())
            {
            case 1: return rendering::ImageFormat::R32F;
            case 2: return rendering::ImageFormat::RG32F;
            case 3: return rendering::ImageFormat::RGB32F;
            case 4: return rendering::ImageFormat::RGBA32F;
            }
            break;
        }
        }

        DEBUG_CHECK(!"Invalid format");
        return rendering::ImageFormat::UNKNOWN;
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

    ImagePreviewElement::ImagePreviewElement(rendering::ImageView view, rendering::ImageView sourceView)
        : m_view(view)
        , m_sourceView(sourceView)
    {}

    ImagePreviewElement::ImagePreviewElement(rendering::ImageView view, int mipIndex /*= 0*/, int sliceIndex /*= 0*/)
        : m_view(view)
        , m_sourceView(view)
        , m_mipIndex(mipIndex)
        , m_sliceIndex(sliceIndex)
    {}    

    ImagePreviewElement::~ImagePreviewElement()
    {
    }

    void ImagePreviewElement::prepareGeometry(ui::CanvasArea* owner, float sx, float sy, ui::Size& outCanvasSizeAtCurrentScale)
    {
        //auto width = std::max<uint32_t>(1, m_view.width() >> m_mip);
        //auto height = std::max<uint32_t>(1, m_view.height() >> m_mip);
        auto width = m_view.width();
        auto height = m_view.height();

        outCanvasSizeAtCurrentScale.x = width * sx;
        outCanvasSizeAtCurrentScale.y = height * sy;
    }

    void ImagePreviewElement::configure(const ImagePreviewPanelSettings& settings)
    {
        m_settings.create(settings);
    }

    void ImagePreviewElement::render(ui::CanvasArea* owner, float x, float y, float sx, float sy, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        if (!m_settings)
            return;

        //auto width = std::max<uint32_t>(1, m_view.width() >> m_mip);
        //auto height = std::max<uint32_t>(1, m_view.height() >> m_mip);
        auto width = m_view.width();
        auto height = m_view.height();

        base::canvas::Canvas::RawVertex v[4];
        v[0].uv.x = 0.0f;
        v[0].uv.y = 0.0f;
        v[1].uv.x = 1.0f;
        v[1].uv.y = 0.0f;
        v[2].uv.x = 1.0f;
        v[2].uv.y = 1.0f;
        v[3].uv.x = 0.0f;
        v[3].uv.y = 1.0f;
        v[0].color = base::Color::WHITE;
        v[1].color = base::Color::WHITE;
        v[2].color = base::Color::WHITE;
        v[3].color = base::Color::WHITE;
        v[0].pos.x = 0;
        v[0].pos.y = 0;
        v[1].pos.x = width;
        v[1].pos.y = 0;
        v[2].pos.x = width;
        v[2].pos.y = height;
        v[3].pos.x = 0;
        v[3].pos.y = height;

        uint16_t i[6];
        i[0] = 0;
        i[1] = 1;
        i[2] = 2;
        i[3] = 0;
        i[4] = 2;
        i[5] = 3;

        base::canvas::Canvas::RawGeometry geom;
        geom.indices = i;
        geom.vertices = v;
        geom.numIndices = 6;

        static const auto customDrawerId = rendering::canvas::GetHandlerIndex<CanvasTextureCompressionPreviewHandler>();

        CanvasTextureCompressionPreviewParams tileData;
        if (m_settings->pointFilter)
        {
            tileData.texture = m_view.createSampledView(rendering::ObjectID::DefaultPointSampler());
            tileData.texture2 = m_sourceView.createSampledView(rendering::ObjectID::DefaultPointSampler());
        }
        else
        {
            tileData.texture = m_view.createSampledView(rendering::ObjectID::DefaultBilinearSampler());
            tileData.texture2 = m_sourceView.createSampledView(rendering::ObjectID::DefaultBilinearSampler());
        }
        tileData.constants.compareMode = m_settings->previewMode;
        tileData.constants.compareScale = m_settings->previewScale;
        tileData.constants.mip = m_mipIndex;
        tileData.constants.colorSelector.x = m_settings->showRed ? 1.0f : 0.0f;
        tileData.constants.colorSelector.y = m_settings->showGreen ? 1.0f : 0.0f;
        tileData.constants.colorSelector.z = m_settings->showBlue ? 1.0f : 0.0f;
        tileData.constants.colorSelector.w = m_settings->showAlpha ? 1.0f : 0.0f;

        if (m_settings->colorSpace == rendering::ImageContentColorSpace::SRGB)
        {
            tileData.constants.colorSpace = 1;
            tileData.constants.colorSpaceScale = 1.0f;
        }
        else if (m_settings->colorSpace == rendering::ImageContentColorSpace::Linear)
        {
            tileData.constants.colorSpace = 0;
            tileData.constants.colorSpaceScale = 1.0f;
        }
        else if (m_settings->colorSpace == rendering::ImageContentColorSpace::HDR)
        {
            tileData.constants.colorSpace = 2;
            tileData.constants.toneMapMode = m_settings->toneMapMode;
            tileData.constants.colorSpaceScale = m_settings->exposureAdj;
        }

        const auto style = base::canvas::SolidColor(base::Color::WHITE);
        const auto op = m_settings->premultiply ? base::canvas::CompositeOperation::SourceOver : base::canvas::CompositeOperation::Blend;
        canvas.place(style, geom, customDrawerId, canvas.uploadCustomPayloadData(tileData), op);
    }

    //--

    void RenderPixelBackground(base::canvas::Canvas& canvas, const ui::Position& tl, const ui::Position& br, const ui::ElementArea& drawArea, const base::Rect& activeImageArea, float colorFrac)
    {
        if (!activeImageArea.empty())
        {
            base::Color color = base::Color::GRAY;
            color.a = base::FloatTo255(colorFrac);

            base::canvas::Canvas::RawVertex v[4];
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

            base::canvas::Canvas::RawGeometry geom;
            geom.indices = i;
            geom.vertices = v;
            geom.numIndices = 6;

            static const auto customDrawerId = rendering::canvas::GetHandlerIndex<CanvasImagePixelGridHandler>();

            const auto style = base::canvas::SolidColor(base::Color::WHITE);
            const auto op = base::canvas::CompositeOperation::Blend;
            canvas.place(style, geom, customDrawerId, canvas.uploadCustomPayloadData(activeImageArea), op);
        }
    }

    //--

} // ed
