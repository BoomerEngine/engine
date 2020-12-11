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
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/canvas/include/renderingCanvasRenderer.h"
#include "rendering/canvas/include/renderingCanvasStorage.h"
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
			if (m_currentTest)
			{
				m_currentTest->shutdown();
				m_currentTest.reset();
			}

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

			// create the canvas storage
			auto dev = base::GetService<DeviceService>()->device();
			m_storage = base::RefNew<rendering::canvas::CanvasStorage>(dev);

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
            if (!testCase)
            {
                TRACE_ERROR("Failed to initialize test '{}'", testClass.m_testClass->name());
                return nullptr;
            }

			testCase->m_storage = m_storage;

			if (!testCase->processInitialization())
			{
				TRACE_ERROR("Failed to initialize test '{}'", testClass.m_testClass->name());
				testCase->shutdown();
				return nullptr;
			}

            return testCase;
        }

		void CanvasTestProject::updateTitleBar()
		{
			const auto scale = (100 + m_pixelScaleTest) / 100.0f;

			// set the window caption
			auto testName = m_testClasses[m_currentTestCaseIndex].m_testName;
			m_renderingOutput->window()->windowSetTitle(base::TempString("Boomer Engine Canvas Tests - {} x{} {}", 
				testName, Prec(scale, 2), m_currentTest ? "" : "(FAILED TO INITIALIZE)"));
		}

		void CanvasTestProject::processInput()
		{
			if (auto inputContext = m_renderingOutput->window()->windowGetInputContext())
			{
				while (auto evt = inputContext->pull())
				{
					if (auto keyEvt = evt->toKeyEvent())
					{
						if (keyEvt->pressedOrRepeated())
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
							else if (keyEvt->keyCode() == base::input::KeyCode::KEY_EQUAL)
							{
								if (keyEvt->keyMask().isShiftDown())
									m_pixelScaleTest += 1;
								else
									m_pixelScaleTest += 10;
								updateTitleBar();
							}
							else if (keyEvt->keyCode() == base::input::KeyCode::KEY_MINUS)
							{
								if (keyEvt->keyMask().isShiftDown())
									m_pixelScaleTest -= 1;
								else
									m_pixelScaleTest -= 10;

								if (m_pixelScaleTest < -99)
									m_pixelScaleTest = 99;

								updateTitleBar();
							}
						}
					}

					if (m_currentTest)
						m_currentTest->processInput(*evt);
				}
			}
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

			// update input
			processInput();

            // navigate to other test
            if (m_pendingTestCaseIndex != m_currentTestCaseIndex)
            {
                // close current test
				if (m_currentTest)
				{
					m_currentTest->shutdown();
					m_currentTest.reset();
				}

                // initialize new test
                m_currentTestCaseIndex = m_pendingTestCaseIndex;
                m_currentTest = initializeTest(m_currentTestCaseIndex);

				updateTitleBar();
            }

            // render canvas on GPU
            // allocate command buffer
            {
                command::CommandWriter cmd("Test");

                if (const auto output = cmd.opAcquireOutput(m_renderingOutput))
                {
					// clear frame buffer
					{
						FrameBuffer fb;
						fb.color[0].view(output.color).clear(0.2, 0.2f, 0.2f, 1.0f);
						fb.depth.view(output.depth).clearDepth().clearStencil();
						cmd.opBeingPass(m_renderingOutput->layout(), fb);
						cmd.opEndPass();
					}

					// create canvas renderer
					rendering::canvas::CanvasRenderer::Setup setup;
					setup.width = output.width;
					setup.height = output.height;
					setup.backBufferColorRTV = output.color;
					setup.backBufferDepthRTV = output.depth;
					setup.backBufferLayout = m_renderingOutput->layout();
					setup.pixelScale = (100 + m_pixelScaleTest) / 100.0f;

					// render test to canvas
					{
						rendering::canvas::CanvasRenderer canvas(setup, m_storage);

						if (m_currentTest)
							m_currentTest->render(canvas);

						cmd.opAttachChildCommandBuffer(canvas.finishRecording());
					}


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

