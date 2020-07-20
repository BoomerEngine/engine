/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "editorConfig.h"

#include "base/app/include/command.h"

namespace ed
{
    ///---

    /// a external process running engine command (like import/export etc)
    /// NOTE: since those are commands we can also run them locally inside the same executable (for debugging)
    class BASE_EDITOR_API IBackgroundCommand : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IBackgroundCommand, IObject);

    public:
        IBackgroundCommand(StringView<char> name);
        virtual ~IBackgroundCommand();

        //---

        /// get name of the command
        INLINE const StringBuf& name() const { return m_name; }

        /// get connection to running command, valid only after we get confirmed
        /// NOTE: this will be null initially until the connection is established (e.g. startup time of the remote executable, etc)
        INLINE const net::MessageConnectionPtr& connection() const { return m_connection; }

        /// get unique connection key that identifies the remote running command
        INLINE const StringBuf& connectionKey() const { return m_connectionKey; }

        //---

        // configure command by collecting the commandline arguments
        virtual bool configure(app::CommandLine& outCommandline) = 0;

        // called when it's safe to update stuff
        virtual void update();

        // remote site just connected to us
        virtual void confirmed(const net::MessageConnectionPtr& connection);

        //---

    private:
        StringBuf m_name;
        StringBuf m_connectionKey;

        SpinLock m_connectionLock;
        net::MessageConnectionPtr m_connection;
    };
    
    ///---

    /// editor side host that runs the command
    class BASE_EDITOR_API IBackgroundJob : public IReferencable
    {
    public:
        IBackgroundJob(StringView<char> name);
        virtual ~IBackgroundJob();

        //---

        /// descriptive name of command we are running
        ALWAYS_INLINE const StringBuf& description() const { return m_description; }

        /// when did we start ?
        ALWAYS_INLINE const NativeTimePoint& startTime() const { return m_startTime; }

        //---

        /// are we running ?
        virtual bool running() const = 0;

        /// if we are not running get the exit code
        virtual int exitCode() const = 0;

        /// request nice, well behaved cancel of the whole thing
        virtual void requestCancel() const = 0;

        //---

        /// update internal state, should return false when runner has finished
        virtual bool update() = 0;

        //---

    private:
        NativeTimePoint m_startTime;
        StringBuf m_description;
    };

    ///---

} // editor

