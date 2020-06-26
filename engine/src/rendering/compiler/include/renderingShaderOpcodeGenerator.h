/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\opcodes #]
***/

#pragma once

#include "base/reflection/include/variantTable.h"
#include "renderingShaderDataType.h"

namespace rendering
{
    namespace compiler
    {
        class ResourceTable;
        class ResourceBindingSetup;

        namespace opcodes
        {
            
            /// vertex input description for shader compilation
            /// usually setup by the pipeline compiler
            struct RENDERING_COMPILER_API ShaderVertexInputEntry
            {
                base::StringID bindingName;
                base::StringID memberName;
                uint16_t offset = 0;
                ImageFormat dataFormat = ImageFormat::UNKNOWN;
                uint8_t streamIndex = 0; // 0,1, etc...
                uint8_t layoutIndex = 0; // index in the whole layout
            };

            typedef base::Array<ShaderVertexInputEntry> ShaderVertexInputSetup;

            /// dependency information for compiled shader on previous stage. ie. list of stage inputs that the shader actually used and WHERE IT PLACED THEM
            /// we compile shaders "back to front", ie. always start with "pixel/fragment shader" and then go back the pipeline eventually finishing at vertex shader
            /// if pixel shader did not use the "vertex color" input than if luck permits we may end up not even fetching the data from vertex buffer in the vertex shader
            /// NOTE: it is STRICTLY recommended to extract this information from reflection of the actually compiled shader binary (ie. HLSL opcodes etc) as this allows for more filtering
            struct RENDERING_COMPILER_API ShaderStageDependency
            {
                base::StringID name; // binding name (WorldPosition, ViewPosition, etc..)
                DataType type; // data type of the dependency, the core type must match, for some stages it's permitted to change stuff to an array, etc
                char assignedLayoutIndex = -1; // where do we expect the stuff to be 

                // TODO: anything else ?
            };

            typedef base::Array<ShaderStageDependency> ShaderStageDependencies;

            //--

            /// information about resource binding
            struct RENDERING_COMPILER_API ShaderResourceBindingEntry
            {
                const ResourceTable* table = nullptr;
                const ResourceTableEntry* tableEntry = nullptr;

                int set = -1; // set (for descriptors) we are bound to, 0 on GLSL, HLSL11 etc
                int position = -1; // position in the set and/or in the binding table for given resource types, depends on the implementation
            };

            typedef base::Array<ShaderResourceBindingEntry> ShaderResourceBindings;

            //--

            /// shader opcode generation context
            struct RENDERING_COMPILER_API ShaderOpcodeGenerationContext : public base::NoCopy
            {
                /// target pipeline type
                ShaderType stage = ShaderType::None;

                /// shader unique identifier (name of the fragments)
                base::StringBuf shaderName;

                /// entry function of shader to convert, rest of the shit is pulled automatically
                /// NOTE: this must be folded function
                const Function* entryFunction = nullptr;

                // bound shader parameters locations
                // NOTE: the same for all shaders
                const ShaderResourceBindings* bindingSetup = nullptr;

                /// vertex shader input format
                /// NOTE: set only for vertex shader
                const ShaderVertexInputSetup* vertexInput = nullptr;

                /// required stage outputs to be produced by this shader
                /// usually this is the input from next stage that is compiled first so we know what is needed
                const ShaderStageDependencies* requiredShaderOutputs = nullptr;
            };

            //---

            /// platform dependent opcode generator for a compiled shader
            /// this is a final step in shader compilation, up to this point everything happens in the same way for all platforms
            class RENDERING_COMPILER_API IShaderOpcodeGenerator : public base::IReferencable
            {
                RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IShaderOpcodeGenerator);

            public:
                virtual ~IShaderOpcodeGenerator();

                /// build resource entries to use in this shader
                virtual bool buildResourceBinding(const base::Array<const rendering::compiler::ResourceTable*>& resourceTables, 
                    rendering::compiler::opcodes::ShaderResourceBindings& outBinding, base::parser::IErrorReporter& err) const = 0;

                /// generate platform dependent opcodes
                /// NOTE: function must be thread safe and reentrant
                virtual bool generateOpcodes(const rendering::compiler::opcodes::ShaderOpcodeGenerationContext& context, 
                    base::Buffer& outData, rendering::compiler::opcodes::ShaderStageDependencies& outDependencies, base::parser::IErrorReporter& err) const = 0;
            };

            //---

        } // opcodes
    } // shader 
}// rendering