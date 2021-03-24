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

#include "core/app/include/application.h"
#include "core/input/include/inputContext.h"
#include "core/input/include/inputStructures.h"
#include "engine/canvas/include/canvas.h"
#include "core/app/include/launcherPlatform.h"
#include "engine/imgui/include/debugPageService.h"

#include "gpu/device/include/device.h"
#include "gpu/device/include/output.h"
#include "gpu/device/include/commandBuffer.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/commandWriter.h"
#include "engine/rendering/include/service.h"
#include "engine/rendering/include/params.h"
#include "engine/rendering/include/cameraContext.h"
#include "engine/canvas/include/service.h"
#include "../../world/include/world.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

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

	delete m_imguiHelper;
	m_imguiHelper = nullptr;

	m_renderingOutput.reset();
}

bool SceneTestProject::initialize(const CommandLine& cmdLine)
{
    // list all test classes
    InplaceArray<ClassType, 100> testClasses;
    RTTI::GetInstance().enumClasses(ISceneTest::GetStaticClass(), testClasses);
    TRACE_INFO("Found {} test classes", testClasses.size());

    // sort test classes by the order
    std::sort(testClasses.begin(), testClasses.end(), [](ClassType a, ClassType b)
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
        testCase.m_testName = StringBuf(shortTestName);
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

	m_imguiHelper = new ImGui::ImGUICanvasHelper();

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

    gpu::OutputInitInfo setup;
    setup.m_width = viewportWidth;
    setup.m_height = viewportHeight;
    setup.m_windowTitle = "Boomer Engine Game Scene Tests";
    setup.m_class = gpu::OutputClass::Window;
	setup.m_windowStartInFullScreen = false;
	setup.m_windowAllowFullscreenToggle = true;

    // create rendering output
    m_renderingOutput = GetService<DeviceService>()->device()->createOutput(setup);
    if (!m_renderingOutput || !m_renderingOutput->window())
    {
        TRACE_ERROR("Failed to acquire window factory, no window can be created");
        return false;
    }

    return true;
}

RefPtr<ISceneTest> SceneTestProject::initializeTest(uint32_t testIndex)
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
        GetLaunchPlatform().requestExit("Window closed");
        return;
    }

    // user exit requested
    if (m_exitRequested)
    {
        GetLaunchPlatform().requestExit("User exit");
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
        m_renderingOutput->window()->windowSetTitle(TempString("Boomer Engine Scene Tests - {} {}", testName, m_currentTest ? "" : "(FAILED TO INITIALIZE)"));
    }

    // create output frame, this will fail if output is not valid
    {
        gpu::CommandWriter cmd("TestScene");

        if (auto output = cmd.opAcquireOutput(m_renderingOutput))
        {
            if (m_currentTest)
                m_currentTest->renderFrame(cmd, output, m_lastFrameStats, m_lastCamera);

			prepareCanvasCommandBuffers(cmd, output);

            cmd.opSwapOutput(m_renderingOutput);
        }

        GetService<DeviceService>()->device()->submitWork(cmd.release());
    }
}

void SceneTestProject::handleTick(float dt)
{
    if (m_currentTest)
        m_currentTest->update(dt);
}

bool SceneTestProject::handleAppInputEvent(const InputEvent& evt)
{
    auto keyEvt = evt.toKeyEvent();
    if (keyEvt && keyEvt->pressed())
    {
        if (keyEvt->keyCode() == InputKey::KEY_ESCAPE)
        {
            m_exitRequested = true;
            return true;
        }
        else if (keyEvt->keyCode() == InputKey::KEY_LEFT)
        {
            if (m_pendingTestCaseIndex <= 0)
                m_pendingTestCaseIndex = m_testClasses.size() - 1;
            else
                m_pendingTestCaseIndex -= 1;
            return true;
        }
        else if (keyEvt->keyCode() == InputKey::KEY_RIGHT)
        {
            m_pendingTestCaseIndex += 1;
            if (m_pendingTestCaseIndex >= (int)m_testClasses.size())
                m_pendingTestCaseIndex = 0;
            return true;
        }
        else if (keyEvt->keyCode() == InputKey::KEY_F1)
        {
            DebugPagesVisibility(!DebugPagesVisible());
        }
        else if (keyEvt->keyCode() == InputKey::KEY_F2)
        {
            m_timeMultiplier = 0;
        }
        else if (keyEvt->keyCode() == InputKey::KEY_PAUSE)
        {
            m_timeAdvance = !m_timeAdvance;
        }
        else if (keyEvt->keyCode() == InputKey::KEY_MINUS)
        {
            if (m_timeAdvance)
                m_timeMultiplier -= 1;
            else
                m_timeAdvance = true;
        }
        else if (keyEvt->keyCode() == InputKey::KEY_EQUAL)
        {
            if (m_timeAdvance)
                m_timeMultiplier += 1;
            else
                m_timeAdvance = true;
        }
    }

    return false;
}

bool SceneTestProject::handleInputEvent(const InputEvent& evt)
{
    if (handleAppInputEvent(evt))
        return true;

    if (DebugPagesVisible())
    {
        if (m_imguiHelper->processInput(evt))
            return true;
    }
    else
    {
        if (m_currentTest && m_currentTest->processInput(evt))
            return true;
    }

    return false;
}

void SceneTestProject::handleInput(IInputContext& context)
{
    while (auto evt = context.pull())
        handleInputEvent(*evt);

    const auto shouldCapture = m_timeAdvance && !DebugPagesVisible();
    context.requestCapture(shouldCapture ? 2 : 0);
}

void SceneTestProject::prepareCanvasCommandBuffers(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output)
{
    Canvas canvas(output.width, output.height);
	renderCanvas(canvas);

	{
        gpu::FrameBuffer fb;
        fb.color[0].view(output.color);
        fb.depth.view(output.depth);

        cmd.opBeingPass(fb);
        GetService<CanvasService>()->render(cmd, canvas);
        cmd.opEndPass();
	}
}

void SceneTestProject::renderCanvas(Canvas& canvas)
{
    // Local stuff
    {
        if (DebugPagesVisible())
        {
            canvas.debugPrint(canvas.width() - 20, canvas.height() - 40,
                TempString("DEBUG GUI ENABLED"),
                Color::RED, 16, FontAlignmentHorizontal::Right);
        }

        /*canvas.placement(canvas.width() - 20, canvas.height() - 40);
        Print(canvas,
            TempString("Camera Rotation: [P={}, Y={}]", Prec(m_lastCamera.rotation().pitch, 1), Prec(m_lastCamera.rotation().yaw, 1)),
            Color::WHITE, 16, FontAlignmentHorizontal::Right);*/

        if (!m_timeAdvance)
        {
            canvas.debugPrint(canvas.width() - 20, 20,
                TempString("PAUSED"),
                Color::ORANGERED, 20, FontAlignmentHorizontal::Right);
        }
        else if (m_timeMultiplier > 0)
        {
            canvas.debugPrint(canvas.width() - 20, 20,
                TempString("Time x{} faster", 1ULL << m_timeMultiplier),
                Color::ORANGERED, 20, FontAlignmentHorizontal::Right);
        }
        else if (m_timeMultiplier < 0)
        {
            canvas.debugPrint(canvas.width() - 20, 20,
                TempString("Time x{} slower", 1ULL << (-m_timeMultiplier)),
                Color::ORANGERED, 20, FontAlignmentHorizontal::Right);
        }
    }

    // ImGui
    if (DebugPagesVisible())
    {
		m_imguiHelper->beginFrame(canvas, 0.01f);

        DebugPagesRender();

        if (m_currentTest)
            m_currentTest->configure();

        rendering::RenderStatsGui(m_lastFrameStats);

		m_imguiHelper->endFrame(canvas);
    }
}

END_BOOMER_NAMESPACE_EX(test)

boomer::IApplication& GetApplicationInstance()
{
    static boomer::test::SceneTestProject theApp;
    return theApp;
}

