/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingCanvasTest.h"

#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvas.h"
#include "base/canvas/include/canvasStyle.h"

namespace rendering
{
    namespace test
    {
        /// test of basic texture rendering
        class SceneTest_CanvasTexture : public ICanvasTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasTexture, ICanvasTest);

        public:
            virtual void initialize() override
            {
                m_lena = loadImage("lena.png");
                m_time.resetToNow();
            }

            virtual void render(base::canvas::Canvas& c) override
            {
                base::canvas::GeometryBuilder b;

                CanvasGridBuilder grid(3, 3, 30, 1024, 1024);

                float time = m_time.timeTillNow().toSeconds();

                // simple image
                {
                    auto r = grid.cell();

                    b.resetTransform();
                    b.translatei(r.left(), r.top());

                    b.compositeOperation(base::canvas::CompositeOperation::Copy);
                    b.beginPath();
                    b.roundedRecti(0, 0, r.width(), r.height(), 20);
                    b.fillPaint(base::canvas::ImagePattern(m_lena, base::canvas::ImagePatternSettings()));
                    b.fill();
                }

                // scaled image pattern
                {
                    auto r = grid.cell();

                    b.resetTransform();
                    b.translatei(r.left(), r.top());

                    b.compositeOperation(base::canvas::CompositeOperation::Copy);
                    b.beginPath();
                    b.roundedRecti(0, 0, r.width(), r.height(), 20);
                    b.fillPaint(base::canvas::ImagePattern(m_lena, base::canvas::ImagePatternSettings().scale(2.0f)));
                    b.fill();                    
                }

                // scaled image pattern with offset
                {
                    auto r = grid.cell();

                    b.resetTransform();
                    b.translatei(r.left(), r.top());

                    b.compositeOperation(base::canvas::CompositeOperation::Copy);
                    b.beginPath();
                    b.roundedRecti(0, 0, r.width(), r.height(), 20);
                    b.fillPaint(base::canvas::ImagePattern(m_lena, base::canvas::ImagePatternSettings().scale(2.0f).offset(20.f, 20.0f)));
                    b.fill();                    
                }

                // scaled image rotated with no pivot
                {
                    auto r = grid.cell();

                    b.resetTransform();
                    b.translatei(r.left(), r.top());

                    b.compositeOperation(base::canvas::CompositeOperation::Copy);
                    b.beginPath();
                    b.roundedRecti(0, 0, r.width(), r.height(), 20);
                    b.fillPaint(base::canvas::ImagePattern(m_lena, base::canvas::ImagePatternSettings().scale(2.0f).angle(1.0f * time)));
                    b.fill();
                }

                // scaled image rotated around center
                {
                    auto r = grid.cell();

                    b.resetTransform();
                    b.translatei(r.left(), r.top());

                    float cx = m_lena->width() / 2;
                    float cy = m_lena->height() / 2;

                    b.compositeOperation(base::canvas::CompositeOperation::Copy);
                    b.beginPath();
                    b.roundedRecti(0, 0, r.width(), r.height(), 20);
                    b.fillPaint(base::canvas::ImagePattern(m_lena, base::canvas::ImagePatternSettings().scale(2.0f).pivot(cx,cy).angle(1.0f * time)));
                    b.fill();
                }

                // scaled image rotated around center
                {
                    auto r = grid.cell();

                    b.resetTransform();
                    b.translatei(r.left(), r.top());

                    float cx = m_lena->width();
                    float cy = m_lena->height();

                    float ox = (m_lena->width() / 1.5f) - r.width();
                    float oy = (m_lena->height() / 1.5f) - r.height();

                    b.compositeOperation(base::canvas::CompositeOperation::Copy);
                    b.beginPath();
                    b.roundedRecti(0, 0, r.width(), r.height(), 20);
                    b.fillPaint(base::canvas::ImagePattern(m_lena, base::canvas::ImagePatternSettings().scale(1.5f).pivot(cx, cy).offset(-ox,-oy).angle(1.0f * time)));
                    b.fill();
                }

                // scaled image rotated with no pivot
                {
                    auto r = grid.cell();

                    b.resetTransform();
                    b.translatei(r.left(), r.top());

                    b.compositeOperation(base::canvas::CompositeOperation::Copy);
                    b.beginPath();
                    b.roundedRecti(0, 0, r.width(), r.height(), 20);
                    b.fillPaint(base::canvas::ImagePattern(m_lena, base::canvas::ImagePatternSettings().scale(3.0f).angle(-1.0f * time).wrap()));
                    b.fill();
                }

                // scaled image rotated around center
                {
                    auto r = grid.cell();

                    b.resetTransform();
                    b.translatei(r.left(), r.top());

                    float cx = m_lena->width() / 2;
                    float cy = m_lena->height() / 2;

                    b.compositeOperation(base::canvas::CompositeOperation::Copy);
                    b.beginPath();
                    b.roundedRecti(0, 0, r.width(), r.height(), 20);
                    b.fillPaint(base::canvas::ImagePattern(m_lena, base::canvas::ImagePatternSettings().scale(2.0f).pivot(cx, cy).angle(1.0f * time).wrap()));
                    b.fill();
                }

                // scaled image rotated around center
                {
                    auto r = grid.cell();

                    b.resetTransform();
                    b.translatei(r.left(), r.top());

                    float cx = m_lena->width();
                    float cy = m_lena->height();

                    float ox = (m_lena->width() / 1.5f) - r.width();
                    float oy = (m_lena->height() / 1.5f) - r.height();

                    b.compositeOperation(base::canvas::CompositeOperation::Copy);
                    b.beginPath();
                    b.roundedRecti(0, 0, r.width(), r.height(), 20);
                    b.fillPaint(base::canvas::ImagePattern(m_lena, base::canvas::ImagePatternSettings().scale(1.5f).pivot(cx, cy).offset(-ox, -oy).angle(1.0f * time).wrap()));
                    b.fill();
                }


                c.placement(0.0f, 0.0f, c.width() / 1024.0f, c.height() / 1024.0f);
                c.place(b);
            }

        private:
            base::image::ImagePtr m_lena;
            base::NativeTimePoint m_time;
            //base::image::ImagePtr m_lena;
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasTexture);
        RTTI_METADATA(CanvasTestOrderMetadata).order(100);
        RTTI_END_TYPE();

        //--

    } // test
} // rendering