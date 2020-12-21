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

namespace ui
{
    //--

    /// a simple element dedicated to displaying images
    /// NOTE: we do not use any styling for the image itself (only the "color")
    class BASE_UI_API Image : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Image, IElement);

    public:
        Image();
        Image(base::canvas::ImageEntry customImage);
        Image(const base::image::ImageRef& customImage);
        Image(base::StringID iconName);

        //--

        // set image to display, sets the "image" style
        void image(base::canvas::ImageEntry ptr);
        void image(base::StringID iconName);

        // set image scaling, if not specified the styles are used
        void imageScale(float same); // sets image-scale-x. image-scale-x
        void imageScale(float x, float y); // sets image-scale-x, image-scale-y

        //--

    protected:
        virtual void computeSize(Size& outSize) const override;
        virtual void prepareShadowGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const override;
        virtual void prepareForegroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const override;

		base::canvas::ImageEntry acquireImageEntry() const;

        mutable Size m_imageSize;        
    };

    //--

} // ui