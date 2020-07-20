/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "editorService.h"
#include "backgroundCommand.h"

#include "base/net/include/messageConnection.h"
#include "base/net/include/messagePool.h"

namespace ed
{
    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IBackgroundCommand);
    RTTI_END_TYPE();

    static const char* KEY_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    static const uint32_t KEY_LENGTH = 20;

    IBackgroundCommand::IBackgroundCommand(StringView<char> name)
        : m_name(name)
    {
        StringBuilder txt;

        FastRandState rng(NativeTimePoint::Now().rawValue()); // TODO: make LESS predictable 
        for (uint32_t i = 0; i < KEY_LENGTH; ++i)
        {
            static const auto NUM_KEYS_CHARS = strlen(KEY_CHARS);
            txt.appendch(KEY_CHARS[RandMax(rng, NUM_KEYS_CHARS)]);
        }

        m_connectionKey = txt.toString();
    }

    IBackgroundCommand::~IBackgroundCommand()
    {}

    void IBackgroundCommand::update()
    {
        if (m_connection)
        {
            // remote connection was closed
            if (!m_connection->isConnected())
            {
                TRACE_WARNING("BackgroundCommand: Remote host for command '{}' ({}) disconnected", name(), connectionKey());
                m_connection->close();
                m_connection.reset();
            }

            // pull messages and dispatch received messages (mostly UI updates)
            while (auto message = m_connection->pullNextMessage())
                message->dispatch(this, m_connection);
        }
    }

    void IBackgroundCommand::confirmed(const net::MessageConnectionPtr& connection)
    {
        DEBUG_CHECK_EX(!m_connection, "Background command was already confirmed");
        TRACE_INFO("BackgroundCommand: Remote command '{}' ({}) confirmed starting", name(), connectionKey());
        m_connection = connection;
    }

    //--

    IBackgroundJob::IBackgroundJob(StringView<char> name)
        : m_description(name)
    {
        m_startTime.resetToNow();
    }

    IBackgroundJob::~IBackgroundJob()
    {}

    //--

} // editor
