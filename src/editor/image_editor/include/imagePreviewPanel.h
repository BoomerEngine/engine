/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/ui/include/uiElement.h"
#include "base/ui/include/uiCanvasArea.h"
#include "import/texture_loader/include/imageCompression.h"

namespace ed
{
    //--

    class ImagePreviewElement;

    //--

    struct ImagePreviewPanelSettings
    {
        bool showCheckers = false;
        bool showGrid = false;
        bool pointFilter = true;
        bool premultiply = false;
        bool showRed = true;
        bool showGreen = true;
        bool showBlue = true;
        bool showAlpha = true;

        bool allSlices = false;
        int selectedSlice = 0;
        int selectedMip = 0;

        int toneMapMode = 0;
        float exposureAdj = 0.0f;

        rendering::ImageContentColorSpace colorSpace;
    };

    //--

    struct ImagePreviewPixel
    {
        base::Vector4 color;
        bool integerValues = false;
        double maxValue = 1.0f;
        uint8_t channels = 0;
        uint32_t x = 0;
        uint32_t y = 0;
    };

    //--

    // a preview panel for an image
    class EDITOR_IMAGE_EDITOR_API ImagePreviewPanel : public ui::CanvasArea
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ImagePreviewPanel, ui::CanvasArea);

    public:
        ImagePreviewPanel();
        virtual ~ImagePreviewPanel();

        inline const ImagePreviewPanelSettings& previewSettings() const { return m_previewSettings; }

        void previewSettings(const ImagePreviewPanelSettings& settings); 

    private:
        ImagePreviewPanelSettings m_previewSettings;

        virtual void renderForeground(ui::DataStash& stash, const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
        virtual void renderBackground(ui::DataStash& stash, const ui::ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity) override;
        virtual bool handleMouseMovement(const base::input::MouseMovementEvent& evt) override;
        virtual void handleHoverLeave(const ui::Position& absolutePosition) override;

        virtual ui::ElementPtr queryTooltipElement(const ui::Position& absolutePosition, ui::ElementArea& outArea) const override;

        void updateTooltip() const;

        mutable base::RefWeakPtr<ui::TextLabel> m_activeTooltipText;
        mutable base::RefWeakPtr<ui::IElement> m_activeTooltipColor;

        ImagePreviewPixel m_previewPixel;
        bool m_previewPixelValid = false;

        int m_hoverPixelX = 0;
        int m_hoverPixelY = 0;
        bool m_hoverPixelValid = false;
    };

    //--

    // a preview panel for an image/texture with buttons that control it
    class EDITOR_IMAGE_EDITOR_API ImagePreviewPanelWithToolbar : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ImagePreviewPanelWithToolbar, ui::IElement);

    public:
        ImagePreviewPanelWithToolbar();
        virtual ~ImagePreviewPanelWithToolbar();

        //--

        // preview panel
        INLINE const base::RefPtr<ImagePreviewPanel>& panel() const { return m_previewPanel; }

        // toolbar
        INLINE const ui::ToolBarPtr& toolbar() const { return m_previewToolbar; }

        // get/set preview settings
        const ImagePreviewPanelSettings& previewSettings() const;
        void previewSettings(const ImagePreviewPanelSettings& settings);

        //--

        // bind a generic texture view
        void bindImageView(const rendering::ImageSampledView* image, rendering::ImageContentColorSpace knownColorSpace = rendering::ImageContentColorSpace::Auto);

        //--

    private:
        base::RefPtr<ImagePreviewPanel> m_previewPanel;
        ui::ToolBarPtr m_previewToolbar;

        ui::HorizontalRulerPtr m_previewHorizontalRuler;
        ui::VerticalRulerPtr m_previewVerticalRuler;

        ui::TrackBarPtr m_colorScaleBar;
        ui::ComboBoxPtr m_previewModeBox;

        ui::ComboBoxPtr m_mipmapChoiceBox;
        ui::ComboBoxPtr m_sliceChoicebox;

        ui::TrackBarPtr m_exposureControl;
        ui::ComboBoxPtr m_toneMapMode;

        uint32_t m_numImageSlices = 0;
        uint32_t m_numImageMips = 0;

		rendering::ImageSampledViewPtr m_mainImageSRV;
		rendering::ImageSampledViewPtr m_sourceImageSRV;

        base::Array<base::RefPtr<ImagePreviewElement>> m_mainImagePreviewElements;

        void updateMipmapList();
        void updateSliceList();
        void updateUIState();
        void recreatePreviewItems();
    };

    //--

} // ed