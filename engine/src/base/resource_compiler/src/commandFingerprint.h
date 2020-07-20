/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#pragma once

#include "base/app/include/command.h"

namespace base
{
    namespace res
    {
        //--

        class CommandFingerprint : public app::ICommand
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CommandFingerprint, app::ICommand);

        public:
            virtual bool run(base::net::MessageConnectionPtr connection, const app::CommandLine& commandline) override final;
        };

        //--

    } // res
} // base
