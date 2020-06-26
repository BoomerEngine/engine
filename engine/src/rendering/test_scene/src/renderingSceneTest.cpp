/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "renderingSceneTest.h"

#include "base/image/include/image.h"

namespace rendering
{
    namespace test
    {

        //--

        RTTI_BEGIN_TYPE_CLASS(SceneTestOrderMetadata);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISceneTest);
        RTTI_END_TYPE();

        ISceneTest::ISceneTest()
            : m_hasErrors(false)
        {
            m_defaultImage = base::CreateSharedPtr<base::image::Image>(16, 16, base::Color::WHITE);
        }

        bool ISceneTest::processInitialization()
        {
            initialize();
            return !m_hasErrors;
        }

        void ISceneTest::reportError(base::StringView<char> msg)
        {
            TRACE_ERROR("SceneTest initialization error: {}", msg);
            m_hasErrors = true;
        }

        base::image::ImagePtr ISceneTest::loadImage(base::StringView<char> assetFile)
        {
            auto fullImagePath = base::res::ResourcePath(base::TempString("engine/tests/textures/{}", assetFile));
            auto imagePtr = base::LoadResource<base::image::Image>(fullImagePath);
            if (!imagePtr)
            {
                reportError(base::TempString("Failed to load image '{}'", fullImagePath));
                return m_defaultImage;
            }

            return imagePtr.acquire();
        }

        base::FontPtr ISceneTest::loadFont(base::StringView<char> assetFile)
        {
            auto fullImagePath = base::res::ResourcePath(base::TempString("engine/tests/fonts/{}", assetFile));
            auto imagePtr = base::LoadResource<base::font::Font>(fullImagePath);
            if (!imagePtr)
            {
                reportError(base::TempString("Failed to load font '{}'", fullImagePath));
                return base::CreateSharedPtr<base::font::Font>();
            }

            return imagePtr.acquire();
        }

        
        //--

    } // test
} // rendering