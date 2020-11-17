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

        CommandHost::CommandHost()
        {}

        CommandHost::~CommandHost()
        {
            if (m_command)
            {
                ScopeTimer timer;

                TRACE_WARNING("Command: Command '{}' is still running, we will have to wait for it to finish", *m_command);
                m_command->requestCancel();

                Fibers::GetInstance().waitForCounterAndRelease(m_finishedSignal);

                TRACE_INFO("Command: Finished waiting for command in {}", timer);
            }
        }

        class CommandHelloMessageHandler : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CommandHelloMessageHandler, IObject);

        public:
            bool m_receivedPing = false;

            CommandHelloMessageHandler()
            {}

            void handleHelloResponseMessage(const CommandHelloResponseMessage& msg, const net::MessageConnectionPtr& connection)
            {
                auto diff = NativeTimePoint(msg.timestampSentBack).timeTillNow().toSeconds();
                TRACE_INFO("Command: Ping to message server: {}", TimeInterval(diff));

                m_receivedPing = true;
            }
        };

        RTTI_BEGIN_TYPE_CLASS(CommandHelloMessageHandler);
        RTTI_FUNCTION_SIMPLE(handleHelloResponseMessage);
        RTTI_END_TYPE();

        bool CommandHost::start(const CommandLine& commandline)
        {
            // find command to execute
            TRACE_INFO("Command: Starting '{}'", commandline.toString());
            const auto commandClass = FindCommandClass(commandline.command());
            if (!commandClass)
                return false;

            // create the command
            auto command = commandClass.create();

            // if we were told to connect to host do it now
            if (commandline.hasParam("messageServer"))
            {
                const auto remoteAddressStr = commandline.singleValue("messageServer");

                // parse the target address
                base::socket::Address remoteAddress;
                if (!base::socket::Address::Parse(remoteAddressStr, remoteAddress))
                {
                    TRACE_ERROR("Command: Address '{}' is not a valid network address", remoteAddressStr);
                    return false;
                }

                // connect to target
                m_connection = base::RefNew<base::net::TcpMessageClient>();
                if (!m_connection->connect(remoteAddress))
                {
                    TRACE_ERROR("Command: Failed to connect to message server at '{}' ", remoteAddressStr);
                    return false;
                }

                // before we event start running send the "hello message" to the let it know that the remote endpoint has been established
                {
                    CommandHelloMessage msg;
                    msg.localTimestamp = NativeTimePoint::Now().rawValue();
                    msg.connectionKey = commandline.singleValue("messageConnectionKey");
                    commandline.singleValue("messageStartupTimestamp").view().match(msg.startupTimestamp);

                    m_connection->send(msg);
                }
            }

            // create the signal to run
            m_finishedFlag = false;
            m_finishedSignal = Fibers::GetInstance().createCounter("CommandFinished");

            // prepare to start
            m_command = command;

            // start the command fiber
            RunFiber("Command") << [commandline, command, this](FIBER_FUNC)
            {
                command->run(m_connection, commandline);
                m_finishedFlag = true;
                Fibers::GetInstance().signalCounter(m_finishedSignal);
            };

            // we are ok, command has started
            return true;
        }

        void CommandHost::cancel()
        {
            if (m_command)
                m_command->requestCancel();
        }

        bool CommandHost::update()
        {
            // monitor connection
            if (m_connection)
            {
                // dispatch all messages to the command
                while (auto message = m_connection->pullNextMessage())
                    m_command->processMessage(message, m_connection);

                // if we have lost connection request command to finish
                if (!m_connection->isConnected())
                {
                    TRACE_WARNING("Connection to host lost, we will attempt to cancel the command");
                    m_command->requestCancel();
                    m_connection.reset(); // just close it on the local end as well
                }
            }                

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