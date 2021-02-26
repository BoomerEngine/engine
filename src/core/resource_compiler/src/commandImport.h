/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#pragma once

#include "core/app/include/command.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

//--

struct ImportQueueFileCancel;

//--

class CommandImport : public app::ICommand
{
    RTTI_DECLARE_VIRTUAL_CLASS(CommandImport, app::ICommand);

public:
    CommandImport();
    virtual ~CommandImport();

    virtual bool run(IProgressTracker* progress, const app::CommandLine& commandline) override final;
};

//--

END_BOOMER_NAMESPACE_EX(res)
