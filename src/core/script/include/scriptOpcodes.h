/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: runtime #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(script)

// opcodes defined in the system
enum class Opcode : uint16_t
{
#define BOOMER_SCRIPT_OPCODE(x) x,
#include "scriptOpcodesRaw.h"
#undef BOOMER_SCRIPT_OPCODE
};

// find opcode index by name
extern CORE_SCRIPT_API bool FindOpcodeByName(StringID name, Opcode& outOpcode);

END_BOOMER_NAMESPACE_EX(script)
