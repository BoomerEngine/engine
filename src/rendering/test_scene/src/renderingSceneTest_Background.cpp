/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "renderingSceneTest.h"

#include "base/image/include/image.h"

BEGIN_BOOMER_NAMESPACE(rendering::test)


        //--

        class SceneTest_Background : public ISceneTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTest_Background, ISceneTest);

        public:
            SceneTest_Background()
            {
                m_backgroundModes.pushBack("NoClear"_id);
                m_backgroundModes.pushBack("Clear"_id);
                m_backgroundModes.pushBack("Gradient"_id);
                m_backgroundModes.pushBack("Checker"_id);
                m_currentBackgroundMode = "Clear"_id;

                m_clearColor = base::Vector4(0.2f, 0.2f, 0.2f, 1.0f);

                m_gradientColorA = base::Vector4(0.45f, 0.45f, 0.5f, 1.0f);
                m_gradientColorB = base::Vector4(0.35f, 0.35f, 0.4f, 1.0f);

                m_checkerColorA = base::Vector4(0.8f, 0.8f, 0.8f, 1.0f);
                m_checkerColorB = base::Vector4(0.4f, 0.4f, 0.4f, 1.0f);
                m_checkerSize = 20;
            }

            virtual void setupFrame(scene::FrameParams& frame) override
            {
                frame.clear.clearColor = m_clearColor;

                if (m_currentBackgroundMode == "NoClear"_id)
                    frame.clear.clear = false;
                else
                    frame.clear.clear = true;

                /*frame.setup("Mode"_id, "Main"_id);
                frame.setup("ClearMode"_id, m_currentBackgroundMode);
                frame.setup("ClearColor"_id, base::Color::FromVectorLinear());
                frame.setup("CheckerColorA"_id, base::Color::FromVectorLinear(m_checkerColorA));
                frame.setup("CheckerColorB"_id, base::Color::FromVectorLinear(m_checkerColorB));
                frame.setup("CheckerSize"_id, m_checkerSize);
                frame.setup("Dupa"_id, m_checkerSize);
                frame.setup("Dupa2"_id, m_checkerSize);*/
            }

            virtual void configure() override
            {
                if (ImGui::Begin("Background"))
                {
                    if (ImGui::BeginCombo("Mode", m_currentBackgroundMode.c_str()))
                    {
                        for (const auto& mode : m_backgroundModes)
                        {
                            ImGui::PushID(mode.c_str());
                            if (ImGui::Selectable(mode.c_str(), m_currentBackgroundMode == mode))
                                m_currentBackgroundMode = mode;
                            ImGui::PopID();
                        }
                        ImGui::EndCombo();
                    }

                    if (m_currentBackgroundMode == "Clear"_id)
                    {
                        ImGui::ColorEdit3("Color", (float*)&m_clearColor, 0);
                    }
                    else if (m_currentBackgroundMode == "Gradient"_id)
                    {
                        ImGui::ColorEdit3("Horizon", (float*)&m_gradientColorA, 0);
                        ImGui::ColorEdit3("Zenith", (float*)&m_gradientColorB, 0);
                    }
                    else if (m_currentBackgroundMode == "Checker"_id)
                    {
                        ImGui::ColorEdit3("A##4", (float*)&m_checkerColorB, 0);
                        ImGui::ColorEdit3("B##5", (float*)&m_checkerColorA, 0);
                        ImGui::DragInt("Size##6", (int*)&m_checkerSize, 1.0f, 1, 200);
                    }
                }

                ImGui::End();
            }

            base::Array<base::StringID> m_backgroundModes;
            base::StringID m_currentBackgroundMode;

            base::Vector4 m_clearColor;

            base::Vector4 m_gradientColorA;
            base::Vector4 m_gradientColorB;

            base::Vector4 m_checkerColorA;
            base::Vector4 m_checkerColorB;
            uint32_t m_checkerSize;

        };

        RTTI_BEGIN_TYPE_CLASS(SceneTest_Background);
            RTTI_METADATA(SceneTestOrderMetadata).order(10);
        RTTI_END_TYPE();
        
        //--

    } // test
} // rendering