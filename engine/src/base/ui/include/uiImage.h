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
        Image(const base::image::ImageRef& customImage);
        Image(base::StringID iconName);

        //--

        // set image to display, sets the "image" style
        void image(const base::image::ImageRef& ptr);
        void image(base::StringID iconName);

        // set image scaling, if not specified the styles are used
        void imageScale(float same); // sets image-scale-x. image-scale-x
        void imageScale(float x, float y); // sets image-scale-x, image-scale-y

        //--

    protected:
        virtual void computeSize(Size& outSize) const override;
        virtual void prepareShadowGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const override;
        virtual void prepareForegroundGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const override;
        virtual bool handleTemplateProperty(base::StringView<char> name, base::StringView<char> value) override;

        mutable Size m_imageSize;        
    };

    //--

} // ui