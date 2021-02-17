/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base/app/include/application.h"

namespace application
{

    // game launcher application
    class LauncherApp : public base::app::IApplication
    {
    public:
        LauncherApp(base::StringView title = "");

        virtual bool initialize(const base::app::CommandLine& commandline) override final;
        virtual void cleanup() override final;
        virtual void update() override final;

    private:
        // game title
        base::StringBuf m_title;

        // rendering output (native window)
        rendering::OutputObjectPtr m_renderingOutput;

        // timing utility used to calculate time delta
        base::NativeTimePoint m_lastTickTime;

        // game host
        base::RefPtr<game::Host> m_gameHost;

        //--

        bool createWindow(const base::app::CommandLine& commandline);
        bool createGame(const base::app::CommandLine& commandline);

        void updateWindow();
        void updateGame(double dt);

        bool processInput(const base::input::BaseEvent& evt);

        void renderFrame();
        void renderGame(rendering::command::CommandWriter& cmd, const game::HostViewport& viewport);
        void renderOverlay(rendering::command::CommandWriter& cmd, const game::HostViewport& viewport);
    };

} // application
