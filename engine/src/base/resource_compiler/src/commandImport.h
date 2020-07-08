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

        class CommandImport : public app::ICommand
        {
            RTTI_DECLARE_VIRTUAL_CLASS(CommandImport, app::ICommand);

        public:
            virtual bool run(const app::CommandLine& commandline) override final;
        };

        //--

    } // res
} // base
