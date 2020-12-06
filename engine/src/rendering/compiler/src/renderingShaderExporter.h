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

namespace rendering
{
	namespace shader
	{
		struct StubProgram;
	} // shader

    namespace compiler
    {

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

    } // compiler
} // rendering


