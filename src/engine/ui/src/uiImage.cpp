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

#include "core/image/include/image.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//--

RTTI_BEGIN_TYPE_CLASS(CustomImage);
    RTTI_METADATA(ElementClassNameMetadata).name("CustomImage");
RTTI_END_TYPE();

CustomImage::CustomImage()
{}

CustomImage::CustomImage(const CanvasImage* entry)
{
    image(entry);
}

CustomImage::CustomImage(const Image* entry)
{
    image(entry);
}

CustomImage::CustomImage(StringID iconName)
{
    image(iconName);
}

void CustomImage::image(const CanvasImage* customImage)
{
    if (customImage)
    {
        style::ImageReference imageStyle;
        imageStyle.canvasImage = AddRef(customImage);
        customStyle("image"_id, imageStyle);
    }
    else
    {
        removeCustomStyle("image"_id);
    }
}

void CustomImage::image(const Image* customImage)
{
    if (customImage)
    {
        style::ImageReference imageStyle;
        imageStyle.rawImage = AddRef(customImage);
        customStyle("image"_id, imageStyle);
    }
    else
    {
        removeCustomStyle("image"_id);
    }
}

void CustomImage::image(StringID iconName)
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

void CustomImage::imageScale(float same)
{
    imageScale(same, same);
}

void CustomImage::imageScale(float x, float y)
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

CanvasImagePtr CustomImage::acquireImageEntry() const
{
    if (const auto* imageStylePtr = evalStyleValueIfPresentPtr<style::ImageReference>("image"_id))
    {
        if (renderer() && !imageStylePtr->canvasImage)
        {
            if (imageStylePtr->rawImage)
            {
                auto entry = renderer()->stash().cacheImage(imageStylePtr->rawImage);
                imageStylePtr->canvasImage = entry;
            }
            else if (imageStylePtr->name)
            {
                auto entry = renderer()->stash().loadImage(imageStylePtr->name);
                imageStylePtr->canvasImage = entry;
            }
        }

        return imageStylePtr->canvasImage;
    }

    return nullptr;
}

void CustomImage::computeSize(Size& outSize) const
{
    TBaseClass::computeSize(outSize);

	if (const auto imageEntry = acquireImageEntry())
    {
        float width = imageEntry->width() * evalStyleValue<float>("image-scale-x"_id, 1.0f);
        float height = imageEntry->height() * evalStyleValue<float>("image-scale-y"_id, 1.0f);

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

void CustomImage::prepareShadowGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, CanvasGeometryBuilder& builder) const
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
                builder.builder().compositeOperation(CompositeOperation::DestinationOut);
                builder.builder().fill();
                builder.builder().popState();
            }
        }
    }*/
}

void CustomImage::prepareForegroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, CanvasGeometryBuilder& builder) const
{
    TBaseClass::prepareForegroundGeometry(stash, drawArea, pixelScale, builder);

	if (const auto imageEntry = acquireImageEntry())
    {
        ImagePatternSettings imageSettings(imageEntry);
        imageSettings.m_scaleX = (float)imageEntry->width() / std::max<float>(1.0f, m_imageSize.x);
        imageSettings.m_scaleY = (float)imageEntry->height() / std::max<float>(1.0f, m_imageSize.y);

		auto style = CanvasStyle_ImagePattern(imageSettings);

        if (const auto* colorStylePtr = evalStyleValueIfPresentPtr<Color>("color"_id))
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

END_BOOMER_NAMESPACE_EX(ui)
