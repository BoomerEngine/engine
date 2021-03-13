/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: commands #]
***/

#pragma once

#include "core/app/include/command.h"

BEGIN_BOOMER_NAMESPACE()

//--

struct ImportQueueFileCancel;

//--

class CommandImport : public ICommand
{
    RTTI_DECLARE_VIRTUAL_CLASS(CommandImport, ICommand);

public:
    CommandImport();
    virtual ~CommandImport();

    virtual bool run(IProgressTracker* progress, const CommandLine& commandline) override final;
};

//--

END_BOOMER_NAMESPACE()
