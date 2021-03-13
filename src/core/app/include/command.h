/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command #]
***/

#pragma once

#include "core/system/include/timing.h"
#include "localServiceContainer.h"

BEGIN_BOOMER_NAMESPACE()

//--

// the application "command" that can be run by the BCC (Boomer Content Compiler)
// the typical role of a Command is to do some processing and exit, there's no notion of a loop
// NOTE: command is ALWAYS run on a separate fiber, never on main thread
class CORE_APP_API ICommand : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(ICommand, IObject);

public:
    ICommand();
    virtual ~ICommand();

    //--

    /// do actual work of the command, called after all local services were initialized
    virtual bool run(IProgressTracker* progress, const CommandLine& commandline) = 0;

    //--
};

//--

// metadata with name of the command 
class CORE_APP_API CommandNameMetadata : public IMetadata
{
    RTTI_DECLARE_VIRTUAL_CLASS(CommandNameMetadata, IMetadata);

public:
    CommandNameMetadata();

    INLINE StringView name() const { return m_name; }

    INLINE void name(StringView name) { m_name = name; }

private:
    StringView m_name;
};

//--

END_BOOMER_NAMESPACE()
