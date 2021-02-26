/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core/app/include/application.h"

BEGIN_BOOMER_NAMESPACE()

// game launcher application
class LauncherApp : public app::IApplication
{
public:
    LauncherApp(StringView title = "");

    virtual bool initialize(const app::CommandLine& commandline) override final;
    virtual void cleanup() override final;
    virtual void update() override final;

private:
    // game title
    StringBuf m_title;

    // rendering output (native window)
    gpu::OutputObjectPtr m_renderingOutput;

    // timing utility used to calculate time delta
    NativeTimePoint m_lastTickTime;

    // game host
    RefPtr<Host> m_gameHost;

    //--

    bool createWindow(const app::CommandLine& commandline);
    bool createGame(const app::CommandLine& commandline);

    void updateWindow();
    void updateGame(double dt);

    bool processInput(const input::BaseEvent& evt);

    void renderFrame();
    void renderGame(gpu::CommandWriter& cmd, const HostViewport& viewport);
    void renderOverlay(gpu::CommandWriter& cmd, const HostViewport& viewport);
};

END_BOOMER_NAMESPACE()