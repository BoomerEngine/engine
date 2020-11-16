/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/app/include/application.h"

namespace rendering
{
    namespace test
    {
        class ICanvasTest;
        
        /// boilerplate for rendering scene test
        /// contains basic scene initialization and other shit
        class CanvasTestProject : public base::app::IApplication
        {
        public:
            CanvasTestProject();

        protected:
            virtual bool initialize(const base::app::CommandLine& commandline) override;
            virtual void cleanup() override;
            virtual void update() override;

            //--

            base::RefPtr<ICanvasTest> m_currentTest;
            int m_currentTestCaseIndex;
            int m_pendingTestCaseIndex;

            ObjectID m_renderingOutput;
            IDriverNativeWindowInterface* m_renderingWindow = nullptr;
            bool m_exitRequested;

            bool createRenderingOutput();

            //--

            struct TestInfo
            {
                base::StringBuf m_testName;
                base::ClassType m_testClass = nullptr;
            };

            // test classes
            base::Array<TestInfo> m_testClasses;

            //--

            base::RefPtr<ICanvasTest> initializeTest(uint32_t testIndex);
        };      

    } // test
} // scene