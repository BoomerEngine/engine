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

#include "core/replication/include/replicationRttiExtensions.h"
#include "core/net/include/messageConnection.h"
#include "core/net/include/messagePool.h"

BEGIN_BOOMER_NAMESPACE_EX(app)

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

END_BOOMER_NAMESPACE_EX(app)
