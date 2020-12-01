/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "renderingCanvasTest.h"
#include "renderingCanvasTestProject.h"

#include "base/app/include/application.h"
#include "base/input/include/inputContext.h"
#include "base/input/include/inputStructures.h"
#include "base/canvas/include/canvas.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingOutput.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/canvas/include/renderingCanvasRenderingService.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "base/app/include/launcherPlatform.h"

namespace rendering
{
    namespace test
    {

        CanvasTestProject::CanvasTestProject()
            : m_currentTest(nullptr)
            , m_currentTestCaseIndex(-1)
            , m_pendingTestCaseIndex(0)
            , m_exitRequested(false)
        {
        }

        void CanvasTestProject::cleanup()
        {
            m_currentTest.reset();
            m_renderingOutput.reset();
        }

        bool CanvasTestProject::initialize(const base::app::CommandLine& cmdLine)
        {
            // list all test classes
            base::InplaceArray<base::ClassType, 100> testClasses;
            RTTI::GetInstance().enumClasses(ICanvasTest::GetStaticClass(), testClasses);
            TRACE_INFO("Found {} test classes", testClasses.size());

            // sort test classes by the order
            std::sort(testClasses.begin(), testClasses.end(), [](base::ClassType a, base::ClassType b)
                {
                    auto orderA  = a->findMetadata<CanvasTestOrderMetadata >();
                    auto orderB  = b->findMetadata<CanvasTestOrderMetadata >();

                    auto valA = orderA ? orderA->m_order : -1;
                    auto valB = orderB ? orderB->m_order : -1;

                    return valA < valB;
                });

            // use only classes with valid order
            auto initialTest = cmdLine.singleValue("test");
            for (auto testClass  : testClasses)
            {
                auto shortTestName = testClass->name().view().afterLast("::").afterFirstOrFull("CanvasTest_");

                auto order  = testClass->findMetadata<CanvasTestOrderMetadata >();
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

            return true;
        }

        bool CanvasTestProject::createRenderingOutput()
        {
            // determine viewport resolution for the test
            // NOTE: the tests may have hard coded placement so this resolution should not be changed that much
            uint32_t viewportWidth = 1024;
            uint32_t viewportHeight = 1024;
            TRACE_INFO("Test viewport size: {}x{}", viewportWidth, viewportHeight);

            rendering::OutputInitInfo setup;
            setup.m_width = viewportWidth;
            setup.m_height = viewportHeight;
            setup.m_windowTitle = "Boomer Engine Canvas Tests";
			setup.m_windowAllowFullscreenToggle = true;
            setup.m_class = OutputClass::Window;

            // create rendering output
            m_renderingOutput = base::GetService<DeviceService>()->device()->createOutput(setup);
            if (!m_renderingOutput)
            {
                TRACE_ERROR("Failed to acquire window factory, no window can be created");
                return false;
            }

            // get window
            if (!m_renderingOutput->window())
            {
                TRACE_ERROR("Rendering output has no valid window");
                return false;
            }

            return true;
        }

        base::RefPtr<ICanvasTest> CanvasTestProject::initializeTest(uint32_t testIndex)
        {
            auto& testClass = m_testClasses[testIndex];

            // initialize the test
            auto testCase = testClass.m_testClass->create<ICanvasTest>();
            if (!testCase || !testCase->processInitialization())
            {
                TRACE_ERROR("Failed to initialize test '{}'", testClass.m_testClass->name());
                return nullptr;
            }

            return testCase;
        }

        void CanvasTestProject::update()
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

            // pump window messages
            //m_windowFactory->pumpMessages();

            // get input
            //auto& inputBuffer = base::GetService<base::input::InputService>()->events();

            // process events
            if (auto inputContext = m_renderingOutput->window()->windowGetInputContext())
            {
                while (auto evt = inputContext->pull())
                {
                    if (auto keyEvt  = evt->toKeyEvent())
                    {
                        if (keyEvt->pressed())
                        {
                            if (keyEvt->keyCode() == base::input::KeyCode::KEY_ESCAPE)
                            {
                                m_exitRequested = true;
                                break;
                            }
                            else if (keyEvt->keyCode() == base::input::KeyCode::KEY_LEFT)
                            {
                                if (m_pendingTestCaseIndex <= 0)
                                    m_pendingTestCaseIndex = m_testClasses.size() - 1;
                                else
                                    m_pendingTestCaseIndex -= 1;
                            }
                            else if (keyEvt->keyCode() == base::input::KeyCode::KEY_RIGHT)
                            {
                                m_pendingTestCaseIndex += 1;
                                if (m_pendingTestCaseIndex >= (int)m_testClasses.size())
                                    m_pendingTestCaseIndex = 0;
                            }
                        }
                    }

                    if (m_currentTest)
                        m_currentTest->processInput(*evt);
                }
            }

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
                m_renderingOutput->window()->windowSetTitle(base::TempString("Boomer Engine Canvas Tests - {} {}", testName, m_currentTest ? "" : "(FAILED TO INITIALIZE)"));
            }

            // render canvas on GPU
            // allocate command buffer
            {
                command::CommandWriter cmd("Test");

                if (const auto output = cmd.opAcquireOutput(m_renderingOutput))
                {
                    // use canvas of the same size as output window
                    base::canvas::Canvas canvas(output.width, output.height);

                    // render to canvas
                    if (m_currentTest)
                        m_currentTest->render(canvas);

                    // set frame buffer
                    FrameBuffer fb;
                    fb.color[0].view(output.color).clear(0.2, 0.2f, 0.2f, 1.0f);
                    fb.depth.view(output.depth).clearDepth().clearStencil();
                    cmd.opBeingPass(fb);

                    // render canvas to command buffer
                    canvas::CanvasRenderingParams renderingParams;
                    renderingParams.frameBufferWidth = output.width;
                    renderingParams.frameBufferHeight = output.height;
                    base::GetService<CanvasService>()->render(cmd, canvas, renderingParams);

                    // finish pass
                    cmd.opEndPass();

                    // swap
                    cmd.opSwapOutput(m_renderingOutput);
                }

                // submit work for rendering on GPU
                base::GetService<DeviceService>()->device()->submitWork(cmd.release());
            }
        }

    } // test
} // rendering

base::app::IApplication& GetApplicationInstance()
{
    static rendering::test::CanvasTestProject theApp;
    return theApp;
}

