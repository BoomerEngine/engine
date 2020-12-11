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

namespace rendering
{
    namespace test
    {
        //---

        // order of test
        class CanvasTestOrderMetadata : public base::rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CanvasTestOrderMetadata, base::rtti::IMetadata);

        public:
            INLINE CanvasTestOrderMetadata()
                : m_order(-1)
            {}

            CanvasTestOrderMetadata& order(int val)
            {
                m_order = val;
                return *this;
            }

            int m_order;
        };

        //---

        /// a basic rendering test for the scene
        class ICanvasTest
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ICanvasTest);

        public:
            ICanvasTest();

			base::canvas::IStorage* m_storage = nullptr;

            bool processInitialization();

            virtual void initialize() {};
			virtual void shutdown() {};
            virtual void render(base::canvas::Canvas& canvas) {};
            virtual void update(float dt) {};
            virtual void processInput(const base::input::BaseEvent& evt) {};

            void reportError(base::StringView msg);

            base::image::ImagePtr loadImage(base::StringView imageName);
            base::FontPtr loadFont(base::StringView fontName);

        protected:
            bool m_hasErrors;

            base::image::ImagePtr m_defaultImage;
        };

        ///---

        /// helper class that build as grid in the 2D space
        class CanvasGridBuilder
        {
        public:
            CanvasGridBuilder(uint32_t sizeX, uint32_t sizeY, uint32_t margin, uint32_t width, uint32_t height)
                : m_x(0)
                , m_y(0)
                , m_maxX(sizeX)
                , m_maxY(sizeY)
                , m_margin(margin)
                , m_width(std::min(width, height))
                , m_height(std::min(width, height))
            {}

            base::Rect cell()
            {
                base::Rect r;
                r.min.x = ((m_x * m_width) / m_maxX) + m_margin;
                r.min.y = ((m_y * m_height) / m_maxY) + m_margin;
                r.max.x = (((m_x + 1) * m_width) / m_maxX) - m_margin;
                r.max.y = (((m_y + 1) * m_height) / m_maxY) - m_margin;

                m_x += 1;
                if (m_x == m_maxX)
                {
                    m_x = 0;
                    m_y += 1;
                    if (m_y == m_maxY)
                        m_y = 0;
                }

                return r;
            }

        private:
            uint32_t m_x;
            uint32_t m_y;
            uint32_t m_maxX;
            uint32_t m_maxY;
            uint32_t m_margin;
            uint32_t m_width;
            uint32_t m_height;
        };

        ///---

    } // test
} // rendering