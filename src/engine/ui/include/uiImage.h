/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#pragma once

#include "uiElement.h"
#include "uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

/// a simple element dedicated to displaying images
/// NOTE: we do not use any styling for the image itself (only the "color")
class ENGINE_UI_API CustomImage : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(CustomImage, IElement);

public:
    CustomImage();
    CustomImage(canvas::ImageEntry customImageWidget);
    CustomImage(const Image* customImageWidget);
    CustomImage(StringID iconName);

    //--

    // set image to display, sets the "image" sty
    void image(const Image* customImageWidget);
    void image(canvas::ImageEntry ptr);
    void image(StringID iconName);

    // set image scaling, if not specified the styles are used
    void imageScale(float same); // sets image-scale-x. image-scale-x
    void imageScale(float x, float y); // sets image-scale-x, image-scale-y

    //--

protected:
    virtual void computeSize(Size& outSize) const override;
    virtual void prepareShadowGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const override;
    virtual void prepareForegroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, canvas::GeometryBuilder& builder) const override;

	canvas::ImageEntry acquireImageEntry() const;

    mutable Size m_imageSize;        
};

//--

END_BOOMER_NAMESPACE_EX(ui)
