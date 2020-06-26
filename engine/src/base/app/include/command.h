/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: app #]
***/

#pragma once

#include "base/system/include/timing.h"
#include "base/io/include/absolutePath.h"
#include "localServiceContainer.h"

namespace base
{
    namespace app
    {
        //--

        // the application "command" that can be run by the BCC (Boomer Content Compiler)
        // the typical role of a Command is to do some processing and exit, there's no notion of a loop
        class BASE_APP_API ICommand : public NoCopy
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ICommand);

        public:
            ICommand();
            virtual ~ICommand();

            /// called after all local services were initialized and is expected to do the work
            virtual bool run(const base::app::CommandLine& commandline) = 0;
        };

        //--

        // metadata with name of the command 
        class BASE_APP_API CommandNameMetadata : public rtti::IMetadata
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CommandNameMetadata, rtti::IMetadata);

        public:
            CommandNameMetadata();

            INLINE base::StringView<char> name() const { return m_name; }

            INLINE void name(base::StringView<char> name) { m_name = name; }

        private:
            base::StringView<char> m_name;
        };

        //--

    } // app
} // base
