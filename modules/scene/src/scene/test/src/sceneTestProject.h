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
#include "scene/common/include/sceneWorld.h"
#include "rendering/scene/include/renderingFrameCamera.h"

namespace scene
{
    namespace test
    {
        
        //---

        /// a simple fly camera for tests that support it
        struct SceneFlyCamera
        {
            enum MovementKey
            {
                KEY_FOWARD,
                KEY_BACKWARD,
                KEY_LEFT,
                KEY_RIGHT,
                KEY_UP,
                KEY_DOWN,
                KEY_FAST,
                KEY_SLOW,

                KEY_MAX,
            };

            base::Vector3 m_cameraPosition;
            base::Angles m_cameraAngles;
            bool m_cameraMovementKeys[KEY_MAX]; // F/B/L/R/U/D/F/S

            SceneFlyCamera();

            void moveCamera(float dt);
            bool processInput(const base::input::BaseEvent& evt);
            void calcCamera(uint32_t width, uint32_t height, rendering::scene::Camera& outCamera);
        };

        //---

        /// boilerplate for rendering scene test
        /// contains basic scene initialization and other shit
        class SceneTestProject : public base::app::IApplication
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneTestProject, base::app::IApplication);

        public:
            SceneTestProject();
            virtual ~SceneTestProject();

        protected:
            virtual bool initialize(const base::app::CommandLine& commandline) override final;
            virtual void update() override final;

            base::Array<base::res::ResourcePath> m_testWorldPaths;

            ScenePtr m_scene;

            CompiledWorldPtr m_currentTestCookedWorld;
            int m_currentTestCaseIndex;
            base::StringBuf m_currentTestCaseName;

            base::NativeTimePoint m_lastUpdateTime;

            rendering::DriverOutputPtr m_renderingOutput;

            rendering::scene::CameraContextPtr m_cameraContext;

            //base::depot::DepotStructure m_loader;

            SceneFlyCamera m_camera;

            //--

            bool enumerateTests(const base::app::CommandLine& cmdLine);
            void switchTest(int newTestIndex);

            bool createWindow();

            void processInput(const base::input::BaseEvent& evt);

            void drawGrid(rendering::scene::FrameInfo& frame) const;
            void drawDebugInfo(rendering::scene::FrameInfo& frame) const;
        };      

    } // test
} // scene