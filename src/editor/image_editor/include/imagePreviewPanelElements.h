/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "engine/ui/include/uiElement.h"
#include "engine/ui/include/uiCanvasArea.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

struct ImagePreviewPanelSettings;
struct ImagePreviewPixel;

//--

// image based preview element
class EDITOR_IMAGE_EDITOR_API IImagePreviewElement : public ui::ICanvasAreaElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(IImagePreviewElement, ui::ICanvasAreaElement);

public:
    virtual ~IImagePreviewElement();

    virtual void configure(const ImagePreviewPanelSettings& settings) {};
    virtual bool queryColor(float x, float y, ImagePreviewPixel& outPixel) const;
};

//--

class EDITOR_IMAGE_EDITOR_API ImagePreviewElement : public IImagePreviewElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(ImagePreviewElement, IImagePreviewElement);

public:
    ImagePreviewElement(const gpu::ImageSampledView* view, int mipIndex, int sliceIndex);
    ~ImagePreviewElement();

    virtual void configure(const ImagePreviewPanelSettings& settings) override;
    virtual void prepareGeometry(ui::CanvasArea* owner, float sx, float sy, ui::Size& outCanvasSizeAtCurrentScale) override;
    virtual void render(ui::CanvasArea* owner, float x, float y, float sx, float sy, Canvas& canvas, float mergedOpacity)  override;

protected:
    gpu::ImageSampledViewPtr m_view;

    int m_mipIndex = 0;
    int m_sliceIndex = 0;

    UniquePtr<ImagePreviewPanelSettings> m_settings; // updated only on configure
};

//--

extern void RenderPixelBackground(Canvas& canvas, const ui::Position& tl, const ui::Position& br, const ui::ElementArea& drawArea, const Rect& activeImageArea, float colorFrac);

//--

END_BOOMER_NAMESPACE_EX(ed)
