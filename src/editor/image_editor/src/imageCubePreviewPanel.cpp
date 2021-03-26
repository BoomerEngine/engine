/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "imageCubePreviewPanel.h"

#include "gpu/device/include/device.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/globalObjects.h"
#include "gpu/device/include/deviceUtils.h"
#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/framebuffer.h"
#include "gpu/device/include/descriptor.h"
#include "gpu/device/include/shader.h"

#include "engine/ui/include/uiElement.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiComboBox.h"
#include "engine/ui/include/uiTrackBar.h"
#include "engine/ui/include/uiWindow.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_CLASS(ImageCubePreviewPanel);
RTTI_END_TYPE();

#pragma pack(push)
#pragma pack(1)
struct GPUCubeVertex
{
    Vector3 pos;
    Vector3 n;
};

struct GPUCubeParams
{
    Matrix worldToScreen;
    Matrix localToWorld;

    Vector4 colorSelector;
    int mip = 0;
    int slice = 0;
    int colorSpace = 0; // 0-texture is in sRGB mode - output directly, 1-texture is linear, take gamma, 2-HDR with no tone mapping, 3-HDR with tonemapping
    float colorSpaceScale = 1.0f;
    int toneMapMode = 0;
};
#pragma pack(pop)

ImageCubePreviewPanel::ImageCubePreviewPanel()
{
    hitTest(true);

    ui::ViewportCameraControllerSettings settings;
    settings.mode = ui::CameraMode::OrbitPerspective;
    settings.position = Vector3(-2, 0, 0);
    m_cameraController.configure(settings);

    {
        GPUCubeVertex vertices[36];
        gpu::GenerateCubeVertices(&vertices[0].pos, sizeof(GPUCubeVertex));
        //gpu::GenerateNormals(&vertices[0].pos, sizeof(GPUCubeVertex), &vertices[0].n, sizeof(GPUCubeVertex), 12);

        auto dev = GetService<DeviceService>()->device();
        m_cubeVertices = gpu::CreateVertexBuffer(sizeof(vertices), vertices, "PreviewCubemapVertexBuffer");
    }

    {
        if (auto shader = gpu::LoadStaticShaderDeviceObject("editor/image_cube_preview.fx"))
            m_shaderSkybox = shader->createGraphicsPipeline();
    }
}

ImageCubePreviewPanel::~ImageCubePreviewPanel()
{}

void ImageCubePreviewPanel::bindImageView(const gpu::ImageSampledView* image)
{
    m_imageSRV = AddRef(image);
}

void ImageCubePreviewPanel::previewSettings(const ImageCubePreviewPanelSettings& settings)
{
    m_previewSettings = settings;
}

void ImageCubePreviewPanel::handleCamera(CameraSetup& outCamera) const
{
    m_cameraController.settings().computeRenderingCamera(cachedDrawArea().size(), outCamera);
    outCamera.farPlane = 1500.0f;
}

void ImageCubePreviewPanel::handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, const CameraSetup& cameraSetup, const FrameParams_Capture* capture)
{
    const Camera camera(cameraSetup);

    gpu::FrameBuffer fb;
    fb.color[0].view(output.color).clear(Vector4(0.1f, 0.1f, 0.1f, 1.0f));
    fb.depth.view(output.depth).clearDepth(1.0f).clearStencil(0);

    const Rect drawArea(0, 0, output.width, output.height);
    cmd.opBeingPass(fb, 1, drawArea);

    if (m_imageSRV)
    {
        GPUCubeParams params;
        params.worldToScreen = camera.worldToScreen().transposed();

        if (m_previewSettings.mode == ed::ImageCubePreviewMode::Skybox)
            params.localToWorld = Matrix::BuildScale(1000.0f);
        else
            params.localToWorld = Matrix::IDENTITY().transposed();

        params.colorSelector.x = m_previewSettings.showRed ? 1.0f : 0.0f;
        params.colorSelector.y = m_previewSettings.showGreen ? 1.0f : 0.0f;
        params.colorSelector.z = m_previewSettings.showBlue ? 1.0f : 0.0f;
        params.colorSelector.w = m_previewSettings.showAlpha ? 1.0f : 0.0f;
        params.mip = m_previewSettings.selectedMip;
        params.slice = m_previewSettings.selectedCube;

        if (m_previewSettings.colorSpace == assets::ImageContentColorSpace::SRGB)
        {
            params.colorSpace = 1;
            params.colorSpaceScale = 1.0f;
        }
        else if (m_previewSettings.colorSpace == assets::ImageContentColorSpace::Linear)
        {
            params.colorSpace = 0;
            params.colorSpaceScale = 1.0f;
        }
        else if (m_previewSettings.colorSpace == assets::ImageContentColorSpace::HDR)
        {
            params.colorSpace = 2;
            params.toneMapMode = m_previewSettings.toneMapMode;
            params.colorSpaceScale = m_previewSettings.exposureAdj;
        }

        gpu::DescriptorEntry desc[3];
        desc[0].constants(params);
        desc[1] = m_previewSettings.pointFilter ? gpu::Globals().SamplerClampPoint : gpu::Globals().SamplerClampBiLinear;
        desc[2] = m_imageSRV;
        cmd.opBindDescriptor("CubePreviewParams"_id, desc);

        cmd.opBindVertexBuffer("CubePreviewVertex"_id, m_cubeVertices);
        cmd.opDraw(m_shaderSkybox, 0, 36);
    }

    cmd.opEndPass();
}

bool ImageCubePreviewPanel::handleMouseWheel(const InputMouseMovementEvent& evt, float delta)
{
    m_cameraController.processMouseWheel(evt, delta);
    return true;
}

ui::InputActionPtr ImageCubePreviewPanel::handleMouseClick(const ui::ElementArea& area, const InputMouseClickEvent& evt)
{
    if (evt.leftClicked())
        return m_cameraController.handleOrbitAroundPoint(this, 0, true);
    if (evt.rightClicked())
        return m_cameraController.handleOrbitAroundPoint(this, 1, true);
    return TBaseClass::handleMouseClick(area, evt);
}

//--

RTTI_BEGIN_TYPE_CLASS(ImageCubePreviewPanelWithToolbar);
RTTI_END_TYPE();

ImageCubePreviewPanelWithToolbar::ImageCubePreviewPanelWithToolbar()
{
    layoutVertical();

    // panel
    {
        m_previewToolbar = createChild<ui::ToolBar>();
        m_previewToolbar->name("PreviewToolbar"_id);
        m_previewToolbar->customMargins(0.0f, 0.0f, 0.0f, 5.0f);

        m_previewPanel = createChild<ImageCubePreviewPanel>();
        m_previewPanel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        m_previewPanel->customVerticalAligment(ui::ElementVerticalLayout::Expand);
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
        
        //--
    }

    // render mode
    {
        m_previewToolbar->createSeparator();

        m_cubeMode = m_previewToolbar->createChildWithType<ui::ComboBox>("ToolbarComboBox"_id);
        m_cubeMode->customMinSize(100, 10);
        m_cubeMode->addOption("Skybox");
        m_cubeMode->addOption("DiffuseCube");
        m_cubeMode->addOption("DiffuseSphere");
        m_cubeMode->addOption("ReflectiveCube");
        m_cubeMode->addOption("ReflectiveSphere");
        m_cubeMode->selectOption(0);

        m_cubeMode->bind(ui::EVENT_COMBO_SELECTED) = [this](int option)
        {
            auto settings = previewSettings();
            settings.mode = (ImageCubePreviewMode)option;
            previewSettings(settings);
        };
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
            settings.selectedCube = option;
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

ImageCubePreviewPanelWithToolbar::~ImageCubePreviewPanelWithToolbar()
{}

const ImageCubePreviewPanelSettings& ImageCubePreviewPanelWithToolbar::previewSettings() const
{
    return m_previewPanel->previewSettings();
}

void ImageCubePreviewPanelWithToolbar::previewSettings(const ImageCubePreviewPanelSettings& settings)
{
    m_previewPanel->previewSettings(settings);
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

void ImageCubePreviewPanelWithToolbar::bindImageView(const gpu::ImageSampledView* view, assets::ImageContentColorSpace knownColorSpace)
{
    if (view && knownColorSpace == assets::ImageContentColorSpace::Auto)
    {
        const auto formatClass = GetImageFormatInfo(view->image()->format()).formatClass;
        if (formatClass == ImageFormatClass::SRGB)
            knownColorSpace = assets::ImageContentColorSpace::SRGB;
        else if (formatClass == ImageFormatClass::FLOAT)
            knownColorSpace = assets::ImageContentColorSpace::HDR;
    }

    auto settings = previewSettings();

    if (view)
    {
        m_numImageMips = view->mips();
        m_numImageCubes = view->slices() / 6;
        m_cubeSize = view->width();

        if (m_numImageMips <= 1)
            settings.selectedMip = 0;
        else if (settings.selectedMip >= m_numImageMips)
            settings.selectedMip = m_numImageMips - 1;

        if (m_numImageCubes <= 1)
            settings.selectedCube = 0;
        else if (settings.selectedCube >= m_numImageCubes)
            settings.selectedCube = m_numImageCubes - 1;

        settings.colorSpace = knownColorSpace;

        m_previewPanel->previewSettings(settings);

        updateMipmapList();
        updateSliceList();
        updateUIState();
    }
    else
    {
        m_numImageMips = 0;
        m_numImageCubes = 0;
        m_cubeSize = 0;
    }

    m_previewPanel->bindImageView(view);
}

void ImageCubePreviewPanelWithToolbar::updateToolbar()
{
    auto settings = previewSettings();
    m_previewToolbar->toggleButton("RedChannel"_id, settings.showRed);
    m_previewToolbar->toggleButton("GreenChannel"_id, settings.showGreen);
    m_previewToolbar->toggleButton("BlueChannel"_id, settings.showBlue);
    m_previewToolbar->toggleButton("AlphaChannel"_id, settings.showAlpha);
    m_previewToolbar->toggleButton("PointFilter"_id, settings.showAlpha);
}

void ImageCubePreviewPanelWithToolbar::updateUIState()
{
    auto settings = previewSettings();

    m_mipmapChoiceBox->visibility(m_numImageMips > 1);
    m_sliceChoicebox->visibility(m_numImageCubes > 1);
    m_exposureControl->visibility(settings.colorSpace == assets::ImageContentColorSpace::HDR);
    m_toneMapMode->visibility(settings.colorSpace == assets::ImageContentColorSpace::HDR);

    updateToolbar();
}

void ImageCubePreviewPanelWithToolbar::updateMipmapList()
{
    m_mipmapChoiceBox->clearOptions();

    if (m_numImageMips > 1)
    {
        auto settings = previewSettings();

        for (uint32_t i = 0; i < m_numImageMips; ++i)
        {
            auto size = std::max<uint32_t>(1, m_cubeSize >> i);
            m_mipmapChoiceBox->addOption(TempString("Mip {} ({}x{})", i, size, size));
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

void ImageCubePreviewPanelWithToolbar::updateSliceList()
{
    m_sliceChoicebox->clearOptions();

    if (m_numImageCubes > 1)
    {
        auto settings = previewSettings();

        for (uint32_t i = 0; i < m_numImageCubes; ++i)
            m_sliceChoicebox->addOption(TempString("Cube {}", i));

		m_sliceChoicebox->selectOption(settings.selectedCube);
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

//--

END_BOOMER_NAMESPACE_EX(ed)
