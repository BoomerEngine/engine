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

    class Game;

    // 2D game example app
    class GameApp : public IApplication
    {
    public:
        GameApp();

        virtual bool initialize(const CommandLine& commandline) override final;
        virtual void update() override final;
        virtual void cleanup() override final;

    private:
        bool createWindow();
        void updateWindow();

        void renderFrame();
        void renderNonGameOverlay(CommandWriter& cmd, uint32_t width, uint32_t height, const rendering::ImageView& colorTarget, const rendering::ImageView& depthTarget);

        bool processInput(const BaseEvent& evt);

        //--

        // timing utility used to calculate time delta
        NativeTimePoint m_lastTickTime;

        // rendering output (native window)
        rendering::ObjectID m_renderingOutput;
        IDriverNativeWindowInterface* m_renderingWindow = nullptr;

        //--

        bool shouldCaptureInput();

        bool m_showDebugPanels = false;
        ImGuiContext* m_imgui = nullptr;

        bool m_paused = false;
        float m_timeScale = 1.0f;

        RefPtr<Game> m_game;

        //--
    };

} // example

