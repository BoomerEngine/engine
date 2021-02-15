/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "command.h"
#include "commandHost.h"
#include "commandline.h"

#include "base/net/include/tcpMessageClient.h"
#include "base/socket/include/address.h"

namespace base
{
    namespace app
    {
        //--


        struct CommandInfo
        {
            StringView name;
            StringView desc;
            SpecificClassType<app::ICommand> cls;
        };

        SpecificClassType<app::ICommand> FindCommandClass(StringView name)
        {
            // list all command classes
            InplaceArray<SpecificClassType<app::ICommand>, 100> commandClasses;
            RTTI::GetInstance().enumClasses(commandClasses);

            // find command by name
            InplaceArray<CommandInfo, 100> commandInfos;
            for (const auto& cls : commandClasses)
            {
                // if user specified direct class name always allow to use it (some commands are only accessible like that)
                if (cls->name() == name)
                    return cls;

                // check the name from the metadata
                if (const auto* nameMetadata = cls->findMetadata<app::CommandNameMetadata>())
                {
                    if (nameMetadata->name() == name)
                        return cls;

                    // store a command info in case we will print an error
                    auto& info = commandInfos.emplaceBack();
                    info.name = nameMetadata->name();
                    info.cls = cls;

                    // TODO: desc ?
                }
            }

            // class not found
            if (!name.empty())
                TRACE_ERROR("Command '{}' was not found", name);

            if (!commandClasses.empty())
            {
                std::sort(commandInfos.begin(), commandInfos.end(), [](const CommandInfo& a, const CommandInfo& b) { return a.name < b.name; });

                TRACE_INFO("There are {} known, public commands:", commandInfos.size());
                for (const auto& info : commandInfos)
                {
                    if (info.desc)
                    {
                        TRACE_INFO("  {} - {}", info.name, info.desc);
                    }
                    else
                    {
                        TRACE_INFO("  {} ({})", info.name, info.cls->name());
                    }
                }
            }

            return nullptr;
        }

        //--

        CommandHost::CommandHost(IProgressTracker* progress)
            : m_externalProgressTracker(progress)
        {}

        CommandHost::~CommandHost()
        {
            if (m_command)
            {
                ScopeTimer timer;

                TRACE_WARNING("Command: Command '{}' is still running, we will have to wait for it to finish", *m_command);
                //m_command->requestCancel();

                Fibers::GetInstance().waitForCounterAndRelease(m_finishedSignal);

                m_command.reset();

                TRACE_INFO("Command: Finished waiting for command in {}", timer);
            }
        }     

        bool CommandHost::checkCancelation() const
        {
            return m_cancelation || (m_externalProgressTracker && m_externalProgressTracker->checkCancelation());
        }

        void CommandHost::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
        {
            if (m_externalProgressTracker)
                m_externalProgressTracker->reportProgress(currentCount, totalCount, text);
        }

        void CommandHost::cancel()
        {
            m_cancelation.exchange(true);
        }

        bool CommandHost::start(const CommandLine& commandline)
        {
            // find command to execute
            TRACE_INFO("Command: Starting '{}'", commandline.toString());
            const auto commandClass = FindCommandClass(commandline.command());
            if (!commandClass)
                return false;

            // create the command
            auto command = commandClass.create();

            // create the signal to run
            m_finishedFlag = false;
            m_finishedSignal = Fibers::GetInstance().createCounter("CommandFinished");

            // prepare to start
            m_command = command;

            // start the command fiber
            RunFiber("Command") << [commandline, command, this](FIBER_FUNC)
            {
                command->run(this, commandline);
                m_finishedFlag = true;
                Fibers::GetInstance().signalCounter(m_finishedSignal);
            };

            // we are ok, command has started
            return true;
        }

        bool CommandHost::update()
        {
            // yay. we are done
            if (m_finishedFlag)
            {
                if (m_command)
                {
                    TRACE_INFO("Command: Command '{}' finished", m_command);
                    m_command.reset();
                }

                m_finishedFlag = false;
            }

            // keep running as long as we have the pointer
            return m_command;
        }

        //--

    } // app
} // base