/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#include "build.h"
#include "exampleApp.h"
#include "exampleAppHelpers.h"

namespace example
{
    //---

    SimpleApp::SimpleApp()
        : m_cameraRotation(-20.0f, 0.0f, 0.0f)
        , m_cameraDistance(10.0f)
    {
        memset(m_cameraKeys, 0, sizeof(m_cameraKeys));
    }

    bool SimpleApp::initialize(const CommandLine& commandline)
    {
        if (!createWindow())
            return false;

        if (!compileShaders())
            return false;

        if (!buildGeometry())
            return false;

        return true;
    }

    void SimpleApp::cleanup()
    {
        if (m_indexBuffer)
        {
            GetService<DeviceService>()->device()->releaseObject(m_indexBuffer.id());
            m_indexBuffer = BufferView();
        }

        if (m_vertexBuffer)
        {
            GetService<DeviceService>()->device()->releaseObject(m_vertexBuffer.id());
            m_vertexBuffer = BufferView();
        }

        if (m_renderingOutput)
        {
            // rendering output must always be explicitly closed
            base::GetService<DeviceService>()->device()->releaseObject(m_renderingOutput);
            m_renderingOutput = rendering::ObjectID();
        }
    }

    bool SimpleApp::createWindow()
    {
        // we need the rendering service for any of stuff to work, get the current instance of the rendering service from local service container
        auto renderingService  = GetService<DeviceService>();
        if (!renderingService)
        {
            TRACE_ERROR("No rendering service running (trying to run in a windowless environment?)");
            return false;
        }

        // setup rendering output as a window
        rendering::DriverOutputInitInfo setup;
        setup.m_width = 1024;
        setup.m_height = 1024;
        setup.m_windowMaximized = false;
        setup.m_windowTitle = "BoomerEngine - SimpleBox";
        setup.m_class = rendering::DriverOutputClass::NativeWindow; // render to native window on given OS

        // create rendering output
        m_renderingOutput = renderingService->device()->createOutput(setup);
        if (!m_renderingOutput)
        {
            TRACE_ERROR("Failed to acquire window factory, no window can be created");
            return false;
        }

        // get window
        m_renderingWindow = renderingService->device()->queryOutputWindow(m_renderingOutput);
        if (!m_renderingWindow)
        {
            TRACE_ERROR("Rendering output has no valid window");
            return false;
        }

        return true;
    }

    void SimpleApp::update()
    {
        // measure and accumulate the dt from last frame
        double dt = std::clamp(m_lastTickTime.timeTillNow().toSeconds(), 0.0001, 0.1);
        m_time += dt;
        m_lastTickTime.resetToNow();

        // update window stuff - read and process input
        updateWindow();

        // adjust camera position based on movement
        updateCamera(dt);

        // record frame content and send it for rendering
        renderFrame();
    }

    void SimpleApp::updateWindow()
    {
        // if the window wants to be closed allow it and close our app as well
        if (m_renderingWindow->windowHasCloseRequest())
            base::platform::GetLaunchPlatform().requestExit("Main window closed");

        // process input from the window
        if (auto inputContext = m_renderingWindow->windowGetInputContext())
        {
            while (auto inputEvent = inputContext->pull())
                processInput(*inputEvent);
        }
    }

    void SimpleApp::processInput(const BaseEvent& evt)
    {
        // is this a key event ?
        if (auto keyEvent  = evt.toKeyEvent())
        {
            // did we press the key ?
            if (keyEvent->pressed())
            {
                switch (keyEvent->keyCode())
                {
                    // Exit application when "ESC" is pressed
                    case KeyCode::KEY_ESCAPE:
                        platform::GetLaunchPlatform().requestExit("ESC key pressed");
                        break;

                    // Camera keys
                    case KeyCode::KEY_W:
                        m_cameraKeys[0] = true;
                        break;
                    case KeyCode::KEY_S:
                        m_cameraKeys[1] = true;
                        break;
                    case KeyCode::KEY_A:
                        m_cameraKeys[2] = true;
                        break;
                    case KeyCode::KEY_D:
                        m_cameraKeys[3] = true;
                        break;
                }
            }
            // did we release the key ?
            else if (keyEvent->released())
            {
                switch (keyEvent->keyCode())
                {
                    // Camera keys
                    case KeyCode::KEY_W:
                        m_cameraKeys[0] = false;
                        break;
                    case KeyCode::KEY_S:
                        m_cameraKeys[1] = false;
                        break;
                    case KeyCode::KEY_A:
                        m_cameraKeys[2] = false;
                        break;
                    case KeyCode::KEY_D:
                        m_cameraKeys[3] = false;
                        break;
                }
            }
        }
    }

    void SimpleApp::updateCamera(float dt)
    {
        float pitchCtrl = 0.0f;
        float yawCtrl = 0.0f;

        if (m_cameraKeys[0]) // W
            pitchCtrl -= 1.0f;
        if (m_cameraKeys[1]) // S
            pitchCtrl += 1.0f;

        if (m_cameraKeys[2]) // A
            yawCtrl += 1.0f;
        if (m_cameraKeys[3]) // D
            yawCtrl -= 1.0f;

        float rotationSpeedPerSecond = 90.0f;
        float rotationAmmount = dt * rotationSpeedPerSecond;

        m_cameraRotation.pitch = std::clamp(m_cameraRotation.pitch + rotationAmmount * pitchCtrl, -89.99f, 89.99f);
        m_cameraRotation.yaw = m_cameraRotation.yaw + rotationAmmount * yawCtrl;
    }
        
    void SimpleApp::renderFrame()
    {
        // create a command buffer writer and tell the "scene" to render to it
        CommandWriter cmd("CommandBuffer");

        base::Rect viewport;
        rendering::ImageView colorBackBuffer, depthBackBuffer;
        if (cmd.opAcquireOutput(m_renderingOutput, viewport, colorBackBuffer, &depthBackBuffer))
        {
            renderScene(cmd, colorBackBuffer, depthBackBuffer);
            cmd.opSwapOutput(m_renderingOutput);
        }

        // submit new work to rendering device
        base::GetService<DeviceService>()->device()->submitWork(cmd.release());
    }

    void SimpleApp::setupCamera(CommandWriter& cmd, const Vector3& from, const Vector3& to, float fov, float aspect)
    {
        // NOTE: this layout must match the shader
        struct CameraDescriptor
        {
            ConstantsView data;
        };

        // NOTE: this layout must match the shader
        struct CameraParams
        {
            Matrix WorldToScreen;
        };

        // calculate camera matrices
        helper::CameraMatrices camera;
        camera.calculate(from, to, fov, aspect); 

        CameraParams params;
        params.WorldToScreen = camera.WorldToScreen.transposed();

        // prepare descriptor
        CameraDescriptor desc;
        desc.data = cmd.opUploadConstants(params);

        // record and set descriptor
        cmd.opBindParametersInline("SceneParams"_id, desc);
    }

    void SimpleApp::setupObject(CommandWriter& cmd, const Matrix& localToWorld)
    {
        // NOTE: this layout must match the shader
        struct ObjectDescriptor
        {
            ConstantsView data;
        };

        // NOTE: this layout must match the shader
        struct ObjectParams
        {
            Matrix localToWorld;
        };

        ObjectParams params;
        params.localToWorld = localToWorld.transposed();

        ObjectDescriptor desc;
        desc.data = cmd.opUploadConstants(params); // copy the content of constant buffer onto the command buffer

        // record and set descriptor
        cmd.opBindParametersInline("ObjectParams"_id, desc);
    }

    void SimpleApp::renderScene(CommandWriter& cmd, const rendering::ImageView& colorTarget, const rendering::ImageView& depthTarget)
    {
        if (!m_shaders)
            return;

        // prepare frame buffer
        FrameBuffer fb;
        fb.color[0].view(colorTarget).clear(0.2f, 0.2f, 0.2f, 1.0f);
        fb.depth.view(depthTarget).clearDepth(1.0f).clearStencil();

        // setup constants for camera
        const auto cameraPos = m_cameraRotation.forward() * m_cameraDistance;
        const float aspect = colorTarget.width() / (float)colorTarget.height(); // NOTE: we never get to rendering phase if we have zero sized render targets
        setupCamera(cmd, cameraPos, Vector3(0, 0, 0), 70.0f, aspect);

        // end pass rendering to that frame buffer
        cmd.opBeingPass(fb);

        // bind buffers
        cmd.opBindVertexBuffer("BoxVertex"_id, m_vertexBuffer);
        cmd.opBindIndexBuffer(m_indexBuffer);

        // enable depth buffer
        cmd.opSetDepthState(true, true);

        // draw objects
        for (int y=-5; y<=5; ++y)
        {
            for (int x = -5; x <= 5; ++x)
            {
                const float speed = (1.0f + std::abs(x * 0.1f) + std::abs(y * 0.1f)) * 60.0f;

                auto localToWorld = Matrix::BuildRotation(0.0f, m_time * speed, 0.0f);
                localToWorld.translation(x, y, 0.0f);

                setupObject(cmd, localToWorld);
                cmd.opDrawIndexed(m_shaders, 0, 0, 36);
            }
        }

        cmd.opEndPass();
    }

    bool SimpleApp::compileShaders()
    {
        m_shaders = LoadResource<ShaderLibrary>(ResourcePath("examples/shaders/simple_box_example.csl")).acquire();
        return !m_shaders.empty();
    }

    bool SimpleApp::buildGeometry()
    {
        const float size = 0.5f;

        InplaceArray<helper::BoxVertex, 6 * 4> boxVertices;
        InplaceArray<uint16_t, 6 * 6> boxIndices;

        helper::BuildBox(size, boxVertices, boxIndices);

        {
            BufferCreationInfo info;
            info.label = "BoxVertices";
            info.size = boxVertices.dataSize();
            info.allowVertex = true;

            SourceData source;
            source.data = boxVertices.createBuffer();

            m_vertexBuffer = GetService<DeviceService>()->device()->createBuffer(info, &source);
            if (!m_vertexBuffer)
                return false;
        }

        {
            BufferCreationInfo info;
            info.label = "BoxIndices";
            info.size = boxIndices.dataSize();
            info.allowIndex = true;

            SourceData source;
            source.data = boxIndices.createBuffer();

            m_indexBuffer = GetService<DeviceService>()->device()->createBuffer(info, &source);
            if (!m_indexBuffer)
                return false;
        }

        return true;
    }

} // example

base::app::IApplication& GetApplicationInstance()
{
    static example::SimpleApp theApp;
    return theApp;
}
