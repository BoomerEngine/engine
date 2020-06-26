/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: viewer #]
***/

#pragma once

namespace viewer
{

    // ModelViewer application, top level object
    class ModelViewerApp : public base::app::IApplication
    {
    public:
        ModelViewerApp();
        virtual ~ModelViewerApp();

        virtual bool initialize(const base::app::CommandLine& commandline) override final;
        virtual void update() override final;

    private:
        bool createWindow();

        void processInput(const base::input::BaseEvent& evt);

        void renderFrame();
        void renderGui(base::canvas::Canvas& c);

        //--

        base::NativeTimePoint m_lastTickTime;

        ImGuiContext* m_imgui = nullptr;

        rendering::DriverOutputPtr m_renderingOutput;
    };

} // viewer

