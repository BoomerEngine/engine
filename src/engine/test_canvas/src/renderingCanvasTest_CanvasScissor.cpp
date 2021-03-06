/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingCanvasTest.h"

#include "engine/canvas/include/geometryBuilder.h"
#include "engine/canvas/include/geometry.h"
#include "engine/canvas/include/canvas.h"
#include "engine/canvas/include/style.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

/// test of scissor cliping 
class SceneTest_CanvasScissor : public ICanvasTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasScissor, ICanvasTest);

public:
    virtual void initialize() override
    {             
    }

    void buildGeometry(MTRandState & rng, float canvasW, float canvasH, canvas::Geometry& outGeometry)
    {
        canvas::GeometryBuilder b(outGeometry);

        for (int i = 0; i < 200; ++i)
        {
            float w = Lerp(20.0f, 200.0f, rng.unit());
            float h = Lerp(20.0f, 200.0f, rng.unit());
            float x = Lerp(0.0f, canvasW - w, rng.unit());
            float y = Lerp(0.0f, canvasH - h, rng.unit());
            float r = Lerp(0.0f, std::min(w, h) * 0.9f, rng.unit());

            b.beginPath();
            b.roundedRect(x, y, w, h, r);

            const auto startColor = Color((uint8_t)rng.next(), (uint8_t)rng.next(), (uint8_t)rng.next(), 255);
            const auto endColor = Color((uint8_t)rng.next(), (uint8_t)rng.next(), (uint8_t)rng.next(), 255);;

            b.fillPaint(canvas::LinearGradienti(0, 0, w, h, startColor, endColor));
            b.fill();
        }
    }

    virtual void render(canvas::Canvas& c) override
    {
        m_time += 1.0f / 60.0f;

        // cache geometry
        if (c.width() != m_drawCanvasWidth || c.height() != m_drawCanvasHeight)
        {
            m_drawCanvasWidth = c.width();
            m_drawCanvasHeight = c.height();

            MTRandState rng;
            buildGeometry(rng, m_drawCanvasWidth, m_drawCanvasHeight, m_drawGeometry[0]);
            buildGeometry(rng, m_drawCanvasWidth, m_drawCanvasHeight, m_drawGeometry[1]);
            buildGeometry(rng, m_drawCanvasWidth, m_drawCanvasHeight, m_drawGeometry[2]);
        }

        // place geometry
        for (int i=0; i<3; ++i)
        {
            const float phaseOffset = i * 0.2f;
            const float speedScale = 1.0f + i * 0.02f;
            const float sizeMod = 1 << i;

            {
                auto ex = c.width() * 0.5f;
                auto ey = c.height() * 0.5f;
                auto cx = 400.0f / sizeMod;
                auto cy = 400.0f / sizeMod;
                auto offsetX = ex + (ex * 0.9f) * sinf(phaseOffset + m_time * 0.06f * speedScale) - (cx * 0.5f);
                auto offsetY = ey + (ey * 0.9f) * sinf(phaseOffset + m_time * 0.05f * speedScale) - (cy * 0.5f);

                c.scissorRect(offsetX, offsetY, cx, cy);
                c.place(Vector2(0,0), m_drawGeometry[i]);
            }
        }
    }

    float m_time = 0.0f;

    canvas::Geometry m_drawGeometry[3];

    float m_drawCanvasWidth = 0.0f;
    float m_drawCanvasHeight = 0.0f;
};

RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasScissor);
RTTI_METADATA(CanvasTestOrderMetadata).order(40);
RTTI_END_TYPE();

//--

END_BOOMER_NAMESPACE_EX(test)
