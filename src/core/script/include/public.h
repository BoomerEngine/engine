/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_script_glue.inl"

BEGIN_BOOMER_NAMESPACE_EX(script)

enum class DebugBreak : uint8_t
{
    None,
};

class FunctionCodeBlock;

class PortableData;
typedef RefPtr<PortableData> PortableDataPtr;

class CompiledProject;
typedef RefPtr<CompiledProject> CompiledProjectPtr;

class JITProject;
typedef RefPtr<JITProject> JITProjectPtr;

END_BOOMER_NAMESPACE_EX(script)
