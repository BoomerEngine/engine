/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "base/system/include/registration.h"
#include "base/system/include/mutex.h"
#include "base/containers/include/queue.h"
#include "base/app/include/commandline.h"

namespace ed
{
    //--

    /// a command definition
    class BASE_EDITOR_API Command : public NoCopy
    {
    public:
        Command();

        //--

        // command tag, so we can count them
        StringID m_tag;

        // description, used in UI for user feedback
        StringBuf m_description;

        // executable, usually the "bcc" but can be overridden
        io::AbsolutePath m_executablePath;

        // commandline
        app::CommandLine m_commandline;

        // can this command run in the background to other commands ?
        bool m_background = false;

        //--

        // user data
        StringBuf m_userDataFilePath;

    public:
        // get the path to BCC
        static io::AbsolutePath GetSelfPath();

        // get the path to the game engine launcher
        static io::AbsolutePath GetLauncherPath();
    };

    //--

    /// manager of background standalone commands
    class BASE_EDITOR_API CommandManager : NoCopy
    {
    public:
        CommandManager();
        ~CommandManager();

        // schedule command to run
        void schedule(Command* command);

        //--

        // count number of commands with given tag
        uint32_t countCommands(const StringID tag, bool countScheduled, bool countProcessing) const;

        //--

        // wait for all currently running and scheduled commands to finish or cancel them
        bool finishAllCommands();

        // advance command execution
        void processPendingCommands();

        // get stats (pending/processing commands)
        void stats(uint32_t& outNumPendingCommands, uint32_t& outNumProcessingCommands);

    private:
        bool m_enabled;

        Queue<Command*> m_commandQueue;
        HashMap<StringID, int> m_commandQueueCounts;
        Mutex m_commandQueueLock;

        struct RunningCommand
        {
            process::IProcess* m_process;
            Command* m_command;
        };

        Array<RunningCommand> m_runningCommands;
        HashMap<StringID, int> m_runningCounts;
        Mutex m_runningCommandsLock;

        void tryStartCommand();
    };

} // editor

