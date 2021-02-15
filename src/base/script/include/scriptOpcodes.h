/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: runtime #]
***/

#pragma once


namespace base
{
    namespace script
    {

        // opcodes defined in the system
        enum class Opcode : uint16_t
        {
#define OPCODE(x) x,
#include "scriptOpcodesRaw.h"
#undef OPCODE
        };

        // find opcode index by name
        extern BASE_SCRIPT_API bool FindOpcodeByName(StringID name, Opcode& outOpcode);

    } // script
} // base

