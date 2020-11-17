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

namespace rendering
{
    namespace test
    {

        //--

        RTTI_BEGIN_TYPE_CLASS(CanvasTestOrderMetadata);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICanvasTest);
        RTTI_END_TYPE();

        ICanvasTest::ICanvasTest()
            : m_hasErrors(false)
        {
            m_defaultImage = base::CreateSharedPtr<base::image::Image>(16, 16, base::Color::WHITE);
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
            auto imagePtr = base::LoadResource<base::image::Image>(base::TempString("/engine/tests/textures/{}", assetFile));
            if (!imagePtr)
            {
                reportError(base::TempString("Failed to load image '{}'", assetFile));
                return m_defaultImage;
            }

            return imagePtr.acquire();
        }

        base::FontPtr ICanvasTest::loadFont(base::StringView assetFile)
        {
            auto imagePtr = base::LoadResource<base::font::Font>(base::TempString("/engine/tests/fonts/{}", assetFile));
            if (!imagePtr)
            {
                reportError(base::TempString("Failed to load font '{}'", assetFile));
                return base::CreateSharedPtr<base::font::Font>();
            }

            return imagePtr.acquire();
        }

        
        //--

    } // test
} // rendering