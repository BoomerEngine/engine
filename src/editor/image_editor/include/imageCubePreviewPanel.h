/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "engine/ui/include/uiElement.h"
#include "engine/ui/include/uiRenderingPanel.h"
#include "import/texture_loader/include/imageCompression.h"
#include "editor/viewport/include/viewportCameraController.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

enum class ImageCubePreviewMode : uint8_t
{
    Skybox,
    DiffuseCube,
    DiffuseSphere,
    ReflectiveCube,
    ReflectiveSphere,
};

struct ImageCubePreviewPanelSettings
{
    ImageCubePreviewMode mode = ImageCubePreviewMode::Skybox;

    bool pointFilter = false;
    bool showRed = true;
    bool showGreen = true;
    bool showBlue = true;
    bool showAlpha = true;

    int selectedCube = 0;
    int selectedMip = 0;

    int toneMapMode = 0;
    float exposureAdj = 0.0f;

    assets::ImageContentColorSpace colorSpace;
};

//--

// a preview panel for an image
class EDITOR_IMAGE_EDITOR_API ImageCubePreviewPanel : public ui::RenderingPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(ImageCubePreviewPanel, ui::RenderingPanel);

public:
    ImageCubePreviewPanel();
    virtual ~ImageCubePreviewPanel();

    inline const ImageCubePreviewPanelSettings& previewSettings() const { return m_previewSettings; }

    void previewSettings(const ImageCubePreviewPanelSettings& settings);
    void bindImageView(const gpu::ImageSampledView* image);

private:
    ImageCubePreviewPanelSettings m_previewSettings;

    ui::ViewportCameraController m_cameraController;

    gpu::ImageSampledViewPtr m_imageSRV;

    gpu::BufferObjectPtr m_cubeVertices;
    gpu::GraphicsPipelineObjectPtr m_shaderSkybox;
    gpu::GraphicsPipelineObjectPtr m_shaderCube;

    virtual void handleCamera(CameraSetup& outCamera) const override;
    virtual void handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, const CameraSetup& camera, const rendering::FrameParams_Capture* capture) override;

    virtual ui::InputActionPtr handleMouseClick(const ui::ElementArea& area, const input::MouseClickEvent& evt) override;
    virtual bool handleMouseWheel(const input::MouseMovementEvent& evt, float delta) override;
};

//--

// a preview panel for an image/texture with buttons that control it
class EDITOR_IMAGE_EDITOR_API ImageCubePreviewPanelWithToolbar : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ImageCubePreviewPanelWithToolbar, ui::IElement);

public:
    ImageCubePreviewPanelWithToolbar();
    virtual ~ImageCubePreviewPanelWithToolbar();

    //--

    // preview panel
    INLINE const RefPtr<ImageCubePreviewPanel>& panel() const { return m_previewPanel; }

    // toolbar
    INLINE const ui::ToolBarPtr& toolbar() const { return m_previewToolbar; }

    // get/set preview settings
    const ImageCubePreviewPanelSettings& previewSettings() const;
    void previewSettings(const ImageCubePreviewPanelSettings& settings);

    //--

    // bind a generic texture view
    void bindImageView(const gpu::ImageSampledView* image, assets::ImageContentColorSpace knownColorSpace = assets::ImageContentColorSpace::Auto);

    //--

private:
    RefPtr<ImageCubePreviewPanel> m_previewPanel;
    ui::ToolBarPtr m_previewToolbar;

    ui::TrackBarPtr m_colorScaleBar;
    ui::ComboBoxPtr m_previewModeBox;

    ui::ComboBoxPtr m_mipmapChoiceBox;
    ui::ComboBoxPtr m_sliceChoicebox;

    ui::TrackBarPtr m_exposureControl;
    ui::ComboBoxPtr m_toneMapMode;

    ui::ComboBoxPtr m_cubeMode;

    uint32_t m_numImageCubes = 0;
    uint32_t m_numImageMips = 0;
    uint32_t m_cubeSize = 0;

    void updateMipmapList();
    void updateSliceList();
    void updateUIState();
    void updateToolbar();
};

//--

END_BOOMER_NAMESPACE_EX(ed)
