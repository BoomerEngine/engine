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
#include "rendering/driver/include/renderingImageView.h"

namespace ed
{
    //--

    struct ImagePreviewPanelSettings;
    struct ImagePreviewPixel;

    //--

    // image based preview element
    class ASSETS_IMAGE_EDITOR_API IImagePreviewElement : public ui::ICanvasAreaElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IImagePreviewElement, ui::ICanvasAreaElement);

    public:
        virtual ~IImagePreviewElement();

        virtual void configure(const ImagePreviewPanelSettings& settings) {};
        virtual bool queryColor(float x, float y, ImagePreviewPixel& outPixel) const;
    };

    //--

    class ASSETS_IMAGE_EDITOR_API ImagePreviewElement : public IImagePreviewElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ImagePreviewElement, IImagePreviewElement);

    public:
        ImagePreviewElement(rendering::ImageView view, rendering::ImageView sourceView);
        ImagePreviewElement(rendering::ImageView view, int mipIndex=0, int sliceIndex=0);
        ~ImagePreviewElement();

        virtual void configure(const ImagePreviewPanelSettings& settings) override;
        virtual void prepareGeometry(ui::CanvasArea* owner, float sx, float sy, ui::Size& outCanvasSizeAtCurrentScale) override;
        virtual void render(ui::CanvasArea* owner, float x, float y, float sx, float sy, base::canvas::Canvas& canvas, float mergedOpacity)  override;

        void mip(int mip);

    protected:
        rendering::ImageView m_view;
        rendering::ImageView m_sourceView;

        int m_mipIndex = 0;
        int m_sliceIndex = 0;

        base::UniquePtr<ImagePreviewPanelSettings> m_settings; // updated only on configure
    };

    //--

    extern void RenderPixelBackground(base::canvas::Canvas& canvas, const ui::Position& tl, const ui::Position& br, const ui::ElementArea& drawArea, const base::Rect& activeImageArea, float colorFrac);

    //--

} // ed