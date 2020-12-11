/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "build.h"
#include "renderingCanvasTest.h"

namespace rendering
{
    namespace test
    {
        /// test of font rendering gradients
        class SceneTest_CanvasText : public ICanvasTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_CanvasText, ICanvasTest);

        public:
            virtual void initialize() override
            {
                m_font = loadFont("aileron_regular.otf");
                m_font2 = loadFont("aileron_ultra_light.otf");
                m_font3 = loadFont("aileron_heavy.otf");
                m_font4 = loadFont("aileron_light_italic.otf");
            }

            base::Color color = base::Color::WHITE;
            int blur = 0;

            void print(base::canvas::Canvas& c, float x, float y, float scale, const base::FontPtr& fontPtr, int size, bool bold, bool italic, const base::StringBuf& text)
            {
                base::font::FontStyleParams params;
                params.italic = italic;
                params.bold = bold;
                params.size = size;
                params.blur = blur;

                base::font::GlyphBuffer glyphs;
                base::font::FontAssemblyParams assemblyParams;
                fontPtr->renderText(params, assemblyParams, base::font::FontInputText(text.c_str()), glyphs);

				base::canvas::Geometry g;
				{
					base::canvas::GeometryBuilder b(m_storage, g);
					b.fillColor(color);
					b.print(glyphs);
				}

                c.place(base::XForm2D(x,y), g);
            }

            virtual void render(base::canvas::Canvas& c) override
            {
                float x = 10.0f;
                float y = 10.0f;

                color = base::Color::RED;

                print(c, x, y, 1.0f, m_font, 12, false, false, "Hello World!");
                y += 12.0f;

                color = base::Color::WHITE;

                print(c, x, y, 1.0f, m_font, 40, false, false, "The quick brown fox jumps over the lazy dog!");
                y += 40.0f;

                print(c, x, y, 1.0f, m_font2, 40, false, false, "The quick brown fox jumps over the lazy dog!");
                y += 40.0f;

                print(c, x, y, 1.0f, m_font4, 10, false, false, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum!");
                y += 30.0f;

                print(c, x, y, 1.0f, m_font4, 12, false, false, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum!");
                y += 30.0f;

                print(c, x, y, 1.0f, m_font4, 16, false, false, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum!");
                y += 30.0f;

                print(c, x, y, 1.0f, m_font4, 20, false, false, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum!");
                y += 30.0f;

                print(c, x, y, 1.0f, m_font4, 40, false, false, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum!");
                y += 50.0f;

                print(c, x, y, 3.0f, m_font4, 12, false, false, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum!");
                y += 50.0f;

                print(c, x, y, 0.5f, m_font4, 50, false, false, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum!");
                y += 50.0f;

                print(c, x, y, 0.25f, m_font4, 50, false, false, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum!");
                y += 50.0f;

                print(c, x, y, 0.2f, m_font4, 50, false, false, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum!");
                y += 12.0f;

                print(c, x, y, 1.0f, m_font4, 10, false, false, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum!");
                y += 12.0f;

                blur = 10;
                print(c, x, y, 1.0f, m_font4, 40, false, false, "Etaion Shdrlu!");
                y += 42.0f;
                blur = 0;
            }

        private:
            base::FontPtr m_font;
            base::FontPtr m_font2;
            base::FontPtr m_font3;
            base::FontPtr m_font4;
        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_CanvasText);
        RTTI_METADATA(CanvasTestOrderMetadata).order(110);
        RTTI_END_TYPE();

        //---

    } // test
} // rendering