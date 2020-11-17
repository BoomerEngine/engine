/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
***/

#pragma once

#include "base/memory/include/linearAllocator.h"
#include "renderingShaderOpcodeGenerator.h"

namespace rendering
{
    namespace compiler
    {

        /// helper class that serves as a cache for generated shader blobs, allows us to skip compilation on various occasions
        class LinkerCache : public base::NoCopy
        {
        public:
            LinkerCache(base::mem::LinearAllocator& mem, CodeLibrary& code);
            ~LinkerCache();

            //--

            // fold an exported function
            const Function* foldFunction(const Function* function, const ProgramInstance* pi, base::parser::IErrorReporter& err);

            //--

            // retrieve a cached generated opcodes
            bool findCompiledShader(ShaderType type, const Function* sourceFunction,
                const opcodes::ShaderResourceBindings* bindingSetup,
                const opcodes::ShaderVertexInputSetup* vertexSetup,
                const opcodes::ShaderStageDependencies* prevStageDeps,
                PipelineIndex& outBlobIndex,
                opcodes::ShaderStageDependencies& outDependencies);

            // store compiled function data for next lucky picker
            void storeCompiledShader(ShaderType type, const Function* sourceFunction,
                const opcodes::ShaderResourceBindings* bindingSetup,
                const opcodes::ShaderVertexInputSetup* vertexSetup,
                const opcodes::ShaderStageDependencies* prevStageDeps,
                PipelineIndex blobIndex,
                const opcodes::ShaderStageDependencies& dependencies);

            //--
            
            // needed only for std::hash
            struct CompiledFunctionKey
            {
                uint8_t type = 0;
                uint64_t functionHash = 0;
                uint64_t bindingHash = 0;
                uint64_t vertexInputHash = 0;
                uint64_t prevStageHash = 0;

                static uint32_t CalcHash(const CompiledFunctionKey& key);

                bool operator==(const CompiledFunctionKey& other) const;
                bool operator!=(const CompiledFunctionKey& other) const;
            };
            

        private:
            base::mem::LinearAllocator& m_mem;

            base::UniquePtr<FunctionFolder> m_folder;

            struct CompiledFunctionData
            {
                PipelineIndex blobIndex;
                opcodes::ShaderStageDependencies dependencies;

                INLINE CompiledFunctionData() {};
                INLINE CompiledFunctionData(const CompiledFunctionData& other) = default;
                INLINE CompiledFunctionData& operator=(const CompiledFunctionData& other) = default;
            };

            base::HashMap<CompiledFunctionKey, CompiledFunctionData> m_compiledFunctionMap;
        };

        // shader bundle is a group of shaders for different pipeline stages
        struct ShaderBunderSetup
        {
            static const uint8_t MAX_SHADERS = (uint8_t)ShaderType::MAX;

            struct Entry
            {
                const Function* func = nullptr;
                const ProgramInstance* pi = nullptr;
            };

            base::parser::Location location;
            Entry stages[MAX_SHADERS];
        };

        // link shaders into a final program (combination of shaders) based on the shaders provided
        // generates all other data for the linked program: parameter mapping (root signature), descriptor layouts as well as vertex input state
        extern PipelineIndex AssembleShaderBundle(
            base::mem::LinearAllocator& mem, // scratch pad JUST for this operation
            ShaderLibraryBuilder& outBuilder, 
            const CodeLibrary& readOnlyLib, 
            LinkerCache& linkerCache,
            const opcodes::IShaderOpcodeGenerator* generator,
            base::StringView contextPath,
            const ShaderBunderSetup& bundleInfo,
            base::parser::IErrorReporter& err);

    } // compiler
} // rendering


