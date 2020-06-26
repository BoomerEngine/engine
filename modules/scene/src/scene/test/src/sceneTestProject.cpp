/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "sceneTestProject.h"

#include "base/app/include/application.h"
#include "base/storage/include/xmlDataDocument.h"
#include "base/resources/include/resourceStaticResource.h"
#include "base/xml/include/xmlUtils.h"
#include "base/xml/include/xmlDocument.h"
#include "base/io/include/ioSystem.h"
#include "base/input/include/inputStructures.h"
#include "base/input/include/inputContext.h"

#include "rendering/scene/include/renderingFrameCamera.h"
#include "rendering/scene/include/renderingFrameCameraContext.h"
#include "rendering/runtime/include/renderingRuntimeService.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/scene/include/renderingFrame.h"
#include "rendering/scene/include/renderingFrameGeometryCanvas.h"
#include "rendering/scene/include/renderingFrameScreenCanvas.h"

#include "scene/common/include/sceneRuntime.h"
#include "scene/compiler/include/sceneNodeCollector.h"
#include "rendering/driver/include/renderingOutput.h"
#include "base/app/include/launcherPlatform.h"

namespace scene
{
    namespace test
    {

        ///---

        SceneFlyCamera::SceneFlyCamera()
            : m_cameraPosition(0, 0, 0)
            , m_cameraAngles(0, 0, 0)
        {
            memzero(m_cameraMovementKeys, sizeof(m_cameraMovementKeys));
        }

        bool SceneFlyCamera::processInput(const base::input::BaseEvent& evt)
        {
            // process events
            if (auto keyEvt  = evt.toKeyEvent())
            {
                if (keyEvt->pressed())
                {
                    if (keyEvt->keyCode() == base::input::KeyCode::KEY_W)
                    {
                        m_cameraMovementKeys[KEY_FOWARD] = true;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_S)
                    {
                        m_cameraMovementKeys[KEY_BACKWARD] = true;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_A)
                    {
                        m_cameraMovementKeys[KEY_LEFT] = true;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_D)
                    {
                        m_cameraMovementKeys[KEY_RIGHT] = true;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_Q)
                    {
                        m_cameraMovementKeys[KEY_UP] = true;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_E)
                    {
                        m_cameraMovementKeys[KEY_DOWN] = true;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_LEFT_SHIFT)
                    {
                        m_cameraMovementKeys[KEY_FAST] = true;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_LEFT_CTRL)
                    {
                        m_cameraMovementKeys[KEY_SLOW] = true;
                        return true;
                    }
                }
                else if (keyEvt->released())
                {
                    if (keyEvt->keyCode() == base::input::KeyCode::KEY_W)
                    {
                        m_cameraMovementKeys[KEY_FOWARD] = false;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_S)
                    {
                        m_cameraMovementKeys[KEY_BACKWARD] = false;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_A)
                    {
                        m_cameraMovementKeys[KEY_LEFT] = false;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_D)
                    {
                        m_cameraMovementKeys[KEY_RIGHT] = false;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_Q)
                    {
                        m_cameraMovementKeys[KEY_UP] = false;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_E)
                    {
                        m_cameraMovementKeys[KEY_DOWN] = false;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_LEFT_SHIFT)
                    {
                        m_cameraMovementKeys[KEY_FAST] = false;
                        return true;
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_LEFT_CTRL)
                    {
                        m_cameraMovementKeys[KEY_SLOW] = false;
                        return true;
                    }
                } // !pressed
            } // key
            else if (auto moveEvent  = evt.toMouseMoveEvent())
            {
                if (moveEvent->keyMask().isLeftDown())
                {
                    m_cameraAngles.pitch += moveEvent->delta().y * 0.1f;
                    m_cameraAngles.yaw += moveEvent->delta().x * 0.1f;
                    return true;
                }
            }

            return false;
        }

        void SceneFlyCamera::moveCamera(float dt)
        {
            base::Vector3 cameraMoveLocalDir(0, 0, 0);
            cameraMoveLocalDir.x += m_cameraMovementKeys[KEY_FOWARD] ? 1.0f : 0.0f;
            cameraMoveLocalDir.x -= m_cameraMovementKeys[KEY_BACKWARD] ? 1.0f : 0.0f;
            cameraMoveLocalDir.y += m_cameraMovementKeys[KEY_RIGHT] ? 1.0f : 0.0f;
            cameraMoveLocalDir.y -= m_cameraMovementKeys[KEY_LEFT] ? 1.0f : 0.0f;
            cameraMoveLocalDir.z += m_cameraMovementKeys[KEY_UP] ? 1.0f : 0.0f;
            cameraMoveLocalDir.z -= m_cameraMovementKeys[KEY_DOWN] ? 1.0f : 0.0f;

            float cameraSpeed = 5.0f;
            if (m_cameraMovementKeys[KEY_FAST]) cameraSpeed = 10.0f;
            if (m_cameraMovementKeys[KEY_SLOW]) cameraSpeed = 1.0f;

            if (!cameraMoveLocalDir.isZero())
            {
                cameraMoveLocalDir.normalize();
                cameraMoveLocalDir *= cameraSpeed;
            }

            base::Vector3 cameraDirForward, cameraDirRight, cameraDirUp;
            m_cameraAngles.angleVectors(cameraDirForward, cameraDirRight, cameraDirUp);

            m_cameraPosition += cameraDirForward * cameraMoveLocalDir.x * dt;
            m_cameraPosition += cameraDirRight * cameraMoveLocalDir.y * dt;
            m_cameraPosition += cameraDirUp * cameraMoveLocalDir.z * dt;
        }

        void SceneFlyCamera::calcCamera(uint32_t width, uint32_t height, rendering::scene::Camera& outCamera)
        {
            rendering::scene::CameraSetup setup;
            setup.aspect = width / (float)height;
            setup.farPlane = 1000.0f;
            setup.nearPlane = 0.01f;
            setup.fov = 90.0f;
            setup.zoom = 1.0f;
            setup.position = m_cameraPosition;
            setup.rotation = m_cameraAngles.toQuat();

            outCamera.setup(setup);
        }

        ///---

        RTTI_BEGIN_TYPE_CLASS(SceneTestProject);
        RTTI_END_TYPE();

        SceneTestProject::SceneTestProject()
            : m_currentTestCaseIndex(0)
        {
            m_camera.m_cameraPosition = base::Vector3(-3.0f, 0.0f, 1.5f);
            m_camera.m_cameraAngles.pitch = 30.0f;
        }

        SceneTestProject::~SceneTestProject()
        {
            if (m_scene)
            {
                if (m_currentTestCookedWorld)
                {
                    m_scene->detachWorldContent(m_currentTestCookedWorld);
                    m_currentTestCookedWorld.reset();
                }

                m_scene.reset();
            }

            if (m_renderingOutput)
            {
                m_renderingOutput->close();
                m_renderingOutput.reset();
            }
        }

        bool SceneTestProject::initialize(const base::app::CommandLine& commandline)
        {
            if (!createWindow())
                return false;

            if (!enumerateTests(commandline))
            {
                TRACE_ERROR("No scene tests implemented");
                return false;
            }


            // create scene
            m_scene = base::CreateSharedPtr<Scene>(SceneType::Game);

            // switch to initial test
            switchTest(m_currentTestCaseIndex);
            return true;
        }

        bool SceneTestProject::createWindow()
        {
            // determine viewport resolution for the test
            // NOTE: the tests may have hard coded placement so this resolution should not be changed that much
            uint32_t viewportWidth = 1920;
            uint32_t viewportHeight = 1080;

            rendering::DriverOutputInitInfo setup;
            setup.m_width = viewportWidth;
            setup.m_height = viewportHeight;
            setup.m_windowTitle = "Boomer Engine Scene Test";
            setup.m_class = rendering::DriverOutputClass::NativeWindow;

            // create rendering output
            m_renderingOutput = base::GetService<rendering::runtime::RuntimeService>()->driver()->createOutput(setup);
            if (!m_renderingOutput)
            {
                TRACE_ERROR("Failed to acquire window factory, no window can be created");
                return false;
            }

            return true;
        }

        void SceneTestProject::update()
        {
            // calc DT
            auto dt = std::clamp<float>(m_lastUpdateTime.timeTillNow().toSeconds(), 0.00001f, 0.1f);
            m_lastUpdateTime.resetToNow();

            // move camera
            m_camera.moveCamera(dt);

            // update test
            {
                UpdateContext ctx;
                ctx.m_dt = dt;
                m_scene->update(ctx);
            }

            // process window
            if (m_renderingOutput)
            {
                // if the rendering output is a window we can do some more stuff - mainly process input from user and other user interactions with the window
                if (auto window = m_renderingOutput->outputWindowInterface())
                {
                    // ask the main rendering output (that is a window in this case) if it has been closed
                    if (window->windowHasCloseRequest())
                        base::platform::GetLaunchPlatform().requestExit("Main window closed");

                    // process input 
                    if (auto inputContext = window->windowGetInputContext())
                    {
                        while (auto inputEvent = inputContext->pull())
                            processInput(*inputEvent);
                    }

                    // render scene
                    if (auto frameOutput = m_renderingOutput->prepareForFrameRendering())
                    {
                        // prepare main camera
                        rendering::scene::Camera camera;
                        m_camera.calcCamera(frameOutput->outputSize().x, frameOutput->outputSize().y, camera);

                        // create frame payload
                        auto frame = base::CreateSharedPtr<rendering::scene::FrameInfo>(frameOutput->outputSize().x, frameOutput->outputSize().y, camera, m_cameraContext);
                        drawGrid(*frame);
                        drawDebugInfo(*frame);
                        m_scene->render(*frame);

                        rendering::runtime::FrameContent frameContent;
                        frameContent.payload = frame;

                        // render the scene
                        auto runtimeSystem = base::GetService<rendering::runtime::RuntimeService>();
                        base::GetService<rendering::runtime::RuntimeService>()->renderFrame(frameOutput, &frameContent, 1);
                    }
                }
            }
        }

        void SceneTestProject::processInput(const base::input::BaseEvent& evt)
        {
            // pass to camera
            if (m_camera.processInput(evt))
                return;
                
            // basic navigation through tests
            if (auto keyEvt  = evt.toKeyEvent())
            {
                if (keyEvt->pressed())
                {
                    if (keyEvt->keyCode() == base::input::KeyCode::KEY_ESCAPE)
                    {
                        base::platform::GetLaunchPlatform().requestExit("User requested exit");
                        return;
                    }

                    if (keyEvt->keyCode() == base::input::KeyCode::KEY_LEFT)
                    {
                        auto newIndex = m_currentTestCaseIndex;
                        if (newIndex <= 0)
                            newIndex = m_testWorldPaths.lastValidIndex();
                        else
                            newIndex -= 1;
                        switchTest(newIndex);
                    }
                    else if (keyEvt->keyCode() == base::input::KeyCode::KEY_RIGHT)
                    {
                        auto newIndex = m_currentTestCaseIndex;
                        newIndex += 1;
                        if (newIndex >= (int)m_testWorldPaths.size())
                            newIndex = 0;
                        switchTest(newIndex);
                    }
                }
            }
        }
        
        bool SceneTestProject::enumerateTests(const base::app::CommandLine& cmdLine)
        {
            auto sceneTestPath = base::StringBuf("engine/tests/scenes/");

            /*base::Array<base::depot::DepotStructure::DirectoryInfo> worldDirectories;
            m_loader.enumDirectoriesAtPath(sceneTestPath, worldDirectories);
            TRACE_INFO("Found {} directorie(s) at test scene location '{}'", worldDirectories.size(), sceneTestPath);

            for (auto& dirInfo : worldDirectories)
            {
                if (dirInfo.name == "prefabs" || dirInfo.name.beginsWith("_"))
                    continue;

                base::StringBuf worldFileName = base::TempString("{}{}/test.v4world", sceneTestPath, dirInfo.name);
                uint64_t crc = 0;
                if (m_loader.queryFileCRC(worldFileName, crc))
                {
                    TRACE_INFO("Found world test scene at '{}'", worldFileName);
                    m_testWorldPaths.pushBack(base::res::ResourcePath(worldFileName));
                }
            }

            return !m_testWorldPaths.empty();*/

            return true;
        }

        void SceneTestProject::switchTest(int newTestIndex)
        {
            if (!m_testWorldPaths.empty())
            {
                // close current scene
                if (m_currentTestCookedWorld)
                    m_scene->detachWorldContent(m_currentTestCookedWorld);
                m_currentTestCookedWorld.reset();

                // create new test
                auto& newTestWorldPath = m_testWorldPaths[newTestIndex];
                m_currentTestCaseName = base::StringBuf(newTestWorldPath.path().afterLast("scenes/").beforeFirst("/"));
                m_currentTestCaseIndex = newTestIndex;

                // load the world
                /*if (auto world = base::LoadResource<World>(newTestWorldPath))
                {
                    base::ScopeTimer timer;

                    base::Array<base::res::ResourcePath> layers;
                    world->collectLayerPaths(m_loader, layers);
                    TRACE_INFO("Found {} layers in world", layers.size());

                    NodeCollector nodes;
                    for (auto& layer : layers)
                        nodes.extractNodesFromLayer(layer);

                    base::Array<WorldSectorDesc> sectors;
                    nodes.pack(sectors);
                    TRACE_INFO("Packed nodes into {} sectors", sectors.size());

                    m_currentTestCookedWorld = base::CreateSharedPtr<CompiledWorld>();
                    m_currentTestCookedWorld->content(world->parameters(), sectors);

                    TRACE_ERROR("Processed world from '{}' in {}", newTestWorldPath, TimeInterval(timer.timeElapsed()));
                }
                else
                {
                    TRACE_ERROR("Unable to load test world from '{}'", newTestWorldPath);
                }*/

                // attach content
                if (m_currentTestCookedWorld)
                    m_scene->attachWorldContent(m_currentTestCookedWorld);

                // update window caption
                if (auto window = m_renderingOutput->outputWindowInterface())
                    window->windowSetTitle(base::TempString("Boomer Engine Rendering Scene Tests - {}", m_currentTestCaseName));
            }
        }

        void SceneTestProject::drawDebugInfo(rendering::scene::FrameInfo& frame) const
        {
            rendering::scene::ScreenCanvas dd(frame);

            dd.lineColor(base::Color::WHITE);
            dd.text(20, 20, base::TempString("Test '{}'", m_currentTestCaseName));

            if (m_currentTestCookedWorld)
            {
                uint32_t numSectors = 0, numEntities = 0;

                for (auto& sector : m_currentTestCookedWorld->sectors())
                {
                    numSectors += 1;

                    if (sector.m_unsavedSectorData)
                        numEntities += sector.m_unsavedSectorData->m_entities.size();
                }

                dd.lineColor(base::Color::WHITE);
                dd.text(20, 38, base::TempString("Loaded {} sector(s), {} entitie(s)", numSectors, numEntities));
            }
            else
            {
                dd.lineColor(base::Color::RED);
                dd.text(20, 38, base::TempString("Failed to load"));
            }
        }

        void SceneTestProject::drawGrid(rendering::scene::FrameInfo& frame) const
        {
            rendering::scene::GeometryCanvas dd(frame);

            // huge grid
            {
                auto gridStep = 100.0f;
                auto gridSteps = 10;
                auto gridSize = gridStep * gridSteps;

                dd.lineColor(base::Color(100, 100, 100));
                for (int i = -gridSteps; i <= gridSteps; ++i)
                {
                    if (i != 0)
                    {
                        auto pos = (float)i * gridStep;
                        dd.line(base::Vector3(-gridSize, pos, 0.0f), base::Vector3(gridSize, pos, 0.0f));
                        dd.line(base::Vector3(pos, -gridSize, 0.0f), base::Vector3(pos, gridSize, 0.0f));
                    }
                }
            }

            // big grid
            {
                auto gridStep = 10.0f;
                auto gridSteps = 10;
                auto gridSize = gridStep * gridSteps;

                dd.lineColor(base::Color(110, 110, 110));
                for (int i = -gridSteps; i <= gridSteps; ++i)
                {
                    if (i != 0)
                    {
                        auto pos = (float)i * gridStep;
                        dd.line(base::Vector3(-gridSize, pos, 0.0f), base::Vector3(gridSize, pos, 0.0f));
                        dd.line(base::Vector3(pos, -gridSize, 0.0f), base::Vector3(pos, gridSize, 0.0f));
                    }
                }
            }

            // small grid
            {
                auto gridStep = 1.0f;
                auto gridSteps = 10;
                auto gridSize = gridStep * gridSteps;

                dd.lineColor(base::Color(120, 120, 120));
                for (int i = -gridSteps; i <= gridSteps; ++i)
                {
                    if (i != 0)
                    {
                        auto pos = (float)i * gridStep;
                        dd.line(base::Vector3(-gridSize, pos, 0.0f), base::Vector3(gridSize, pos, 0.0f));
                        dd.line(base::Vector3(pos, -gridSize, 0.0f), base::Vector3(pos, gridSize, 0.0f));
                    }
                }
            }

            // tiny grid
            {
                auto gridStep = 0.1f;
                auto gridSteps = 10;
                auto gridSize = gridStep * gridSteps;

                dd.lineColor(base::Color(130, 130, 130));
                for (int i = -gridSteps; i <= gridSteps; ++i)
                {
                    if (i != 0)
                    {
                        auto pos = (float)i * gridStep;
                        dd.line(base::Vector3(-gridSize, pos, 0.0f), base::Vector3(gridSize, pos, 0.0f));
                        dd.line(base::Vector3(pos, -gridSize, 0.0f), base::Vector3(pos, gridSize, 0.0f));
                    }
                }
            }

            // center lines
            dd.lineColor(base::Color(130, 150, 150));
            dd.line(base::Vector3(-1000.0f, 0.0f, 0.0f), base::Vector3(1000.0f, 0.0f, 0.0f));
            dd.line(base::Vector3(0.0f, -1000.0f, 0.0f), base::Vector3(0.0f, 1000.0f, 0.0f));
        }

    } // test
} // scene


base::app::IApplication& GetApplicationInstance()
{
    static scene::test::SceneTestProject theApp;
    return theApp;
}


