/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "staticTextureEditor.h"
#include "imagePreviewPanel.h"
#include "imageHistogramWidget.h"
#include "imageHistogramCalculation.h"

#include "editor/common/include/managedFile.h"
#include "editor/common/include/managedFileFormat.h"
#include "engine/ui/include/uiDockLayout.h"
#include "engine/ui/include/uiDockPanel.h"
#include "engine/ui/include/uiDataInspector.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiRuler.h"
#include "engine/ui/include/uiSplitter.h"
#include "core/image/include/image.h"
#include "core/image/include/imageView.h"
#include "core/resource/include/reference.h"
#include "engine/ui/include/uiNotebook.h"

#include "engine/texture/include/staticTexture.h"
#include "engine/texture/include/texture.h"
#include "gpu/device/include/image.h"
#include "editor/common/include/managedFileNativeResource.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(StaticTextureEditor);
RTTI_END_TYPE();

StaticTextureEditor::StaticTextureEditor(ManagedFileNativeResource* file)
    : ResourceEditorNativeFile(file, { ResourceEditorFeatureBit::Save, ResourceEditorFeatureBit::Imported })
    , m_histogramCheckTimer(this, "HistogramCheckTimer"_id)
{
    createInterface();

    m_histogramCheckTimer = [this]() { checkHistograms(); };
    m_histogramCheckTimer.startRepeated(0.1f);
}

StaticTextureEditor::~StaticTextureEditor()
{}

void StaticTextureEditor::createInterface()
{
    {
        auto tab = RefNew<ui::DockPanel>("[img:world] Preview", "PreviewPanel");
        m_previewTab = tab;
        m_previewTab->layoutVertical();

        m_previewPanel = m_previewTab->createChild<ImagePreviewPanelWithToolbar>();
        m_previewPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_previewPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            
        dockLayout().attachPanel(tab);
    }

    {
        auto panel = RefNew<ui::DockPanel>("[img:table] Image", "ImageInfoPanel");
        panel->layoutVertical();

        auto splitter = panel->createChild<ui::Splitter>(ui::Direction::Horizontal, 0.33f);

        {
            auto notebook = splitter->createChild<ui::Notebook>();

            auto colorHistogram = RefNew<ImageHistogramWidget>();
            m_colorHistogram = colorHistogram;
            m_colorHistogram->customStyle<StringBuf>("title"_id, "[img:color_wheel]RGB");
            notebook->attachTab(m_colorHistogram, nullptr, true);

            auto lumHistogram = RefNew<ImageHistogramWidget>();
            m_lumHistogram = lumHistogram;
            m_lumHistogram->customStyle<StringBuf>("title"_id, "[img:lightbulb]Luminance");
            notebook->attachTab(m_lumHistogram, nullptr, false);
        }

        {
            m_imageInfoLabel = splitter->createChild<ui::TextLabel>();
            m_imageInfoLabel->customMargins(5);
        }

        dockLayout().right().attachPanel(panel);
    }
}

void StaticTextureEditor::updateHistogram()
{
    m_pendingHistograms.clear();

    if (auto data = texture())
    {
        if (auto view = data->view())
        {
            const auto numChannels = GetImageFormatInfo(view->image()->format()).numComponents;

            for (int i = 0; i < std::min<int>(numChannels, 3); ++i)
            {
                ImageComputationSettings settings;
                settings.alphaThreshold = 0.5f;
                settings.channel = i;
                settings.mipIndex = 0;
                settings.sliceIndex = 0;

                if (auto pendingHistogram = ComputeHistogram(view, settings))
                    m_pendingHistograms.pushBack(pendingHistogram);
            }
        }
    }
}

void StaticTextureEditor::checkHistograms()
{
    if (!m_pendingHistograms.empty())
    {
        bool allHistogramsReady = true;
        for (const auto& hist : m_pendingHistograms)
            if (!hist->fetchDataIfReady())
                allHistogramsReady = false;

        if (allHistogramsReady)
        {
            if (m_colorHistogram)
                m_colorHistogram->removeHistograms();

            if (m_lumHistogram)
                m_lumHistogram->removeHistograms();

            for (const auto& hist : m_pendingHistograms)
            {
                if (auto data = hist->fetchDataIfReady())
                {
                    if (data->channel == 0 && m_colorHistogram)
                        m_colorHistogram->addHistogram(data, Color::RED, "Red");
                    else if (data->channel == 1 && m_colorHistogram)
                        m_colorHistogram->addHistogram(data, Color::GREEN, "Green");
                    else if (data->channel == 2 && m_colorHistogram)
                        m_colorHistogram->addHistogram(data, Color::BLUE, "Blue");
                    else if (data->channel == 3 && m_lumHistogram)
                        m_lumHistogram->addHistogram(data, Color::WHITE, "Luminance");
                }
            }
        }
    }
}

void StaticTextureEditor::updateImageInfoText()
{
    StringBuilder txt;

    if (auto data = texture())
    {
        const auto info = data->info();

        if (info.compressed)
            txt.append("[color:#AFA][b]COMPRESSED![/b][/color]\n \n");
        else
            txt.append("[color:#FAA][b]UNCOMPRESSED![/b][/color]\n \n");

        txt.appendf("Type: [b]{}[/b]\n", info.type);

        txt.appendf("Size: [b]");
        if (info.type == ImageViewType::View3D)
            txt.appendf("{}x{}x{}", info.width, info.height, info.depth);
        else if (info.type == ImageViewType::View2D || info.type == ImageViewType::View2DArray)
            txt.appendf("{}x{}", info.width, info.height);
        else if (info.type == ImageViewType::ViewCube || info.type == ImageViewType::ViewCubeArray)
            txt.appendf("{}x{}", info.width, info.height);
        else if (info.type == ImageViewType::View1D)
            txt.appendf("{}", info.width);
        txt.append("[/b]\n");

        txt.appendf("Format: [b]{}[/b] [color:#888](BPP: {})[/color]\n", GetImageFormatInfo(info.format).name,
            GetImageFormatInfo(info.format).bitsPerPixel);

        if (info.type == ImageViewType::ViewCubeArray || info.type == ImageViewType::View2DArray)
            txt.appendf("Array slices: [b]{}[/b]\n", info.slices);

        txt.appendf("Color space: [b]{}[/b]\n", info.colorSpace);

        if (info.premultipliedAlpha)
            txt.appendf("Alpha: [b]Premultiplied[/b]\n");
        else if (GetImageFormatInfo(info.format).numComponents == 4)
            txt.appendf("Alpha: [b]Linear[/b]\n");
        else
            txt.appendf("Alpha: [b]None[/b]\n");

        txt.appendf("Mipmaps: [b]{}[/b]\n", info.mips);

        txt.appendf(" \n");
        txt.appendf("\nTotal data size: [b]{}[/b]\n", MemSize(data->persistentData().size()));

        m_imageInfoLabel->text(txt.toString());
    }
}

/*void StaticTextureEditor::updateImageInfo(const image::ImagePtr& image)
{
    StringBuilder txt;

    if (image->depth() > 1)
        txt.appendf("3D Image [{}x{}x{}]", image->width(), image->height(), image->depth());
    else if (image->height() > 1)
        txt.appendf("2D Image [{}x{}]", image->width(), image->height());
    else if (image->width() > 1)
        txt.appendf("1D Image [{}x{}]", image->width(), image->height());
    else if (image->width() > 1)
        txt.appendf("Pixel Image");

    txt.append("\n");
    txt.appendf("Data size: {}\n\n", MemSize(image->view().dataSize()));

    m_uncompressionStatusText = txt.toString();

    updateImageInfoText();
}*/

static assets::ImageContentColorSpace ConvertColorSpace(image::ColorSpace space)
{
    switch (space)
    {
    case image::ColorSpace::SRGB: return assets::ImageContentColorSpace::SRGB;
    case image::ColorSpace::Linear: return assets::ImageContentColorSpace::Linear;
    case image::ColorSpace::Normals: return assets::ImageContentColorSpace::Normals;
    case image::ColorSpace::HDR: return assets::ImageContentColorSpace::HDR;
    }

    return assets::ImageContentColorSpace::Linear;
}

bool StaticTextureEditor::initialize()
{
    if (!TBaseClass::initialize())
        return false;

    m_texture = rtti_cast<StaticTexture>(resource());
    if (!m_texture)
        return false;

    updateHistogram();
    updateImageInfoText();

    if (auto data = texture())
        m_previewPanel->bindImageView(data->view(), ConvertColorSpace(data->info().colorSpace));

    return true;
}

void StaticTextureEditor::handleLocalReimport(const ResourcePtr& ptr)
{
    if (auto newTexture = rtti_cast<StaticTexture>(ptr))
    {
        m_texture = newTexture;

        updateHistogram();
        updateImageInfoText();

        m_previewPanel->bindImageView(newTexture->view(), ConvertColorSpace(newTexture->info().colorSpace));

            
    }     
}

//---

class StaticTextureResourceEditorOpener : public IResourceEditorOpener
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticTextureResourceEditorOpener, IResourceEditorOpener);

public:
    virtual bool canOpen(const ManagedFileFormat& format) const override
    {
        return format.nativeResourceClass() == StaticTexture::GetStaticClass();
    }

    virtual RefPtr<ResourceEditor> createEditor(ManagedFile* file) const override
    {
        if (auto nativeFile = rtti_cast<ManagedFileNativeResource>(file))
            return RefNew<StaticTextureEditor>(nativeFile);

        return nullptr;
    }
};

RTTI_BEGIN_TYPE_CLASS(StaticTextureResourceEditorOpener);
RTTI_END_TYPE();

//---

END_BOOMER_NAMESPACE_EX(ed)
