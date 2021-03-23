/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core/app/include/application.h"

BEGIN_BOOMER_NAMESPACE()

// game launcher application
class LauncherApp : public IApplication
{
public:
    LauncherApp(StringView title = "");

    virtual bool initialize(const CommandLine& commandline) override final;
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
    GameScreenStackPtr m_screenStack;

    //--

    bool parseWindowParams(const CommandLine& commandline);

    void toggleFullscreen();

    bool createWindow();
    bool createGame(const CommandLine& commandline);

    void updateWindow();
    void updateGame(double dt);

    bool processInput(const input::BaseEvent& evt);

    void renderFrame();
};

END_BOOMER_NAMESPACE()