/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "backgroundCommand.h"
#include "backgroundCommandLocalRunner.h"

#include "editorService.h"

#include "base/app/include/commandline.h"
#include "base/app/include/commandHost.h"

namespace ed
{
    //--

    BackgroundCommandRunnerLocal::BackgroundCommandRunnerLocal(const app::CommandHostPtr& host, IBackgroundCommand* command)
        : IBackgroundJob(TempString("Command '{}'", command->name()))
        , m_host(host)
        , m_command(AddRef(command))
    {}

    BackgroundCommandRunnerLocal::~BackgroundCommandRunnerLocal()
    {
        m_command.reset();
        m_host.reset();
    }

    BackgroundJobPtr BackgroundCommandRunnerLocal::Run(const app::CommandLine& cmdline, IBackgroundCommand* command)
    {
        // create command running host
        auto host = CreateSharedPtr<app::CommandHost>();
        if (!host->start(cmdline))
        {
            TRACE_WARNING("Editor: Background command '{}' failed to start properly", command->name());
            return nullptr;
        }

        // ok, we started, created the wrapper
        return CreateSharedPtr<BackgroundCommandRunnerLocal>(host, command);
    }

    bool BackgroundCommandRunnerLocal::update()
    {
        // update the command
        m_command->update();

        // update the command host
        return m_host->update();
    }

    bool BackgroundCommandRunnerLocal::running() const
    {
        return m_host->running();
    }

    int BackgroundCommandRunnerLocal::exitCode() const
    {
        return 0;
    }

    void BackgroundCommandRunnerLocal::requestCancel() const
    {
        m_host->cancel();
    }

    //--

} // editor
