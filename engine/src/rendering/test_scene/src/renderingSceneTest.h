/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/object/include/rttiMetadata.h"
#include "base/resource/include/resourceLoader.h"

#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasStyle.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"

#include "base/font/include/font.h"
#include "base/font/include/fontInputText.h"
#include "base/font/include/fontGlyphCache.h"
#include "base/font/include/fontGlyphBuffer.h"

#include "rendering/scene/include/renderingFrameParams.h"

namespace rendering
{
    namespace test
    {
        //---

        // order of test
        class SceneTestOrderMetadata : public base::rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTestOrderMetadata, base::rtti::IMetadata);

        public:
            INLINE SceneTestOrderMetadata()
                : m_order(-1)
            {}

            SceneTestOrderMetadata& order(int val)
            {
                m_order = val;
                return *this;
            }

            int m_order;
        };

        //---

        /// a basic rendering test for the scene
        class ISceneTest : public base::NoCopy
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ISceneTest);

        public:
            ISceneTest();

            bool processInitialization();

            virtual void initialize() {};
            virtual void setupFrame(scene::FrameParams& frame) {};
            virtual void configure() {};
            virtual void update(float dt) {};
            virtual bool processInput(const base::input::BaseEvent& evt) { return false; };

            void reportError(base::StringView msg);

            base::image::ImagePtr loadImage(base::StringView imageName);
            base::FontPtr loadFont(base::StringView fontName);

        protected:
            bool m_hasErrors;

            base::image::ImagePtr m_defaultImage;
        };

        ///---

    } // test
} // rendering