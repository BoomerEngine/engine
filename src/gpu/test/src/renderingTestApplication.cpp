/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "renderingTest.h"

#include "gpu/device/include/commandBuffer.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/output.h"
#include "gpu/device/include/deviceService.h"

#include "core/app/include/launcherPlatform.h"
#include "core/app/include/commandline.h"
#include "core/app/include/localServiceContainer.h"
#include "core/app/include/application.h"
#include "core/io/include/io.h"
#include "core/input/include/inputContext.h"
#include "core/input/include/inputStructures.h"
#include "core/system/include/thread.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

//---

enum class CameraButtonBit : uint8_t
{
	Forward,
	Backward,
	Left,
	Right,
	Up,
	Down,
	Fast,
	Slow
};

typedef BitFlags<CameraButtonBit> CameraButtonMask;

//---

// command to spawn a test window with basic rendering/GPU testes
// running this command with different rendering drivers and on different platforms should produce the same results which can be easily and automatically verified (by taking screenshots)
class TestRenderingFramework : public IApplication
{
public:
    TestRenderingFramework();
    virtual ~TestRenderingFramework();

    virtual bool initialize(const CommandLine& commandline) override final;
    virtual void update() override final;
    virtual void cleanup() override final;

private:
    RefPtr<IRenderingTest> initializeTest(uint32_t testIndex);

    StringBuf m_assetsPath;

    // current test case
    int m_currentTestCaseIndex;
    int m_pendingTestCaseIndex;
    RefPtr<IRenderingTest> m_currentTestCase;

    // timing
    double m_timeCounter = 0.0f;
    NativeTimePoint m_lastFrameTime;
    double m_advanceTimeSpeed = 1.0f;
    bool m_advanceTime = true;

	// camera
	Vector3 m_cameraPosition;
	Vector2 m_cameraViewDeltas;
	Angles m_cameraAngles;
	CameraButtonMask m_cameraButtons;

    // render the test case
    OutputObjectPtr m_renderingOutput;
    bool m_exitRequested;

    bool createRenderingOutput();
	void updateTitleBar();
	void processInput();
	void processCamera(float dt);


    // syncing
    NativeTimePoint m_frameSubmissionTime;

    // tests
    struct TestCase
    {
        ClassType m_testClass = nullptr;
        uint32_t m_subTestIndex = 0;
        StringBuf m_name;
    };

    Array<TestCase> m_testClasses;
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

RefPtr<IRenderingTest> TestRenderingFramework::initializeTest(uint32_t testIndex)
{
    auto& testClass = m_testClasses[testIndex];

    // get device
    if (auto device = GetService<DeviceService>()->device())
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

bool TestRenderingFramework::initialize(const CommandLine& cmdLine)
{
    // list all test classes
    InplaceArray<ClassType, 100> testClasses;
    RTTI::GetInstance().enumClasses(IRenderingTest::GetStaticClass(), testClasses);
    TRACE_INFO("Found {} test classes", testClasses.size());

    // sort test classes by the order
    std::sort(testClasses.begin(), testClasses.end(), [](ClassType a, ClassType b)
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
            testCase.m_name = StringBuf(shortTestName);
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

    gpu::OutputInitInfo setup;
    setup.m_width = viewportWidth;
    setup.m_height = viewportHeight;
    setup.m_windowTitle = "Boomer Engine Rendering Tests";
    setup.m_class = OutputClass::Window;

    // create rendering output
    m_renderingOutput = GetService<DeviceService>()->device()->createOutput(setup);
    if (!m_renderingOutput || !m_renderingOutput->window())
    {
        TRACE_ERROR("Failed to acquire window factory, no window can be created");
        return false;
    }
            
    return true;
}

void TestRenderingFramework::processInput()
{
	if (auto inputContext = m_renderingOutput->window()->windowGetInputContext())
	{
		while (auto evt = inputContext->pull())
		{
			if (auto keyEvt = evt->toKeyEvent())
			{
				if (keyEvt->pressed())
				{
					if (keyEvt->keyCode() == input::KeyCode::KEY_ESCAPE)
					{
						m_exitRequested = true;
						break;
					}
					else if (keyEvt->keyCode() == input::KeyCode::KEY_LEFT)
					{
						if (m_pendingTestCaseIndex <= 0)
							m_pendingTestCaseIndex = m_testClasses.size() - 1;
						else
							m_pendingTestCaseIndex -= 1;
					}
					else if (keyEvt->keyCode() == input::KeyCode::KEY_RIGHT)
					{
						m_pendingTestCaseIndex += 1;
						if (m_pendingTestCaseIndex >= (int)m_testClasses.size())
							m_pendingTestCaseIndex = 0;
					}
					else if (keyEvt->keyCode() == input::KeyCode::KEY_SPACE)
					{
						m_advanceTime = !m_advanceTime;
					}
					else if (keyEvt->keyCode() == input::KeyCode::KEY_MINUS)
					{
						m_advanceTimeSpeed *= 0.5f;
					}
					else if (keyEvt->keyCode() == input::KeyCode::KEY_EQUAL)
					{
						m_advanceTimeSpeed *= 2.0f;
					}

					else if (keyEvt->keyCode() == input::KeyCode::KEY_R)
					{
						m_advanceTimeSpeed = 1.0f;
						m_advanceTime = true;
						m_timeCounter = 0.0f;
					}
				}

				if (keyEvt->keyCode() == input::KeyCode::KEY_W)
					m_cameraButtons.configure(CameraButtonBit::Forward, keyEvt->isDown());
				else if (keyEvt->keyCode() == input::KeyCode::KEY_S)
					m_cameraButtons.configure(CameraButtonBit::Backward, keyEvt->isDown());
				else if (keyEvt->keyCode() == input::KeyCode::KEY_A)
					m_cameraButtons.configure(CameraButtonBit::Left, keyEvt->isDown());
				else if (keyEvt->keyCode() == input::KeyCode::KEY_D)
					m_cameraButtons.configure(CameraButtonBit::Right, keyEvt->isDown());
				else if (keyEvt->keyCode() == input::KeyCode::KEY_Q)
					m_cameraButtons.configure(CameraButtonBit::Up, keyEvt->isDown());
				else if (keyEvt->keyCode() == input::KeyCode::KEY_E)
					m_cameraButtons.configure(CameraButtonBit::Down, keyEvt->isDown());
				else if (keyEvt->keyCode() == input::KeyCode::KEY_LEFT_SHIFT)
					m_cameraButtons.configure(CameraButtonBit::Fast, keyEvt->isDown());
				else if (keyEvt->keyCode() == input::KeyCode::KEY_LEFT_CTRL)
					m_cameraButtons.configure(CameraButtonBit::Slow, keyEvt->isDown());
			}
			else if (auto keyEvt = evt->toAxisEvent())
			{
				if (keyEvt->axisCode() == input::AxisCode::AXIS_MOUSEX)
					m_cameraViewDeltas.x += keyEvt->displacement();
				else if (keyEvt->axisCode() == input::AxisCode::AXIS_MOUSEY)
					m_cameraViewDeltas.y += keyEvt->displacement();
			}
		}
	}
}

void TestRenderingFramework::processCamera(float dt)
{
	Vector3 moveFactors;

	moveFactors.x += m_cameraButtons.test(CameraButtonBit::Forward) ? 1.0f : 0.0f;
	moveFactors.x += m_cameraButtons.test(CameraButtonBit::Backward) ? -1.0f : 0.0f;
	moveFactors.y += m_cameraButtons.test(CameraButtonBit::Left) ? -1.0f : 0.0f;
	moveFactors.y += m_cameraButtons.test(CameraButtonBit::Right) ? 1.0f : 0.0f;
	moveFactors.z += m_cameraButtons.test(CameraButtonBit::Up) ? 1.0f : 0.0f;
	moveFactors.z += m_cameraButtons.test(CameraButtonBit::Down) ? -1.0f : 0.0f;

	m_cameraAngles.pitch = std::clamp<float>(m_cameraAngles.pitch + m_cameraViewDeltas.y * 0.2f, -90.0f, 90.0f);
	m_cameraAngles.yaw = m_cameraAngles.yaw + m_cameraViewDeltas.x * 0.2f;
	m_cameraViewDeltas = Vector2::ZERO();

	if (!moveFactors.isNearZero())
	{
		moveFactors.normalize();

		float speed = 3.0f;
		if (m_cameraButtons.test(CameraButtonBit::Fast))
			speed *= 2.0f;
		if (m_cameraButtons.test(CameraButtonBit::Slow))
			speed /= 4.0f;

		Vector3 forward, right, up;
		m_cameraAngles.angleVectors(forward, right, up);				

		m_cameraPosition += forward * speed * dt * moveFactors.x;
		m_cameraPosition += right * speed * dt * moveFactors.y;
		m_cameraPosition.z += speed * dt * moveFactors.z;
	}
}

void TestRenderingFramework::updateTitleBar()
{
	auto testName = m_testClasses[m_currentTestCaseIndex].m_name;

	StringBuilder txt;
	txt.appendf("Rendering Tests - {} ", testName);

	if (m_currentTestCase)
	{
		txt << "(";
		m_currentTestCase->describeSubtest(txt);
		txt << ")";
	}
	else
	{
		auto testIndex = m_testClasses[m_currentTestCaseIndex].m_subTestIndex;
		txt.appendf("(SubTest{} - FAILED TO INITIALIZE)", testIndex);
	}

	m_renderingOutput->window()->windowSetTitle(txt.toString());
}

void TestRenderingFramework::update()
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

	// calculate real time delta
	const auto dt = m_lastFrameTime.timeTillNow().toSeconds();
	m_lastFrameTime.resetToNow();
			
	// process user input
	processInput();

	// move camera
	processCamera(dt);

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
		const bool resetTiming = prevTestClass != m_testClasses[m_currentTestCaseIndex].m_testClass;
        if (resetTiming)
        {
			if (m_currentTestCase)
				m_currentTestCase->queryInitialCamera(m_cameraPosition, m_cameraAngles);

            m_lastFrameTime.resetToNow();
            m_timeCounter = 0.0;
        }

		updateTitleBar();
    }

    {
        CommandWriter cmd("Test");

        // acquire the back buffer
        if (auto output = cmd.opAcquireOutput(m_renderingOutput))
        {
            // render the test
			if (m_currentTestCase)
			{
				m_currentTestCase->updateCamera(m_cameraPosition, m_cameraAngles);
				m_currentTestCase->render(cmd, (float)m_timeCounter, output.color, output.depth);
			}

            // swap
            cmd.opSwapOutput(m_renderingOutput);
        }

        // accumulate time
        if (m_advanceTime)
            m_timeCounter += dt * m_advanceTimeSpeed;
				
        // create new fence
        m_frameSubmissionTime = NativeTimePoint::Now();

        // submit new work
        GetService<DeviceService>()->device()->submitWork(cmd.release());
    }
}

//---

END_BOOMER_NAMESPACE_EX(gpu::test)

boomer::IApplication& GetApplicationInstance()
{
    static boomer::gpu::test::TestRenderingFramework theApp;
    return theApp;
}