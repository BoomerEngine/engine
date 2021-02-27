/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\stubs #]
***/

#pragma once

#include "core/memory/include/linearAllocator.h"
#include "core/memory/include/buffer.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

namespace shader
{
    struct StubProgram;
} // shader

END_BOOMER_NAMESPACE_EX(gpu)

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//--

struct AssembledShader
{
    Buffer blob;
    ShaderMetadataPtr metadata;
};

extern bool Assemblestubs(
    mem::LinearAllocator& mem,
    CodeLibrary& lib, // TODO: const!
    AssembledShader& outAssembledShader,
    StringView contextPath, // copied into program data, debug source file location, can be empty
    StringView contextOptions, // copied into program data, debug source file compilation options, can be empty
    parser::IErrorReporter& err);

//--

END_BOOMER_NAMESPACE_EX(gpu::compiler)
