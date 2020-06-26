/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#include "build.h"
#include "command.h"
#include "commandline.h"

namespace base
{
    namespace app
    {
        //--

        RTTI_BEGIN_TYPE_CLASS(CommandNameMetadata);
        RTTI_END_TYPE();

        CommandNameMetadata::CommandNameMetadata()
        {}

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ICommand);
        RTTI_END_TYPE();

        //--

        ICommand::ICommand()
        {}

        ICommand::~ICommand()
        {}

        //--

    } // app
} // base