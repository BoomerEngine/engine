/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"

#include "bccApplication.h"
#include "base/app/include/application.h"
#include "base/app/include/command.h"
#include "base/app/include/commandline.h"
#include "base/app/include/launcherPlatform.h"
#include "base/socket/include/address.h"
#include "base/app/include/commandHost.h"

namespace application
{
 
    BCCApp::BCCApp()
    {}

    BCCApp::~BCCApp()
    {}

    struct CommandInfo
    {
        base::StringView name;
        base::StringView desc;
        base::SpecificClassType<base::app::ICommand> cls;
    };

    base::SpecificClassType<base::app::ICommand> FindCommandClass(base::StringView name)
    {
        // list all command classes
        base::InplaceArray<base::SpecificClassType<base::app::ICommand>, 100> commandClasses;
        RTTI::GetInstance().enumClasses(commandClasses);

        // find command by name
        base::InplaceArray<CommandInfo, 100> commandInfos;
        for (const auto& cls : commandClasses)
        {
            // if user specified direct class name always allow to use it (some commands are only accessible like that)
            if (cls->name() == name)
                return cls;

            // check the name from the metadata
            if (const auto* nameMetadata = cls->findMetadata<base::app::CommandNameMetadata>())
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

    class BCCLogSinkWithErrorCapture : public base::logging::GlobalLogSink
    {
    public:
        BCCLogSinkWithErrorCapture(bool captureErrors)
            : m_captureErrors(captureErrors)
        {
        }

        ~BCCLogSinkWithErrorCapture()
        {
            m_allErrors.clearPtr();
        }

        virtual bool print(base::logging::OutputLevel level, const char* file, uint32_t line, const char* context, const char* text) override final
        {
            if (level == logging::OutputLevel::Warning)
                m_numWarnings += 1;

            if (level == logging::OutputLevel::Error)
            {
                m_numErrors += 1;

                if (m_captureErrors)
                {
                    auto lock = base::CreateLock(m_allErrorsLock);

                    auto errorInfo = new ErrorInfo;
                    errorInfo->line = line;
                    errorInfo->file = base::StringBuf(file);
                    errorInfo->text = base::StringBuf(text);
                    errorInfo->context = base::StringBuf(context);
                    m_allErrors.pushBack(errorInfo);
                }
            }

            return false;
        }

        bool printSummary(double time)
        {
            base::logging::Log::DetachGlobalSink(this);

            const auto numErrors = m_numErrors.load();
            const auto numWarnings = m_numWarnings.load();

            {
                auto lock = base::CreateLock(m_allErrorsLock);
                if (!m_allErrors.empty())
                {
                    TRACE_ERROR("Captured {} errors:", m_allErrors.size());
                    for (const auto* msg : m_allErrors)
                        logging::Log::Print(logging::OutputLevel::Error, msg->file.c_str(), msg->line, msg->context.c_str(), msg->text.c_str());
                }
            }

            if (numErrors == 0)
            {
                TRACE_INFO("========== SUCCESS: {}, {} errors, {} warning(s) ==========", TimeInterval(time), numErrors, numWarnings);
            }
            else
            {
                TRACE_ERROR("========== FAILURE: {}, {} errors, {} warning(s) ==========", TimeInterval(time), numErrors, numWarnings);
            }

            return 0 == numErrors;
        }

    private:
        std::atomic<uint32_t> m_numErrors = 0;
        std::atomic<uint32_t> m_numWarnings = 0;

        struct ErrorInfo : public mem::GlobalPoolObject<>
        {
            uint32_t line = 0;
            base::StringBuf file;
            base::StringBuf context;
            base::StringBuf text;
        };

        base::Array<ErrorInfo*> m_allErrors;
        base::SpinLock m_allErrorsLock;

        bool m_captureErrors;
    };

    bool BCCApp::initialize(const base::app::CommandLine& commandline)
    {
        // should we capture errors ?
        const auto captureErrors = !commandline.hasParam("noErrorCapture");
        m_globalSink.create(captureErrors);

        // remember the time we started at
        m_startedTime.resetToNow();

        // create the host for the command to run
        auto host = base::RefNew<base::app::CommandHost>();
        if (!host->start(commandline))
            return false;

        // keep running
        m_commandHost = host;
        return true;
    }

    void BCCApp::update()
    {
        if (!m_commandHost->update())
        {
            TRACE_INFO("Command: Host signalled that it's finished");
            m_commandHost.reset();
        }

        if (!m_commandHost)
        {
            base::platform::GetLaunchPlatform().requestExit("Command finished");
        }
    }

    void BCCApp::cleanup()
    {
        if (m_globalSink)
        {
            m_globalSink->printSummary(m_startedTime.timeTillNow().toSeconds());
            m_globalSink.reset();
        }
    }

} // application
