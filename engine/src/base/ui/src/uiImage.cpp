/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\simple #]
***/

#include "build.h"
#include "uiImage.h"
#include "uiGeometryBuilder.h"
#include "uiStyleValue.h"
#include "uiRenderer.h"
#include "uiDataStash.h"

#include "base/image/include/image.h"

namespace ui
{
    //--

    RTTI_BEGIN_TYPE_CLASS(Image);
        RTTI_METADATA(ElementClassNameMetadata).name("Image");
    RTTI_END_TYPE();

    Image::Image()
    {}

    Image::Image(base::canvas::ImageEntry entry)
    {
        image(entry);
    }

    Image::Image(base::StringID iconName)
    {
        image(iconName);
    }

    Image::Image(const base::image::ImageRef& customImage)
    {
        if (customImage)
        {
            style::ImageReference imageStyle;
            imageStyle.image = customImage;
            customStyle("image"_id, imageStyle);
        }
        else
        {
            removeCustomStyle("image"_id);
        }
    }

    void Image::image(base::canvas::ImageEntry customImage)
    {
        if (customImage)
        {
            style::ImageReference imageStyle;
            imageStyle.canvasImage = customImage;
            customStyle("image"_id, imageStyle);
        }
		else
		{
			removeCustomStyle("image"_id);
		}
    }

    void Image::image(base::StringID iconName)
    {
        if (iconName)
        {
            style::ImageReference imageStyle;
            imageStyle.name = iconName;
            customStyle("image"_id, imageStyle);
        }
		else
		{
			removeCustomStyle("image"_id);
		}
    }

    void Image::imageScale(float same)
    {
        imageScale(same, same);
    }

    void Image::imageScale(float x, float y)
    {
        if (x == 1.0f)
            removeCustomStyle("image-scale-x"_id);
        else
            customStyle("image-scale-x"_id, x);

        if (y == 1.0f)
            removeCustomStyle("image-scale-y"_id);
        else
            customStyle("image-scale-y"_id, y);
    }

	base::canvas::ImageEntry Image::acquireImageEntry() const
	{
		if (const auto* imageStylePtr = evalStyleValueIfPresentPtr<style::ImageReference>("image"_id))
		{
			if (!imageStylePtr->canvasImage)
			{
				if (auto loadedImage = imageStylePtr->image.acquire())
				{
					if (renderer())
					{
						auto entry = renderer()->stash().cacheImage(loadedImage);
						imageStylePtr->canvasImage = entry;
					}
				}
			}

			return imageStylePtr->canvasImage;
		}
		
		return base::canvas::ImageEntry();
	}

    void Image::computeSize(Size& outSize) const
    {
        TBaseClass::computeSize(outSize);

		if (const auto imageEntry = acquireImageEntry())
        {
            float width = imageEntry.width * evalStyleValue<float>("image-scale-x"_id, 1.0f);
            float height = imageEntry.height * evalStyleValue<float>("image-scale-y"_id, 1.0f);

            if (const auto* maxWidthPtr = evalStyleValueIfPresentPtr<float>("max-width"_id))
            {
                if (*maxWidthPtr > 0.0f && width > *maxWidthPtr)
                {
                    height *= (*maxWidthPtr / width);
                    width = *maxWidthPtr;
                }
            }

            if (const auto* maxHeightPtr = evalStyleValueIfPresentPtr<float>("max-height"_id))
            {
                if (*maxHeightPtr > 0.0f && height > * maxHeightPtr)
                {
                    width *= (*maxHeightPtr / height);
                    height = *maxHeightPtr;
                }
            }

            width *= cachedStyleParams().pixelScale;
            height *= cachedStyleParams().pixelScale;

            m_imageSize = Size(width, height);

            outSize.x = std::max<float>(outSize.x, width);
            outSize.y = std::max<float>(outSize.y, height);
        }
    }

    void Image::prepareShadowGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const
    {
        TBaseClass::prepareShadowGeometry(stash, drawArea, pixelScale, builder);

        // TODO: proper "drop shadow"
        /*if (const auto* imageStylePtr = evalStyleValueIfPresentPtr<style::RenderStyle>("image"_id))
        {
            const auto *shadowPos = cachedStyleParams().m_params->typedPtr(params::GShadowPosX);
            if (shadowPos != nullptr)
            {
                auto imageArea = computeInternalPlacement(drawArea, builder);
                auto imageStyle = imageStylePtr->evaluateStyle(builder.scale(), imageArea);

                auto shadowRx = cachedStyleParams().m_params->typed(params::GShadowPosX).value().eval() * builder.scale();
                auto shadowRy = cachedStyleParams().m_params->typed(params::GShadowPosY).value().eval() * builder.scale();
                auto shadowAlpha = cachedStyleParams().m_params->typed(params::GShadowAlpha).value().eval();

                {
                    builder.builder().pushState();
                    builder.builder().translate(shadowRx, shadowRy);
                    builder.builder().beginPath();
                    builder.builder().globalAlpha(shadowAlpha);
                    prepareBoundaryGeometry(imageArea, builder, 0.0f);
                    builder.builder().fillPaint(imageStyle);
                    builder.builder().compositeOperation(base::canvas::CompositeOperation::DestinationOut);
                    builder.builder().fill();
                    builder.builder().popState();
                }
            }
        }*/
    }

    void Image::prepareForegroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const
    {
        TBaseClass::prepareForegroundGeometry(stash, drawArea, pixelScale, builder);

		if (const auto imageEntry = acquireImageEntry())
        {
            base::canvas::ImagePatternSettings imageSettings(imageEntry);
            imageSettings.m_scaleX = (float)imageEntry.width / std::max<float>(1.0f, m_imageSize.x);
            imageSettings.m_scaleY = (float)imageEntry.height / std::max<float>(1.0f, m_imageSize.y);

			auto style = base::canvas::ImagePattern(imageSettings);

            if (const auto* colorStylePtr = evalStyleValueIfPresentPtr<base::Color>("color"_id))
                style.innerColor = style.outerColor = *colorStylePtr;

            float ox = std::max<float>(0.0f, drawArea.size().x - m_imageSize.x) * 0.5f;
            float oy = std::max<float>(0.0f, drawArea.size().y - m_imageSize.y) * 0.5f;

            {
                builder.pushTransform();
                builder.translate(ox, oy);
                builder.beginPath();
                builder.rect(0, 0, m_imageSize.x, m_imageSize.y);
                builder.fillPaint(style);
                builder.fill();
				builder.popTransform();
            }
        }
    }    

    //--

} // ui
