/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "base/system/include/timing.h"
#include "localServiceContainer.h"

namespace base
{
    namespace app
    {

        //--

        // the application "command" that can be run by the BCC (Boomer Content Compiler)
        // the typical role of a Command is to do some processing and exit, there's no notion of a loop
        // NOTE: command is ALWAYS run on a separate fiber, never on main thread
        class BASE_APP_API ICommand : public IObject
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ICommand, IObject);

        public:
            ICommand();
            virtual ~ICommand();

            //--

            /// do actual work of the command, called after all local services were initialized
            virtual bool run(IProgressTracker* progress, const base::app::CommandLine& commandline) = 0;

            //--
        };

        //--

        // metadata with name of the command 
        class BASE_APP_API CommandNameMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CommandNameMetadata, rtti::IMetadata);

        public:
            CommandNameMetadata();

            INLINE base::StringView name() const { return m_name; }

            INLINE void name(base::StringView name) { m_name = name; }

        private:
            base::StringView m_name;
        };

        //--

    } // app
} // base
