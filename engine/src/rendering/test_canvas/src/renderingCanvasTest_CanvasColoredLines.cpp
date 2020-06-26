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
        /// test of lines of different width
        class SceneTest_CanvasColoredLines : public ICanvasTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasColoredLines, ICanvasTest);

        public:
            virtual void initialize() override
            {}

            virtual void render(base::canvas::Canvas& c) override
            {
                base::canvas::GeometryBuilder builder;

                {
                    //builder.transform(50, 50);
                }

                c.place(builder);
            }
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasColoredLines);
            RTTI_METADATA(CanvasTestOrderMetadata).order(22);
        RTTI_END_TYPE();

        //---      

    } // test
} // rendering