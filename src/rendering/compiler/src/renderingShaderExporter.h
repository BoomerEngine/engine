/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\stubs #]
***/

#pragma once

#include "base/memory/include/linearAllocator.h"
#include "base/memory/include/buffer.h"

BEGIN_BOOMER_NAMESPACE(rendering)

namespace shader
{
    struct StubProgram;
} // shader

END_BOOMER_NAMESPACE(rendering)

BEGIN_BOOMER_NAMESPACE(rendering::shadercompiler)

//--

struct AssembledShader
{
    base::Buffer blob;
    ShaderMetadataPtr metadata;
};

extern bool AssembleShaderStubs(
    base::mem::LinearAllocator& mem,
    CodeLibrary& lib, // TODO: const!
    AssembledShader& outAssembledShader,
    base::StringView contextPath, // copied into program data, debug source file location, can be empty
    base::StringView contextOptions, // copied into program data, debug source file compilation options, can be empty
    base::parser::IErrorReporter& err);

//--

END_BOOMER_NAMESPACE(rendering::shadercompiler)