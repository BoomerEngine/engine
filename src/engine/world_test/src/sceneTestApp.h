/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "core/app/include/application.h"

#include "engine/rendering/include/stats.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

class ISceneTest;
        
/// boilerplate for rendering scene test
/// contains basic scene initialization and other shit
class SceneTestProject : public app::IApplication
{
public:
    SceneTestProject();

protected:
    virtual bool initialize(const app::CommandLine& commandline) override;
    virtual void update() override;
    virtual void cleanup() override;

    void handleTick(float dt);
    void handleInput(input::IContext& context);
    bool handleInputEvent(const input::BaseEvent& evt);
    bool handleAppInputEvent(const input::BaseEvent& evt);

    void prepareSceneCommandBuffers(gpu::CommandWriter& cmd, const rendering::FrameCompositionTarget& target);
    void prepareCanvasCommandBuffers(gpu::CommandWriter& cmd, const rendering::FrameCompositionTarget& target);
    void renderCanvas(canvas::Canvas& c);

    //--

	ImGui::ImGUICanvasHelper* m_imguiHelper = nullptr;

    //--

    NativeTimePoint m_lastUpdateTime;
    double m_lastGameTime;
    int m_timeMultiplier = 0;
    bool m_timeAdvance = true;

    //--

    RefPtr<ISceneTest> m_currentTest;
    int m_currentTestCaseIndex;
    int m_pendingTestCaseIndex;

    gpu::OutputObjectPtr m_renderingOutput;
    bool m_exitRequested;

    rendering::CameraContextPtr m_cameraContext;
    Camera m_lastCamera;

    rendering::FrameStats m_lastFrameStats;
    rendering::SceneStats m_lastSceneStats;

    gpu::ImageObjectPtr m_flippedColorTarget;
    gpu::RenderTargetViewPtr m_flippedColorTargetRTV;

    gpu::ImageObjectPtr m_flippedDepthTarget;
    gpu::RenderTargetViewPtr m_flippedDepthTargetRTV;

    bool createRenderingOutput();
    bool shouldCaptureInput();

    //--

    struct TestInfo
    {
        StringBuf m_testName;
        ClassType m_testClass = nullptr;
    };

    // test classes
    Array<TestInfo> m_testClasses;

    //--

    RefPtr<ISceneTest> initializeTest(uint32_t testIndex);
};

END_BOOMER_NAMESPACE_EX(test)
