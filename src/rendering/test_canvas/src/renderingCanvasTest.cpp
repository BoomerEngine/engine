/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*


* [#filter: command #]
***/

#include "build.h"
#include "renderingCanvasTest.h"
#include "base/image/include/image.h"

BEGIN_BOOMER_NAMESPACE(rendering::test)

//--

RTTI_BEGIN_TYPE_CLASS(CanvasTestOrderMetadata);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICanvasTest);
RTTI_END_TYPE();

ICanvasTest::ICanvasTest()
    : m_hasErrors(false)
{
    m_defaultImage = base::RefNew<base::image::Image>(16, 16, base::Color::WHITE);
}

bool ICanvasTest::processInitialization()
{
    initialize();
    return !m_hasErrors;
}

void ICanvasTest::reportError(base::StringView msg)
{
    TRACE_ERROR("CanvasTest initialization error: {}", msg);
    m_hasErrors = true;
}

base::image::ImagePtr ICanvasTest::loadImage(base::StringView assetFile)
{
    auto imagePtr = base::LoadImageFromDepotPath(base::TempString("/engine/tests/textures/{}", assetFile));
    if (!imagePtr)
    {
        reportError(base::TempString("Failed to load image '{}'", assetFile));
        return m_defaultImage;
    }

    return imagePtr;
}

base::FontPtr ICanvasTest::loadFont(base::StringView assetFile)
{
    auto imagePtr = base::LoadFontFromDepotPath(base::TempString("/engine/interface/fonts/{}", assetFile));
    if (!imagePtr)
    {
        reportError(base::TempString("Failed to load font '{}'", assetFile));
        return base::RefNew<base::font::Font>();
    }

    return imagePtr;
}

//--

END_BOOMER_NAMESPACE(rendering::test)