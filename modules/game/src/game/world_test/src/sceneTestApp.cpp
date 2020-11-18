/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "sceneTest.h"
#include "sceneTestApp.h"

#include "base/app/include/application.h"
#include "base/input/include/inputContext.h"
#include "base/input/include/inputStructures.h"
#include "base/canvas/include/canvas.h"
#include "base/app/include/launcherPlatform.h"
#include "base/imgui/include/debugPageService.h"


#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingOutput.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/canvas/include/renderingCanvasRenderingService.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/scene/include/renderingFrameRenderingService.h"
#include "rendering/scene/include/renderingFrameParams.h"
#include "rendering/scene/include/renderingFrameCameraContext.h"

namespace game
{
    namespace test
    {

        SceneTestProject::SceneTestProject()
            : m_currentTest(nullptr)
            , m_currentTestCaseIndex(-1)
            , m_pendingTestCaseIndex(0)
            , m_exitRequested(false)
        {
        }

        void SceneTestProject::cleanup()
        {
            m_currentTest.reset();
            m_cameraContext.reset();

            if (m_imgui)
            {
                ImGui::SetCurrentContext(nullptr);
                ImGui::DestroyContext(m_imgui);
                m_imgui = nullptr;
            }

            m_renderingOutput.reset();
        }

        bool SceneTestProject::initialize(const base::app::CommandLine& cmdLine)
        {
            // list all test classes
            base::InplaceArray<base::ClassType, 100> testClasses;
            RTTI::GetInstance().enumClasses(ISceneTest::GetStaticClass(), testClasses);
            TRACE_INFO("Found {} test classes", testClasses.size());

            // sort test classes by the order
            std::sort(testClasses.begin(), testClasses.end(), [](base::ClassType a, base::ClassType b)
                {
                    auto orderA  = a->findMetadata<SceneTestOrderMetadata >();
                    auto orderB  = b->findMetadata<SceneTestOrderMetadata >();

                    auto valA = orderA ? orderA->m_order : -1;
                    auto valB = orderB ? orderB->m_order : -1;

                    return valA < valB;
                });

            // use only classes with valid order
            auto initialTest = cmdLine.singleValue("test");
            for (auto testClass  : testClasses)
            {
                auto shortTestName = testClass->name().view().afterLast("::").afterFirstOrFull("SceneTest_");

                auto order  = testClass->findMetadata<SceneTestOrderMetadata >();
                if (!order || order->m_order < 0)
                {
                    if (initialTest != shortTestName)
                    {
                        TRACE_WARNING("Test class '{}' has no order specified, ignoring", testClass->name());
                        continue;
                    }
                }

                auto& testCase = m_testClasses.emplaceBack();
                testCase.m_testClass = testClass;
                testCase.m_testName = base::StringBuf(shortTestName);
            }

            // nothing to test
            TRACE_INFO("Found {} valid test cases", m_testClasses.size());
            if (m_testClasses.empty())
                return 0;

            // print tests (for reference)
            for (uint32_t i = 0; i < m_testClasses.size(); ++i)
            {
                if (initialTest == m_testClasses[i].m_testName)
                {
                    m_pendingTestCaseIndex = i;
                }
            }

            // setup rendering output
            if (!createRenderingOutput())
                return false;

            m_imgui = ImGui::CreateContext();// ImGui::GetSharedFontAtlas());
            ImGui::SetCurrentContext(m_imgui);

            m_cameraContext = base::RefNew<rendering::scene::CameraContext>();

            m_lastUpdateTime.resetToNow();
            return true;
        }

        bool SceneTestProject::createRenderingOutput()
        {
            // determine viewport resolution for the test
            // NOTE: the tests may have hard coded placement so this resolution should not be changed that much
            uint32_t viewportWidth = 1920;
            uint32_t viewportHeight = 1080;
            TRACE_INFO("Test viewport size: {}x{}", viewportWidth, viewportHeight);

            rendering::OutputInitInfo setup;
            setup.m_width = viewportWidth;
            setup.m_height = viewportHeight;
            setup.m_windowTitle = "Boomer Engine Game Scene Tests";
            setup.m_class = rendering::DriverOutputClass::NativeWindow;

            // create rendering output
            m_renderingOutput = base::GetService<DeviceService>()->device()->createOutput(setup);
            if (!m_renderingOutput || !m_renderingOutput->window())
            {
                TRACE_ERROR("Failed to acquire window factory, no window can be created");
                return false;
            }

            return true;
        }

        base::RefPtr<ISceneTest> SceneTestProject::initializeTest(uint32_t testIndex)
        {
            auto& testClass = m_testClasses[testIndex];

            // initialize the test
            auto testCase = testClass.m_testClass->create<ISceneTest>();
            if (!testCase || !testCase->processInitialization())
            {
                TRACE_ERROR("Failed to initialize test '{}'", testClass.m_testClass->name());
                return nullptr;
            }

            return testCase;
        }

        void SceneTestProject::update()
        {
            // exit when window closed
            if (m_renderingOutput->window()->windowHasCloseRequest())
            {
                TRACE_INFO("Main window closed, exiting");
                base::platform::GetLaunchPlatform().requestExit("Window closed");
                return;
            }

            // user exit requested
            if (m_exitRequested)
            {
                base::platform::GetLaunchPlatform().requestExit("User exit");
                return;
            }

            // simulation
            {
                auto systemTimeElapsed = m_lastUpdateTime.timeTillNow().toSeconds();
                m_lastUpdateTime.resetToNow();

                auto gameTime = m_lastGameTime;
                if (m_timeAdvance)
                {
                    if (m_timeMultiplier)
                        gameTime += systemTimeElapsed * std::pow(2.0f, m_timeMultiplier);
                    else
                        gameTime += systemTimeElapsed;

                    float dt = std::clamp<float>(gameTime - m_lastGameTime, 0.0001f, 0.2f);
                    if (dt > 0.0f)
                        handleTick(dt);

                    m_lastGameTime = gameTime;
                }
            }

            // process events
            if (auto inputContext = m_renderingOutput->window()->windowGetInputContext())
                handleInput(*inputContext);

            // navigate to other test
            if (m_pendingTestCaseIndex != m_currentTestCaseIndex)
            {
                // close current test
                if (m_currentTest)
                    m_currentTest.reset();

                // initialize new test
                m_currentTestCaseIndex = m_pendingTestCaseIndex;
                m_currentTest = initializeTest(m_currentTestCaseIndex);

                // set the window caption
                auto testName = m_testClasses[m_currentTestCaseIndex].m_testName;
                m_renderingOutput->window()->windowSetTitle(base::TempString("Boomer Engine Scene Tests - {} {}", testName, m_currentTest ? "" : "(FAILED TO INITIALIZE)"));
            }

            // create output frame, this will fail if output is not valid
            {
                rendering::command::CommandWriter cmd("TestScene");

                if (auto output = cmd.opAcquireOutput(m_renderingOutput))
                {
                    prepareSceneCommandBuffers(cmd, output.color, output.depth, output.width, output.width);
                    prepareCanvasCommandBuffers(cmd, output.color, output.depth, output.height, output.height);

                    cmd.opSwapOutput(m_renderingOutput);
                }

                base::GetService<DeviceService>()->device()->submitWork(cmd.release());
            }
        }

        void SceneTestProject::handleTick(float dt)
        {
            if (m_currentTest)
                m_currentTest->update(dt);

            ImGui::GetIO().DeltaTime = dt;
        }

        bool SceneTestProject::handleAppInputEvent(const base::input::BaseEvent& evt)
        {
            auto keyEvt = evt.toKeyEvent();
            if (keyEvt && keyEvt->pressed())
            {
                if (keyEvt->keyCode() == base::input::KeyCode::KEY_ESCAPE)
                {
                    m_exitRequested = true;
                    return true;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_LEFT)
                {
                    if (m_pendingTestCaseIndex <= 0)
                        m_pendingTestCaseIndex = m_testClasses.size() - 1;
                    else
                        m_pendingTestCaseIndex -= 1;
                    return true;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_RIGHT)
                {
                    m_pendingTestCaseIndex += 1;
                    if (m_pendingTestCaseIndex >= (int)m_testClasses.size())
                        m_pendingTestCaseIndex = 0;
                    return true;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_F1)
                {
                    base::DebugPagesVisibility(!base::DebugPagesVisible());
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_F2)
                {
                    m_timeMultiplier = 0;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_PAUSE)
                {
                    m_timeAdvance = !m_timeAdvance;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_MINUS)
                {
                    if (m_timeAdvance)
                        m_timeMultiplier -= 1;
                    else
                        m_timeAdvance = true;
                }
                else if (keyEvt->keyCode() == base::input::KeyCode::KEY_EQUAL)
                {
                    if (m_timeAdvance)
                        m_timeMultiplier += 1;
                    else
                        m_timeAdvance = true;
                }
            }

            return false;
        }

        bool SceneTestProject::handleInputEvent(const base::input::BaseEvent& evt)
        {
            if (handleAppInputEvent(evt))
                return true;

            if (base::DebugPagesVisible())
            {
                if (ImGui::ProcessInputEvent(m_imgui, evt))
                    return true;
            }
            else
            {
                if (m_currentTest && m_currentTest->processInput(evt))
                    return true;
            }

            return false;
        }

        void SceneTestProject::handleInput(base::input::IContext& context)
        {
            while (auto evt = context.pull())
                handleInputEvent(*evt);

            const auto shouldCapture = m_timeAdvance && !base::DebugPagesVisible();
            context.requestCapture(shouldCapture ? 2 : 0);
        }
        
        void SceneTestProject::prepareSceneCommandBuffers(rendering::command::CommandWriter& cmd, const rendering::ImageView& color, const rendering::ImageView& depth, uint32_t width, uint32_t height)
        {
            rendering::scene::FrameParams frame(width, height, rendering::scene::Camera());
            frame.resolution.width = width;
            frame.resolution.height = height;
            frame.resolution.finalCompositionWidth = width;
            frame.resolution.finalCompositionHeight = height;
            frame.filters = rendering::scene::FilterFlags::DefaultGame();
            frame.mode = rendering::scene::FrameRenderMode::Default;
            frame.time.engineRealTime = m_lastGameTime;
            frame.time.gameTime = m_lastGameTime;
            frame.camera.cameraContext = m_cameraContext;

            m_currentTest->render(frame);

            m_lastCamera = frame.camera.camera;
            m_lastFrameStats.reset();
            m_lastSceneStats.reset();

            // generate command buffers
            if (auto sceneRenderingCommands = base::GetService<rendering::scene::FrameRenderingService>()->renderFrame(frame, color, &m_lastFrameStats, &m_lastSceneStats))
                cmd.opAttachChildCommandBuffer(sceneRenderingCommands);
        }

        static base::res::StaticResource<base::font::Font> resDefaultFont("/engine/fonts/aileron_regular.otf");

        static void Print(base::canvas::Canvas& c, const base::StringBuf& text, base::Color color = base::Color::WHITE, int size=16, 
            base::font::FontAlignmentHorizontal align = base::font::FontAlignmentHorizontal::Left, bool bold=false)
        {
            if (auto font = resDefaultFont.loadAndGet())
            {
                base::font::FontStyleParams params;
                params.bold = bold;
                params.size = size;

                base::font::GlyphBuffer glyphs;
                base::font::FontAssemblyParams assemblyParams;
                assemblyParams.horizontalAlignment = align;
                font->renderText(params, assemblyParams, base::font::FontInputText(text.c_str()), glyphs);

                base::canvas::GeometryBuilder b;
                b.fillColor(color);
                b.print(glyphs);
                c.place(b);
            }
        }

        void SceneTestProject::prepareCanvasCommandBuffers(rendering::command::CommandWriter& parentCmd, const rendering::ImageView& color, const rendering::ImageView& depth, uint32_t width, uint32_t height)
        {
            base::canvas::Canvas canvas(color.width(), color.height());
            renderCanvas(canvas);

            rendering::command::CommandWriter cmd(parentCmd.opCreateChildCommandBuffer());

            // set frame buffer
            rendering::FrameBuffer fb;
            fb.color[0].view(color);// .clear(0.2, 0.2f, 0.2f, 1.0f);
            fb.depth.view(depth).clearDepth().clearStencil();
            cmd.opBeingPass(fb);

            // render canvas to command buffer
            rendering::canvas::CanvasRenderingParams renderingParams;
            renderingParams.frameBufferWidth = color.width();
            renderingParams.frameBufferHeight = depth.height();
            base::GetService<CanvasService>()->render(cmd, canvas, renderingParams);

            // finish pass
            cmd.opEndPass();
        }

        base::ConfigProperty<bool> cvShowSceneStats("DebugPage.Rendering.FrameStats", "IsVisible", false);

        void SceneTestProject::renderCanvas(base::canvas::Canvas& canvas)
        {
            // Local stuff
            {
                canvas.placement(canvas.width() - 20, canvas.height() - 60);
                Print(canvas,
                    base::TempString("Camera Position: [X={}, Y={}, Z={}]", Prec(m_lastCamera.position().x, 2), Prec(m_lastCamera.position().y, 2), Prec(m_lastCamera.position().z, 2)),
                    base::Color::WHITE, 16, base::font::FontAlignmentHorizontal::Right);

                if (base::DebugPagesVisible())
                {
                    canvas.placement(canvas.width() - 20, canvas.height() - 40);
                    Print(canvas,
                        base::TempString("DEBUG GUI ENABLED"),
                        base::Color::RED, 16, base::font::FontAlignmentHorizontal::Right);
                }

                /*canvas.placement(canvas.width() - 20, canvas.height() - 40);
                Print(canvas,
                    base::TempString("Camera Rotation: [P={}, Y={}]", Prec(m_lastCamera.rotation().pitch, 1), Prec(m_lastCamera.rotation().yaw, 1)),
                    base::Color::WHITE, 16, base::font::FontAlignmentHorizontal::Right);*/

                if (!m_timeAdvance)
                {
                    canvas.placement(canvas.width() - 20, 20);
                    Print(canvas,
                        base::TempString("PAUSED"),
                        base::Color::ORANGERED, 20, base::font::FontAlignmentHorizontal::Right);
                }
                else if (m_timeMultiplier > 0)
                {
                    canvas.placement(canvas.width() - 20, 20);
                    Print(canvas,
                        base::TempString("Time x{} faster", 1ULL << m_timeMultiplier),
                        base::Color::ORANGERED, 20, base::font::FontAlignmentHorizontal::Right);
                }
                else if (m_timeMultiplier < 0)
                {
                    canvas.placement(canvas.width() - 20, 20);
                    Print(canvas,
                        base::TempString("Time x{} slower", 1ULL << (-m_timeMultiplier)),
                        base::Color::ORANGERED, 20, base::font::FontAlignmentHorizontal::Right);
                }
            }

            // ImGui
            if (base::DebugPagesVisible())
            {
                ImGui::BeginCanvasFrame(canvas);

                base::DebugPagesRender();

                if (m_currentTest)
                    m_currentTest->configure();

                if (cvShowSceneStats.get() && ImGui::Begin("Frame stats", &cvShowSceneStats.get()))
                {
                    rendering::scene::RenderStatsGui(m_lastFrameStats, m_lastSceneStats);
                    ImGui::End();
                }

                ImGui::EndCanvasFrame(canvas);
            }
        }

    } // test
} // rendering

base::app::IApplication& GetApplicationInstance()
{
    static game::test::SceneTestProject theApp;
    return theApp;
}

