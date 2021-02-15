/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/app/include/application.h"

#include "simpleCamera.h"

namespace rendering
{
    namespace test
    {
        class ISceneTest;
        
        /// boilerplate for rendering scene test
        /// contains basic scene initialization and other shit
        class SceneTestProject : public base::app::IApplication
        {
        public:
            SceneTestProject();

        protected:
            virtual bool initialize(const base::app::CommandLine& commandline) override;
            virtual void update() override;
            virtual void cleanup() override;

            void handleTick(float dt);
            void handleInput(base::input::IContext& context);
            bool handleInputEvent(const base::input::BaseEvent& evt);
            bool handleAppInputEvent(const base::input::BaseEvent& evt);

            void prepareSceneCommandBuffers(command::CommandWriter& cmd, const scene::FrameCompositionTarget& target);
            void prepareCanvasCommandBuffers(command::CommandWriter& cmd, const scene::FrameCompositionTarget& target);
            void renderCanvas(base::canvas::Canvas& c);
            void renderGui();
            
            //--

            SimpleCamera m_camera;

			ImGui::ImGUICanvasHelper* m_imguiHelper = nullptr;

            base::NativeTimePoint m_lastUpdateTime;
            double m_lastGameTime;
            int m_timeMultiplier = 0;
            bool m_timeAdvance = true;

            //--

            base::RefPtr<ISceneTest> m_currentTest;
            int m_currentTestCaseIndex;
            int m_pendingTestCaseIndex;

            OutputObjectPtr m_renderingOutput;
            bool m_exitRequested;

            bool createRenderingOutput();

            //--

            struct TestInfo
            {
                base::StringBuf m_testName;
                base::ClassType m_testClass = nullptr;
            };

            // test classes
            base::Array<TestInfo> m_testClasses;

            //--

            base::RefPtr<ISceneTest> initializeTest(uint32_t testIndex);
        };      

    } // test
} // scene