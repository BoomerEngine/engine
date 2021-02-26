/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "core/app/include/application.h"

BEGIN_BOOMER_NAMESPACE_EX(test)

class ICanvasTest;
        
/// boilerplate for rendering scene test
/// contains basic scene initialization and other shit
class CanvasTestProject : public app::IApplication
{
public:
    CanvasTestProject();

protected:
    virtual bool initialize(const app::CommandLine& commandline) override;
    virtual void cleanup() override;
    virtual void update() override;

    //--
			
    RefPtr<ICanvasTest> m_currentTest;
    int m_currentTestCaseIndex;
    int m_pendingTestCaseIndex;

    gpu::OutputObjectPtr m_renderingOutput;
    bool m_exitRequested;
	int m_pixelScaleTest = 0;

    bool createRenderingOutput();
	void updateTitleBar();
	void processInput();

    //--

    struct TestInfo
    {
        StringBuf m_testName;
        ClassType m_testClass = nullptr;
    };

    // test classes
    Array<TestInfo> m_testClasses;

    //--

    RefPtr<ICanvasTest> initializeTest(uint32_t testIndex);
};

END_BOOMER_NAMESPACE_EX(test)
