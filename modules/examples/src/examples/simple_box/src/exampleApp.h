/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#pragma once

namespace example
{

    // Simple example application 
    class SimpleApp : public IApplication
    {
    public:
        SimpleApp();

        virtual bool initialize(const CommandLine& commandline) override final;
        virtual void update() override final;
        virtual void cleanup() override final;

    private:
        bool createWindow();
        void updateWindow();

        void updateCamera(float dt);

        void renderFrame();
        void renderScene(CommandWriter& cmd, const rendering::RenderTargetView* colorTarget, const rendering::RenderTargetView* depthTarget);
        void setupCamera(CommandWriter& cmd, const Vector3& from, const Vector3& to, float fov, float aspect);
        void setupObject(CommandWriter& cmd, const Matrix& localToWorld);

        void processInput(const BaseEvent& evt);

        //--

        bool compileShaders();
        bool buildGeometry();

        //--

        // timing utility used to calculate time delta
        NativeTimePoint m_lastTickTime;

        // camera rotation
        Angles m_cameraRotation;
        float m_cameraDistance;
        bool m_cameraKeys[4];

        // accumulated simulation time
        double m_time = 0.0;

        //--

        // pipeline state for rendering
		GraphicsPipelineObjectPtr m_shadersPSO;

        // rendering output (native window)
        OutputObjectPtr m_renderingOutput;

        // box vertex and index buffer
        BufferObjectPtr m_vertexBuffer;
        BufferObjectPtr m_indexBuffer;
    };

} // example

