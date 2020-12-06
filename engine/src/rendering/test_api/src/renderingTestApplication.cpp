/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "renderingTest.h"

#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingOutput.h"
#include "rendering/device/include/renderingDeviceService.h"

#include "base/app/include/launcherPlatform.h"
#include "base/app/include/commandline.h"
#include "base/app/include/localServiceContainer.h"
#include "base/app/include/application.h"
#include "base/io/include/ioSystem.h"
#include "base/input/include/inputContext.h"
#include "base/input/include/inputStructures.h"
#include "base/system/include/thread.h"
#include "base/script/include/scriptCompiledProject.h"

namespace rendering
{
    namespace test
    {
        //---

        // command to spawn a test window with basic rendering/GPU testes
        // running this command with different rendering drivers and on different platforms should produce the same results which can be easily and automatically verified (by taking screenshots)
        class TestRenderingFramework : public base::app::IApplication
        {
        public:
            TestRenderingFramework();
            virtual ~TestRenderingFramework();

            virtual bool initialize(const base::app::CommandLine& commandline) override final;
            virtual void update() override final;
            virtual void cleanup() override final;

        private:
            base::RefPtr<IRenderingTest> initializeTest(uint32_t testIndex);

            base::StringBuf m_assetsPath;

            // current test case
            int m_currentTestCaseIndex;
            int m_pendingTestCaseIndex;
            base::RefPtr<IRenderingTest> m_currentTestCase;

            // timing
            double m_timeCounter = 0.0f;
            base::NativeTimePoint m_lastFrameTime;
            double m_advanceTimeSpeed = 1.0f;
            bool m_advanceTime = true;

            // render the test case
            OutputObjectPtr m_renderingOutput;
            bool m_exitRequested;

            bool createRenderingOutput();


            // syncing
            base::NativeTimePoint m_frameSubmissionTime;

            // tests
            struct TestCase
            {
                base::ClassType m_testClass = nullptr;
                uint32_t m_subTestIndex = 0;
                base::StringBuf m_name;
            };

            base::Array<TestCase> m_testClasses;
        };

        //---

        TestRenderingFramework::TestRenderingFramework()
            : m_exitRequested(false)
            , m_currentTestCaseIndex(-1)
            , m_pendingTestCaseIndex(0)
        {}

        TestRenderingFramework::~TestRenderingFramework()
        {
        }

        void TestRenderingFramework::cleanup()
        {
            // shut down the test
            if (m_currentTestCase)
            {
                m_currentTestCase->shutdown();
                m_currentTestCase.reset();
            }

            // release output
            m_renderingOutput.reset();
        }

        base::RefPtr<IRenderingTest> TestRenderingFramework::initializeTest(uint32_t testIndex)
        {
            auto& testClass = m_testClasses[testIndex];

            // get device
            if (auto device = base::GetService<DeviceService>()->device())
            {
                // initialize the test
                auto testCase = testClass.m_testClass->create<IRenderingTest>();
                if (testCase && testCase->prepareAndInitialize(device, testClass.m_subTestIndex, m_renderingOutput))
                    return testCase;

                TRACE_ERROR("Failed to initialize test '{}', sub test {}", testClass.m_testClass->name(), testClass.m_subTestIndex);
                if (testCase)
                    testCase->shutdown();
            }

            return nullptr;
        }

        bool TestRenderingFramework::initialize(const base::app::CommandLine& cmdLine)
        {
            // list all test classes
            base::InplaceArray<base::ClassType, 100> testClasses;
            RTTI::GetInstance().enumClasses(IRenderingTest::GetStaticClass(), testClasses);
            TRACE_INFO("Found {} test classes", testClasses.size());

            // sort test classes by the order
            std::sort(testClasses.begin(), testClasses.end(), [](base::ClassType a, base::ClassType b)
                {
                    auto orderA  = a->findMetadata<RenderingTestOrderMetadata>();
                    auto orderB  = b->findMetadata<RenderingTestOrderMetadata>();

                    auto valA = orderA ? orderA->m_order : -1;
                    auto valB = orderB ? orderB->m_order : -1;

                    return valA < valB;
                });

            // use only classes with valid order
            auto initialTest = cmdLine.singleValue("test");
            for (auto testClass  : testClasses)
            {
                auto shortTestName = testClass->name().view().afterLast("::").afterFirstOrFull("RenderingTest_");

                auto order  = testClass->findMetadata<RenderingTestOrderMetadata>();
                if (!order || order->m_order < 0)
                {
                    if (initialTest != shortTestName)
                    {
                        TRACE_WARNING("Test class '{}' has no order specified, ignoring", testClass->name());
                        continue;
                    }
                }

                auto countInfo  = testClass->findMetadata<RenderingTestSubtestCountMetadata>();
                auto subtestCount = countInfo ? countInfo->m_count : 1;

                for (uint32_t i = 0; i < subtestCount; ++i)
                {
                    auto& testCase = m_testClasses.emplaceBack();
                    testCase.m_testClass = testClass;
                    testCase.m_subTestIndex = i;
                    testCase.m_name = base::StringBuf(shortTestName);
                }
            }

            // nothing to test
            TRACE_INFO("Found {} valid test cases", m_testClasses.size());
            if (m_testClasses.empty())
                return 0;

            // print tests (for reference)
            for (uint32_t i=0; i<m_testClasses.size(); ++i)
            {
                if (initialTest == m_testClasses[i].m_name)
                {
                    m_pendingTestCaseIndex = i;
                }
            }

            // setup rendering output
            if (!createRenderingOutput())
                return false;

            return true;
        }

        bool TestRenderingFramework::createRenderingOutput()
        {
            // determine viewport resolution for the test
            // NOTE: the tests may have hard coded placement so this resolution should not be changed that much
            uint32_t viewportWidth = 1024;
            uint32_t viewportHeight = 1024;
            TRACE_INFO("Test viewport size: {}x{}", viewportWidth, viewportHeight);

            rendering::OutputInitInfo setup;
            setup.m_width = viewportWidth;
            setup.m_height = viewportHeight;
            setup.m_windowTitle = "Boomer Engine Rendering Tests";
            setup.m_class = OutputClass::Window;

            // create rendering output
            m_renderingOutput = base::GetService<DeviceService>()->device()->createOutput(setup);
            if (!m_renderingOutput || !m_renderingOutput->window())
            {
                TRACE_ERROR("Failed to acquire window factory, no window can be created");
                return false;
            }
            
            return true;
        }

        void TestRenderingFramework::update()
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
                            else if (keyEvt->keyCode() == base::input::KeyCode::KEY_SPACE)
                            {
                                m_advanceTime = !m_advanceTime;
                            }
                            else if (keyEvt->keyCode() == base::input::KeyCode::KEY_MINUS)
                            {
                                m_advanceTimeSpeed *= 0.5f;
                            }
                            else if (keyEvt->keyCode() == base::input::KeyCode::KEY_EQUAL)
                            {
                                m_advanceTimeSpeed *= 2.0f;
                            }

                            else if (keyEvt->keyCode() == base::input::KeyCode::KEY_R)
                            {
                                m_advanceTimeSpeed = 1.0f;
                                m_advanceTime = true;
                                m_timeCounter = 0.0f;
                            }
                        }
                    }
                }
            }

            // navigate to other test
            if (m_pendingTestCaseIndex != m_currentTestCaseIndex)
            {
                // close current test
                if (m_currentTestCase)
                {
                    m_currentTestCase->shutdown();
                    m_currentTestCase.reset();
                }

                // initialize new test
                const auto prevTestClass = (m_currentTestCaseIndex != -1) ? m_testClasses[m_currentTestCaseIndex].m_testClass : nullptr;
                m_currentTestCaseIndex = m_pendingTestCaseIndex;
                m_currentTestCase = initializeTest(m_currentTestCaseIndex);

                // should we reset time
                if (prevTestClass != m_testClasses[m_currentTestCaseIndex].m_testClass)
                {
                    m_lastFrameTime.resetToNow();
                    m_timeCounter = 0.0;
                }

                // set the window caption
                auto testName = m_testClasses[m_currentTestCaseIndex].m_name;
                auto testIndex = m_testClasses[m_currentTestCaseIndex].m_subTestIndex;
                m_renderingOutput->window()->windowSetTitle(base::TempString("Boomer Engine Rendering Tests - {} ({}) {}", testName, testIndex, m_currentTestCase ? "" : "(FAILED TO INITIALIZE)"));
            }

            {
                command::CommandWriter cmd("Test");

                // acquire the back buffer
                if (auto output = cmd.opAcquireOutput(m_renderingOutput))
                {
                    // render the test
                    if (m_currentTestCase)
                        m_currentTestCase->render(cmd, (float)m_timeCounter, output.color, output.depth);

                    // swap
                    cmd.opSwapOutput(m_renderingOutput);
                }

                // accumulate time
                const auto dt = m_lastFrameTime.timeTillNow().toSeconds();
                if (m_advanceTime)
                    m_timeCounter += dt * m_advanceTimeSpeed;
                m_lastFrameTime.resetToNow();

                // create new fence
                m_frameSubmissionTime = base::NativeTimePoint::Now();

                // submit new work
                base::GetService<DeviceService>()->device()->submitWork(cmd.release());
            }
        }

        //---

    } // test
} // rendering

base::app::IApplication& GetApplicationInstance()
{
    static rendering::test::TestRenderingFramework theApp;
    return theApp;
}