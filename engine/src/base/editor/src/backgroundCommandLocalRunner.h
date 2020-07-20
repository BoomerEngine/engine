/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "backgroundCommand.h"

namespace ed
{
    ///---

    /// local runner runs the command using a local command host
    class BackgroundCommandRunnerLocal : public IBackgroundJob
    {
    public:
        BackgroundCommandRunnerLocal(const app::CommandHostPtr& host, IBackgroundCommand* command);
        virtual ~BackgroundCommandRunnerLocal();

        /// create a local runner, fails if the command cannot be started
        static BackgroundJobPtr Run(const app::CommandLine& cmdline, IBackgroundCommand* command);

    protected:
        virtual bool update() override final;
        virtual bool running() const override final;
        virtual int exitCode() const override final;
        virtual void requestCancel() const override final;

        app::CommandHostPtr m_host;
        BackgroundCommandPtr m_command;
    };

    ///---

} // editor

