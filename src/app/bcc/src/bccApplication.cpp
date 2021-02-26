/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"

#include "bccApplication.h"
#include "core/app/include/application.h"
#include "core/app/include/command.h"
#include "core/app/include/commandline.h"
#include "core/app/include/launcherPlatform.h"
#include "core/socket/include/address.h"
#include "core/app/include/commandHost.h"

BEGIN_BOOMER_NAMESPACE()

BCCApp::BCCApp()
{}

BCCApp::~BCCApp()
{}

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

class BCCLogSinkWithErrorCapture : public logging::GlobalLogSink
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

    virtual bool print(logging::OutputLevel level, const char* file, uint32_t line, const char* module, const char* context, const char* text) override final
    {
        if (level == logging::OutputLevel::Warning)
            m_numWarnings += 1;

        if (level == logging::OutputLevel::Error)
        {
            m_numErrors += 1;

            if (m_captureErrors)
            {
                auto lock = CreateLock(m_allErrorsLock);

                auto errorInfo = new ErrorInfo;
                errorInfo->line = line;
                errorInfo->file = StringBuf(file);
                errorInfo->text = StringBuf(text);
                errorInfo->context = StringBuf(context);
                m_allErrors.pushBack(errorInfo);
            }
        }

        return false;
    }

    bool printSummary(double time)
    {
        logging::Log::DetachGlobalSink(this);

        const auto numErrors = m_numErrors.load();
        const auto numWarnings = m_numWarnings.load();

        {
            auto lock = CreateLock(m_allErrorsLock);
            if (!m_allErrors.empty())
            {
                TRACE_ERROR("Captured {} errors:", m_allErrors.size());
                for (const auto* msg : m_allErrors)
                    logging::Log::Print(logging::OutputLevel::Error, msg->file.c_str(), msg->line, "", msg->context.c_str(), msg->text.c_str());
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

    struct ErrorInfo
    {
        RTTI_DECLARE_POOL(POOL_TEMP)

    public:
        uint32_t line = 0;
        StringBuf file;
        StringBuf context;
        StringBuf text;
    };

    Array<ErrorInfo*> m_allErrors;
    SpinLock m_allErrorsLock;

    bool m_captureErrors;
};

bool BCCApp::initialize(const app::CommandLine& commandline)
{
    // should we capture errors ?
    const auto captureErrors = !commandline.hasParam("noErrorCapture");
    m_globalSink.create(captureErrors);

    // remember the time we started at
    m_startedTime.resetToNow();

    // create the host for the command to run
    auto host = RefNew<app::CommandHost>();
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
        platform::GetLaunchPlatform().requestExit("Command finished");
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

END_BOOMER_NAMESPACE()
