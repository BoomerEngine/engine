/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*


* [#filter: command #]
***/

#include "build.h"
#include "renderingCanvasTest.h"
#include "core/image/include/image.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

//--

RTTI_BEGIN_TYPE_CLASS(CanvasTestOrderMetadata);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICanvasTest);
RTTI_END_TYPE();

ICanvasTest::ICanvasTest()
    : m_hasErrors(false)
{
    m_defaultImage = RefNew<Image>(16, 16, Color::WHITE);
}

bool ICanvasTest::processInitialization()
{
    initialize();
    return !m_hasErrors;
}

void ICanvasTest::reportError(StringView msg)
{
    TRACE_ERROR("CanvasTest initialization error: {}", msg);
    m_hasErrors = true;
}

CanvasImagePtr ICanvasTest::loadCanvasImage(StringView assetFile)
{
    return RefNew<CanvasImage>(loadImage(assetFile));
}

ImagePtr ICanvasTest::loadImage(StringView assetFile)
{
    auto imagePtr = LoadImageFromDepotPath(TempString("/engine/tests/textures/{}", assetFile));
    if (!imagePtr)
    {
        reportError(TempString("Failed to load image '{}'", assetFile));
        return m_defaultImage;
    }

    return imagePtr;
}

FontPtr ICanvasTest::loadFont(StringView assetFile)
{
    auto imagePtr = LoadFontFromDepotPath(TempString("/engine/interface/fonts/{}", assetFile));
    if (!imagePtr)
    {
        reportError(TempString("Failed to load font '{}'", assetFile));
        return RefNew<Font>();
    }

    return imagePtr;
}

//--

END_BOOMER_NAMESPACE_EX(test)
