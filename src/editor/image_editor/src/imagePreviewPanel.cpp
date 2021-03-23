/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "imagePreviewPanel.h"
#include "imagePreviewPanelElements.h"

#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/device.h"

#include "engine/canvas/include/geometryBuilder.h"
#include "engine/canvas/include/canvas.h"
#include "core/image/include/imageView.h"
#include "engine/ui/include/uiElement.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiRuler.h"
#include "engine/ui/include/uiComboBox.h"
#include "engine/ui/include/uiTrackBar.h"
#include "engine/ui/include/uiWindow.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_CLASS(ImagePreviewPanel);
RTTI_END_TYPE();

ImagePreviewPanel::ImagePreviewPanel()
{
    hitTest(true);
    m_maxZoom = 20.0f;
}

ImagePreviewPanel::~ImagePreviewPanel()
{}

void ImagePreviewPanel::previewSettings(const ImagePreviewPanelSettings& settings)
{
    m_previewSettings = settings;
}

void ImagePreviewPanel::renderForeground(ui::DataStash& stash, const ui::ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    TBaseClass::renderForeground(stash, drawArea, canvas, mergedOpacity);

    ui::Position minPos = ui::Position::INF();
    ui::Position maxPos = -ui::Position::INF();

    for (auto* proxy : m_elementProxies)
    {
        minPos = minPos.min(proxy->virtualPlacement);
        maxPos = maxPos.max(proxy->virtualPlacement + proxy->virtualSize);
    }

    if (m_hoverPixelValid)
    {
        if (m_viewScale.x > 1.0f)
        {
            float x0 = ((m_hoverPixelX) * m_viewScale.x) + m_viewOffset.x;
            float x1 = ((m_hoverPixelX+1) * m_viewScale.x) + m_viewOffset.x;
            float y0 = ((m_hoverPixelY) * m_viewScale.y) + m_viewOffset.y;
            float y1 = ((m_hoverPixelY+1)*m_viewScale.y) + m_viewOffset.y;

            canvas::InplaceGeometryBuilder b(canvas);
            b.fillColor(Color(64, 64, 64, 64));
            b.beginPath();
            b.rect(x0, 0.0f, x1 - x0, drawArea.size().y);
            b.fill();
            b.beginPath();
            b.rect(0.0f, y0, drawArea.size().x, y1 - y0);
            b.fill();
            b.render(drawArea.absolutePosition());
        }
        else
        {
            float x = ((m_hoverPixelX)*m_viewScale.x) + m_viewOffset.x;
            float y = ((m_hoverPixelY)*m_viewScale.y) + m_viewOffset.y;

            canvas::InplaceGeometryBuilder b(canvas);
			b.strokeColor(Color(64, 64, 64, 64));
			b.beginPath();
			b.moveTo(x, 0.0f);
			b.lineTo(x, drawArea.size().y);
			b.stroke();
			b.beginPath();
			b.moveTo(0.0f, y);
			b.lineTo(drawArea.size().x, y);
			b.stroke();
            b.render(drawArea.absolutePosition());
        }
    }

    if (m_previewSettings.showGrid && m_viewScale.x >= 5.0f)
    {
        if (minPos.x < maxPos.x || minPos.y < maxPos.y)
        {
            Rect activeImageRect;
            activeImageRect.min.x = (int)std::floor(minPos.x);
            activeImageRect.min.y = (int)std::floor(minPos.y);
            activeImageRect.max.x = (int)std::ceil(maxPos.x);
            activeImageRect.max.y = (int)std::ceil(maxPos.y);

            const auto colorFrac = std::clamp((m_viewScale.x - 5.0f) / 5.0f, 0.0f, 1.0f);
            const auto tl = absoluteToVirtual(cachedDrawArea().absolutePosition());
            const auto br = absoluteToVirtual(cachedDrawArea().absolutePosition() + cachedDrawArea().size());

            RenderPixelBackground(canvas, tl, br, cachedDrawArea(), activeImageRect, colorFrac);
        }
    }
}

void ImagePreviewPanel::renderBackground(ui::DataStash& stash, const ui::ElementArea& drawArea, canvas::Canvas& canvas, float mergedOpacity)
{
    TBaseClass::renderBackground(stash, drawArea, canvas, mergedOpacity);
}

void ImagePreviewPanel::updateTooltip() const
{
    auto text = m_activeTooltipText.lock();
    auto color = m_activeTooltipColor.lock();

    if (m_previewPixelValid)
    {
        Color previewColor;
        if (m_previewPixel.integerValues)
        {
            previewColor.r = FloatTo255(m_previewPixel.color.x / m_previewPixel.maxValue);
            previewColor.g = FloatTo255(m_previewPixel.color.y / m_previewPixel.maxValue);
            previewColor.b = FloatTo255(m_previewPixel.color.z / m_previewPixel.maxValue);
            previewColor.a = 255;
        }
        else
        {
            previewColor.r = FloatTo255(m_previewPixel.color.x);
            previewColor.g = FloatTo255(m_previewPixel.color.y);
            previewColor.b = FloatTo255(m_previewPixel.color.z);
        }

        if (color)            
            color->customBackgroundColor(previewColor);

        StringBuilder txt;
        txt.appendf("[{},{}]\n\n", m_previewPixel.x, m_previewPixel.y);

        if (m_previewPixel.channels >= 1)
        {
            txt.append("[color:#F00]R:[/color] ");
            if (m_previewPixel.integerValues)
                txt.appendf("{}", (int)m_previewPixel.color.x);
            else
                txt.appendf("{}", Prec(m_previewPixel.color.x, 2));
            txt.append("\n");
        }

        if (m_previewPixel.channels >= 2)
        {
            txt.append("[color:#0F0]G:[/color] ");
            if (m_previewPixel.integerValues)
                txt.appendf("{}", (int)m_previewPixel.color.y);
            else
                txt.appendf("{}", Prec(m_previewPixel.color.y, 2));
            txt.append("\n");
        }

        if (m_previewPixel.channels >= 3)
        {
            txt.append("[color:#00F]B:[/color] ");
            if (m_previewPixel.integerValues)
                txt.appendf("{}", (int)m_previewPixel.color.z);
            else
                txt.appendf("{}", Prec(m_previewPixel.color.z, 2));
            txt.append("\n");
        }

        if (m_previewPixel.channels >= 4)
        {
            txt.append("[color:#AAA]A:[/color] ");
            if (m_previewPixel.integerValues)
                txt.appendf("{}", (int)m_previewPixel.color.w);
            else
                txt.appendf("{}", Prec(m_previewPixel.color.w, 2));
            txt.append("\n");
        }

        if (text)
            text->text(txt.view());
    }
    else if (color)
    {
        if (auto window = color->findParentWindow())
            window->requestClose();
    }
}

bool ImagePreviewPanel::handleMouseMovement(const InputMouseMovementEvent& evt)
{
    m_previewPixelValid = false;

    const auto virtualPos = absoluteToVirtual(evt.absolutePosition());

    m_hoverPixelValid = true;
    m_hoverPixelX = (int)std::floor(virtualPos.x);
    m_hoverPixelY = (int)std::floor(virtualPos.y);

    for (const auto* proxy : m_elementProxies)
    {
        if (auto imageElement = rtti_cast<IImagePreviewElement>(proxy->element))
        {
            if (virtualPos.x >= proxy->virtualPlacement.x && virtualPos.x >= proxy->virtualPlacement.y)
            {
                float x = virtualPos.x - proxy->virtualPlacement.x;
                float y = virtualPos.y - proxy->virtualPlacement.y;
                if (x < proxy->virtualSize.x && y < proxy->virtualSize.y)
                {
                    if (imageElement->queryColor(x, y, m_previewPixel))
                    {
                        m_previewPixelValid = true;
                        break;
                    }
                }
            }
        }
    }

    updateTooltip();

    return TBaseClass::handleMouseMovement(evt);
}

void ImagePreviewPanel::handleHoverLeave(const ui::Position& absolutePosition)
{
    TBaseClass::handleHoverLeave(absolutePosition);

    m_hoverPixelValid = false;
    m_previewPixelValid = false;
    updateTooltip();
}

ui::ElementPtr ImagePreviewPanel::queryTooltipElement(const ui::Position& absolutePosition, ui::ElementArea& outArea) const
{
    if (!m_previewPixelValid)
        return nullptr;

    auto list = RefNew<ui::IElement>();
    list->layoutVertical();

    auto color = list->createChild<ui::IElement>();
    color->customStyle<float>("border-width"_id, 1.0f);
    color->customStyle<float>("border-radius"_id, 8.0f);
    color->customPadding(2.0f);
    color->customMargins(6.0f);
    color->customMinSize(80, 80);
    m_activeTooltipColor = color;

    auto text = list->createChild<ui::TextLabel>();
    color->customMargins(6.0f);
    m_activeTooltipText = text;

    outArea = cachedDrawArea();

    updateTooltip();

    return list;
}

//--

RTTI_BEGIN_TYPE_CLASS(ImagePreviewPanelWithToolbar);
RTTI_END_TYPE();

ImagePreviewPanelWithToolbar::ImagePreviewPanelWithToolbar()
{
    layoutVertical();

    // panel with rullers
    {
        m_previewToolbar = createChild<ui::ToolBar>();
        m_previewToolbar->name("PreviewToolbar"_id);
        m_previewToolbar->customMargins(0.0f, 0.0f, 0.0f, 5.0f);

        {
            auto topPanel = createChild<ui::IElement>();
            topPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            topPanel->customVerticalAligment(ui::ElementVerticalLayout::Top);
            topPanel->layoutHorizontal();
            topPanel->createChildWithType<ui::IElement>("RulerCorner"_id);
            m_previewHorizontalRuler = topPanel->createChild<ui::HorizontalRuler>();
        }

        {
            auto innerPanel = createChild<ui::IElement>();
            innerPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            innerPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            innerPanel->layoutHorizontal();

            m_previewVerticalRuler = innerPanel->createChild<ui::VerticalRuler>();
            m_previewPanel = innerPanel->createChild<ImagePreviewPanel>();
            m_previewPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_previewPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);
        }


        m_previewPanel->bind(ui::EVENT_CANVAS_VIEW_CHANGED) = [this]()
        {
            m_previewHorizontalRuler->region(m_previewPanel->virtualAreaLeft(), m_previewPanel->virtualAreaRight());
            m_previewVerticalRuler->region(m_previewPanel->virtualAreaTop(), m_previewPanel->virtualAreaBottom());
        };
    }

    // basic toolbar
    {
        m_previewToolbar->createSeparator();

        m_previewToolbar->createButton(ui::ToolBarButtonInfo("RedChannel"_id).caption("Red", "channel_red").tooltip("Toggle display of the [b][color:#F00]RED[/color][/b] channel")) = [this] {
            auto settings = previewSettings();
            settings.showRed = !settings.showRed;
            previewSettings(settings);
        };

        m_previewToolbar->createButton(ui::ToolBarButtonInfo("GreenChannel"_id).caption("Green", "channel_green").tooltip("Toggle display of the [b][color:#0F0]GREEN[/color][/b] channel")) = [this] {
            auto settings = previewSettings();
            settings.showGreen = !settings.showGreen;
            previewSettings(settings);
        };

        m_previewToolbar->createButton(ui::ToolBarButtonInfo("BlueChannel"_id).caption("Blue", "channel_blue").tooltip("Toggle display of the [b][color:#00F]BLUE[/color][/b] channel")) = [this] {
            auto settings = previewSettings();
            settings.showBlue = !settings.showBlue;
            previewSettings(settings);
        };

        m_previewToolbar->createButton(ui::ToolBarButtonInfo("AlphaChannel"_id).caption("Blue", "channel_alpha").tooltip("Toggle display of the [b][color:#FFF]ALPHA[/color][/b] channel")) = [this] {
            auto settings = previewSettings();
            settings.showAlpha = !settings.showAlpha;
            previewSettings(settings);
        };

        m_previewToolbar->createSeparator();

        m_previewToolbar->createButton(ui::ToolBarButtonInfo("PointFilter"_id).caption("Point filter", "plane").tooltip("Toggle point filtering of texture")) = [this] {
            auto settings = previewSettings();
            settings.pointFilter = !settings.pointFilter;
            previewSettings(settings);
        };

        m_previewToolbar->createButton(ui::ToolBarButtonInfo("Premultiply"_id).caption("Alpha premultiply", "blend").tooltip("Toggle premultiplied blending")) = [this] {
            auto settings = previewSettings();
            settings.premultiply = !settings.premultiply;
            previewSettings(settings);
        };

        m_previewToolbar->createButton(ui::ToolBarButtonInfo("Checkers"_id).caption("Background", "background").tooltip("Toggle checked background")) = [this] {
            auto settings = previewSettings();
            settings.showCheckers = !settings.showCheckers;
            previewSettings(settings);
        };

        m_previewToolbar->createSeparator();

        m_previewToolbar->createButton(ui::ToolBarButtonInfo("PixelGrid"_id).caption("Pixel grid", "grid").tooltip("Toggle pixel grid overlay")) = [this] {
            auto settings = previewSettings();
            settings.showGrid = !settings.showGrid;
            previewSettings(settings);
        };

        m_previewToolbar->createSeparator();

        m_previewToolbar->createButton(ui::ToolBarButtonInfo().caption("Fit", "zoom").tooltip("Show all content")) = [this] {  m_previewPanel->zoomToFit(); };
        m_previewToolbar->createButton(ui::ToolBarButtonInfo().caption("Fill", "arrow_inout").tooltip("Fill window with content")) = [this] {  m_previewPanel->zoomToFill(); };
        m_previewToolbar->createButton(ui::ToolBarButtonInfo().caption("Reset", "1to1").tooltip("Reset zoom to 1:1")) = [this] { m_previewPanel->resetZoomToOne(); };

        bindShortcut("Home") = [this] {  m_previewPanel->zoomToFit(); };

        //--
    }

    // mip selector
    {
        m_previewToolbar->createSeparator();

        m_mipmapChoiceBox = m_previewToolbar->createChildWithType<ui::ComboBox>("ToolbarComboBox"_id);
        m_mipmapChoiceBox->customMinSize(150, 10);
        m_mipmapChoiceBox->addOption("No mipmap");
        m_mipmapChoiceBox->selectOption(0);
        m_mipmapChoiceBox->visibility(false);

        m_mipmapChoiceBox->bind(ui::EVENT_COMBO_SELECTED) = [this](int option)
        {
            auto settings = previewSettings();
            settings.selectedMip = option;
            previewSettings(settings);
        };
    }

    // slice selector
    {
        m_sliceChoicebox = m_previewToolbar->createChildWithType<ui::ComboBox>("ToolbarComboBox"_id);
        m_sliceChoicebox->customMinSize(100, 10);
        m_sliceChoicebox->addOption("No slices");
        m_sliceChoicebox->selectOption(0);
        m_sliceChoicebox->visibility(false);

        m_sliceChoicebox->bind(ui::EVENT_COMBO_SELECTED) = [this](int option)
        {
            auto settings = previewSettings();
            settings.selectedSlice = option;
            previewSettings(settings);
        };
    }

    // HDR exposure/tonemap
    {
        m_previewToolbar->createSeparator();
        //m_previewToolbar->createButton(ui::ToolBarButtonInfo("ToneMap").caption("Tonemap").tooltip("Apply HDR tonemapping to image preview")) 

        m_toneMapMode = m_previewToolbar->createChildWithType<ui::ComboBox>("ToolbarComboBox"_id);
        m_toneMapMode->customMinSize(150, 10);
        m_toneMapMode->addOption("No tone mapping");
        m_toneMapMode->addOption("Linear");
        m_toneMapMode->addOption("Simple Reinhard");
        m_toneMapMode->addOption("Luma based Reinhard");
        m_toneMapMode->addOption("WP Luma based Reinhard");
        m_toneMapMode->addOption("RomBinDaHouse");
        m_toneMapMode->addOption("Filmic");
        m_toneMapMode->addOption("Uncharted 2");
        m_toneMapMode->selectOption(0);
        m_toneMapMode->visibility(false);

        m_toneMapMode->bind(ui::EVENT_COMBO_SELECTED) = [this](int option)
        {
            auto settings = previewSettings();
            settings.toneMapMode = option;
            previewSettings(settings);
        };
            
        m_exposureControl = m_previewToolbar->createChild<ui::TrackBar>();
        m_exposureControl->range(-5.0f, 5.0f);
        m_exposureControl->value(0.0f);
        m_exposureControl->resolution(1);
        m_exposureControl->units(" EV");
        m_exposureControl->visibility(false);

        m_exposureControl->bind(ui::EVENT_TRACK_VALUE_CHANGED) = [this]()
        {
            auto settings = previewSettings();
            settings.exposureAdj = m_exposureControl->value();
            previewSettings(settings);
        };
    }

    // initial update
    updateMipmapList();
    updateSliceList();
}

ImagePreviewPanelWithToolbar::~ImagePreviewPanelWithToolbar()
{}

const ImagePreviewPanelSettings& ImagePreviewPanelWithToolbar::previewSettings() const
{
    return m_previewPanel->previewSettings();
}

void ImagePreviewPanelWithToolbar::previewSettings(const ImagePreviewPanelSettings& settings)
{
    auto oldMip = previewSettings().selectedMip;
    auto oldSlice = previewSettings().selectedSlice;

    m_previewPanel->previewSettings(settings);

    if (settings.selectedMip != oldMip || settings.selectedSlice != oldSlice)
        recreatePreviewItems();

    updateUIState();
}

static gpu::ImageSampledViewPtr AdaptView(const gpu::ImageSampledView* view)
{
    if (!view)
        return nullptr;

    auto imageType = view->image()->type();
    if (imageType == ImageViewType::ViewCube)
    {
        return view->image()->createSampledViewEx(ImageViewType::View2DArray, 0, 0, view->image()->mips(), 6);
    }
    else if (imageType == ImageViewType::ViewCubeArray)
    {
        return nullptr;
    }

    return AddRef(view);
}

void ImagePreviewPanelWithToolbar::bindImageView(const gpu::ImageSampledView* view, assets::ImageContentColorSpace knownColorSpace)
{
    if (view && knownColorSpace == assets::ImageContentColorSpace::Auto)
    {
        const auto formatClass = GetImageFormatInfo(view->image()->format()).formatClass;
        if (formatClass == ImageFormatClass::SRGB)
            knownColorSpace = assets::ImageContentColorSpace::SRGB;
        else if (formatClass == ImageFormatClass::FLOAT)
            knownColorSpace = assets::ImageContentColorSpace::HDR;
    }

    bool hadContent = !m_imageSRV.empty();
	bool biggerContent = hadContent && (view->width() > m_imageSRV->width() || view->height() > m_imageSRV->height());
    auto settings = previewSettings();

    m_imageSRV = AdaptView(view);
    if (m_imageSRV)
    {
        m_imageIsCube = m_imageSRV->image()->type() == ImageViewType::ViewCube || m_imageSRV->image()->type() == ImageViewType::ViewCubeArray;
        m_numImageMips = m_imageSRV->mips();
        m_numImageSlices = m_imageSRV->slices();

        if (m_numImageMips <= 1)
            settings.selectedMip = 0;
        else if (settings.selectedMip >= m_numImageMips)
            settings.selectedMip = m_numImageMips - 1;

        if (m_numImageSlices <= 1)
            settings.selectedSlice = 0;
        else if (settings.selectedSlice >= m_numImageSlices)
            settings.selectedSlice = m_numImageSlices - 1;

        settings.colorSpace = knownColorSpace;

        m_previewPanel->previewSettings(settings);

        updateMipmapList();
        updateSliceList();
        updateUIState();
    }
    else
    {
        m_numImageMips = 0;
        m_numImageSlices = 0;
        m_imageIsCube = false;
    }

    recreatePreviewItems();

    if (!hadContent || biggerContent)
        m_previewPanel->zoomToFit();
}

void ImagePreviewPanelWithToolbar::updateToolbar()
{
    auto settings = previewSettings();
    m_previewToolbar->toggleButton("RedChannel"_id, settings.showRed);
    m_previewToolbar->toggleButton("GreenChannel"_id, settings.showGreen);
    m_previewToolbar->toggleButton("BlueChannel"_id, settings.showBlue);
    m_previewToolbar->toggleButton("AlphaChannel"_id, settings.showAlpha);
    m_previewToolbar->toggleButton("PointFilter"_id, settings.showAlpha);
    m_previewToolbar->toggleButton("Premultiply"_id, settings.premultiply);
    m_previewToolbar->toggleButton("Checkers"_id, settings.showCheckers);
    m_previewToolbar->toggleButton("PixelGrid"_id, settings.showGrid);
}

void ImagePreviewPanelWithToolbar::updateUIState()
{
    auto settings = previewSettings();

    if (m_imagePreview)
        m_imagePreview->configure(settings);
    
    m_mipmapChoiceBox->visibility(m_numImageMips > 1);
    m_sliceChoicebox->visibility(m_numImageSlices > 1);
    m_exposureControl->visibility(settings.colorSpace == assets::ImageContentColorSpace::HDR);
    m_toneMapMode->visibility(settings.colorSpace == assets::ImageContentColorSpace::HDR);

    if (m_imageSRV)
    {
        m_previewHorizontalRuler->activeRegion(0.0f, m_imageSRV->width());
        m_previewVerticalRuler->activeRegion(0.0f, m_imageSRV->height());
    }

    updateToolbar();
}

void ImagePreviewPanelWithToolbar::updateMipmapList()
{
    m_mipmapChoiceBox->clearOptions();

    if (m_numImageMips > 1)
    {
        auto settings = previewSettings();

        for (uint32_t i = 0; i < m_numImageMips; ++i)
        {
            auto mipWidth = std::max<uint32_t>(1, m_imageSRV->width() >> i);
            auto mipHeight = std::max<uint32_t>(1, m_imageSRV->height() >> i);
            auto mipDepth = std::max<uint32_t>(1, m_imageSRV->depth() >> i);

			const auto viewType = m_imageSRV->image()->type();
            if (viewType == ImageViewType::View1D || viewType == ImageViewType::View1DArray)
                m_mipmapChoiceBox->addOption(TempString("Mip {} ({})", i, mipWidth));
            else if (viewType == ImageViewType::View2D || viewType == ImageViewType::View2DArray)
                m_mipmapChoiceBox->addOption(TempString("Mip {} ({}x{})", i, mipWidth, mipHeight));
            else if (viewType == ImageViewType::ViewCube || viewType == ImageViewType::ViewCubeArray)
                m_mipmapChoiceBox->addOption(TempString("Mip {} ({}x{})", i, mipWidth, mipHeight));
            else if (viewType == ImageViewType::View3D)
                m_mipmapChoiceBox->addOption(TempString("Mip {} ({}x{}x{})", i, mipWidth, mipHeight, mipDepth));
        }

        m_mipmapChoiceBox->selectOption(settings.selectedMip);
        m_mipmapChoiceBox->enable(true);
    }
    else
    {
        m_mipmapChoiceBox->addOption("(no mips)");
        m_mipmapChoiceBox->selectOption(0);
        m_mipmapChoiceBox->enable(false);
    }
}

void ImagePreviewPanelWithToolbar::updateSliceList()
{
    m_sliceChoicebox->clearOptions();

    if (m_numImageSlices)
    {
        auto settings = previewSettings();

        if (m_imageIsCube)
        {
            const char* SideNames[6] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
            if (m_numImageSlices > 6)
            {
                for (uint32_t i = 0; i < m_numImageSlices; ++i)
                {
                    auto index = i / 6;
                    m_sliceChoicebox->addOption(TempString("Cube {} {}", i, SideNames[i % 6]));
                }
            }
            else
            {
                for (uint32_t i = 0; i < m_numImageSlices; ++i)
                    m_sliceChoicebox->addOption(TempString("Slice {}", SideNames[i % 6]));
            }
        }
        else
        {
            for (uint32_t i = 0; i < m_numImageSlices; ++i)
                m_sliceChoicebox->addOption(TempString("Slice {}", i));
        }

		m_sliceChoicebox->selectOption(settings.selectedSlice);
		m_sliceChoicebox->enable(true);
        m_sliceChoicebox->visibility(true);
    }
    else
    {
        m_sliceChoicebox->addOption("(no slices)");
        m_sliceChoicebox->selectOption(0);
        m_sliceChoicebox->visibility(false);
    }
}

void ImagePreviewPanelWithToolbar::recreatePreviewItems()
{
    // detach existing elements
    if (m_imagePreview)
    {
        m_previewPanel->removeElement(m_imagePreview);
        m_imagePreview.reset();
    }

    // get main view
    if (m_imageSRV)
    {
        const auto& settings = previewSettings();

        m_imagePreview = RefNew<ImagePreviewElement>(m_imageSRV, settings.selectedMip, settings.selectedSlice);
        m_imagePreview->configure(settings);
        m_previewPanel->addElement(m_imagePreview, ui::Position(0, 0));
    }
}

//--

END_BOOMER_NAMESPACE_EX(ed)
