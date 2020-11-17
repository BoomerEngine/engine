/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: viewer #]
***/

#include "build.h"
#include "modelViewerApp.h"

#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingOutput.h"
#include "rendering/runtime/include/renderingRuntimeService.h"

#include "base/app/include/launcherPlatform.h"
#include "base/input/include/inputContext.h"
#include "base/input/include/inputStructures.h"
#include "rendering/driver/include/renderingCommandBuffer.h"
#include "rendering/driver/include/renderingCommandWriter.h"

namespace viewer
{
    //---

    base::ConfigProperty<uint32_t> cvModelViewerWindowWidth("ModelViewer.Window", "Width", 1920);
    base::ConfigProperty<uint32_t> cvModelViewerWindowHeight("ModelViewer.Window", "Height", 1080);
    base::ConfigProperty<bool> cvModelViewerWindowMaximized("ModelViewer.Window", "Maximized", false);

    //---

    ModelViewerApp::ModelViewerApp()
    {
    }

    ModelViewerApp::~ModelViewerApp()
    {
        if (m_imgui)
        {
            ImGui::DestroyContext(m_imgui);
            m_imgui = nullptr;
        }

        if (m_renderingOutput)
        {
            m_renderingOutput->close();
            m_renderingOutput.reset();
        }
    }

    bool ModelViewerApp::initialize(const base::app::CommandLine& commandline)
    {
        if (!createWindow())
            return false;

        m_imgui = ImGui::CreateContext();

        return true;
    }

    bool ModelViewerApp::createWindow()
    {
        // we need the rendering service for any of stuff to work, get the current instance of the rendering service from local service container
        auto renderingService  = base::GetService<rendering::runtime::RuntimeService>();
        if (!renderingService)
        {
            TRACE_ERROR("No rendering service running (trying to run in a windowless environment?)");
            return false;
        }

        // setup rendering output as a window
        rendering::DriverOutputInitInfo setup;
        setup.m_width = cvModelViewerWindowWidth.get();
        setup.m_height = cvModelViewerWindowHeight.get();
        setup.m_windowMaximized = cvModelViewerWindowMaximized.get();
        setup.m_windowTitle = "BoomerEngine - ModelViewer";
        setup.m_class = rendering::DriverOutputClass::NativeWindow;

        // create rendering output
        m_renderingOutput = renderingService->driver()->createOutput(setup);
        if (!m_renderingOutput)
        {
            TRACE_ERROR("Failed to acquire window factory, no window can be created");
            return false;
        }

        return true;
    }

    void ModelViewerApp::update()
    {
        ImGui::SetCurrentContext(m_imgui);

        // measure dt from last frame
        const auto dt = std::clamp(m_lastTickTime.timeTillNow().toSeconds(), 0.0001, 0.1);
        m_lastTickTime.resetToNow();

        // update ImGui DT
        ImGui::GetIO().DeltaTime = dt;

        // render stuff
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
            }

            // render the frame
            renderFrame();
        }

        ImGui::SetCurrentContext(nullptr);
    }

    void ModelViewerApp::processInput(const base::input::BaseEvent& evt)
    {
        if (ImGui::ProcessInputEvent(evt))
            return;

        if (auto keyEvent  = evt.toKeyEvent())
        {
            if (keyEvent->pressed())
            {
                switch (keyEvent->keyCode())
                {
                case base::input::KeyCode::KEY_ESCAPE:
                    base::platform::GetLaunchPlatform().requestExit("ESC key pressed");
                    break;
                }
            }
        }


    }
        
    void ModelViewerApp::renderGui(base::canvas::Canvas& c)
    {
        ImGui::BeginCanvasFrame(c);

        static bool open = false;
        ImGui::ShowDemoWindow(&open);

        ImGui::EndCanvasFrame(c);
    }

    void ModelViewerApp::renderFrame()
    {
        // create output frame, this will fail if output is not valid
        if (auto frameOutput = m_renderingOutput->prepareForFrameRendering())
        {
            // create canvas payload to render gui into
            auto canvasPayload = base::RefNew<rendering::canvas::CanvasFramePayload>(frameOutput->outputSize().x, frameOutput->outputSize().y);
            canvasPayload->canvasCollector().clearColor(Color(40, 40, 40, 255));
            renderGui(canvasPayload->canvasCollector());

            // create the frame payloads
            rendering::runtime::FrameContent content[2];
            content[0].payload = canvasPayload;

            // dispatch the frame
            base::GetService<rendering::runtime::RuntimeService>()->renderFrame(frameOutput, content, 1);
        }
    }
    
} // viewer

base::app::IApplication& GetApplicationInstance()
{
    static viewer::ModelViewerApp theApp;
    return theApp;
}
