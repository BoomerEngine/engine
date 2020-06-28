/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/app/include/application.h"

#include "simpleCamera.h"

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

            void prepareSceneCommandBuffers(rendering::command::CommandWriter& cmd, const rendering::ImageView& color, const rendering::ImageView& depth, base::Rect& area);
            void prepareCanvasCommandBuffers(rendering::command::CommandWriter& cmd, const rendering::ImageView& color, const rendering::ImageView& depth, base::Rect& area);
            void renderCanvas(base::canvas::Canvas& c);
            void renderGui();

            //--

            ImGuiContext* m_imgui = nullptr;

            //--

            base::NativeTimePoint m_lastUpdateTime;
            double m_lastGameTime;
            int m_timeMultiplier = 0;
            bool m_timeAdvance = true;

            //--

            base::RefPtr<ISceneTest> m_currentTest;
            int m_currentTestCaseIndex;
            int m_pendingTestCaseIndex;

            rendering::ObjectID m_renderingOutput;
            rendering::IDriverNativeWindowInterface* m_renderingWindow = nullptr;
            bool m_exitRequested;

            rendering::scene::CameraContextPtr m_cameraContext;
            rendering::scene::Camera m_lastCamera;

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
} // game