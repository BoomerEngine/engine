/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/app/include/application.h"

#include "rendering/scene/include/renderingSceneStats.h"

namespace game
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

            void prepareSceneCommandBuffers(rendering::command::CommandWriter& cmd, const rendering::scene::FrameCompositionTarget& target);
            void prepareCanvasCommandBuffers(rendering::command::CommandWriter& cmd, const rendering::scene::FrameCompositionTarget& target);
            void renderCanvas(base::canvas::Canvas& c);

            //--

			ImGui::ImGUICanvasHelper* m_imguiHelper = nullptr;

            //--

            base::NativeTimePoint m_lastUpdateTime;
            double m_lastGameTime;
            int m_timeMultiplier = 0;
            bool m_timeAdvance = true;

            //--

            base::RefPtr<ISceneTest> m_currentTest;
            int m_currentTestCaseIndex;
            int m_pendingTestCaseIndex;

            rendering::OutputObjectPtr m_renderingOutput;
            bool m_exitRequested;

            rendering::scene::CameraContextPtr m_cameraContext;
            rendering::scene::Camera m_lastCamera;

            rendering::scene::FrameStats m_lastFrameStats;
            rendering::scene::SceneStats m_lastSceneStats;

            rendering::ImageObjectPtr m_flippedColorTarget;
            rendering::RenderTargetViewPtr m_flippedColorTargetRTV;

            rendering::ImageObjectPtr m_flippedDepthTarget;
            rendering::RenderTargetViewPtr m_flippedDepthTargetRTV;

            bool createRenderingOutput();
            bool shouldCaptureInput();

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
} // game