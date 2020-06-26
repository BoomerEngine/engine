/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shaders #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/containers/include/hashMap.h"
#include "base/containers/include/hashSet.h"
#include "base/containers/include/queue.h"
#include "rendering/compiler/include/renderingShaderOpcodeGenerator.h"
#include "rendering/compiler/include/renderingShaderProgramInstance.h"

namespace rendering
{
    namespace glsl
    {
        //---


    #define GLSL_FUNCTION GLSLCodeGenerator& gen, Function& function, const compiler::CodeNode* node, base::IFormatStream& f

        /// generation context for GLSL
        struct GLSLCodeGenerator
        {
        public:
            GLSLCodeGenerator(const compiler::opcodes::ShaderOpcodeGenerationContext& context, base::parser::IErrorReporter& err);
            ~GLSLCodeGenerator();

            bool generateShader(base::Buffer& outData, compiler::opcodes::ShaderStageDependencies& outDependencies);

        private:
            const compiler::opcodes::ShaderOpcodeGenerationContext& m_context;
            base::parser::IErrorReporter& m_err;
            bool m_hasErrors;

            void reportError(const base::parser::Location& loc, base::StringView<char> message);
            void reportWarning(const base::parser::Location& loc, base::StringView<char> message);

            base::HashSet<base::StringID> m_allowedBuiltInInputs;
            base::HashSet<base::StringID> m_allowedBuiltInOutputs;

            struct UsedInputOutput;

            struct InterfaceBlock
            {
                base::StringID m_blockName;
                base::StringID m_instanceName;
                bool m_array = false;
                base::Array<int> m_members;
            };

            struct UsedInputOutput
            {
                const compiler::DataParameter* m_param;
                compiler::DataType m_exportType;
                base::StringID m_name;
                //base::StringID m_printName;
                bool m_declare = true;
                bool m_buildIn = false;
                int m_assignedLayoutIndex = -1;
                int m_interfaceBlockIndex = -1;
            };

            struct UsedVertexInput
            {
                const compiler::DataParameter* m_param;
                base::StringID m_memberName;
                compiler::DataType m_memberType;
                base::StringID m_name; // _BindingPoint_name: .ie. "_SimpleVertex3D_pos";                
                int m_assignedLayoutIndex = -1;
            };

            struct UsedGroupShared
            {
                const compiler::DataParameter* m_param;
                base::StringID m_name;
                bool m_print = false;
            };

            struct FunctionLocalVar
            {
                base::StringID m_name;
                base::Array<std::pair<compiler::DataType, base::StringID>> m_names;
            };

            struct FunctionInputArg
            {
                const compiler::DataParameter* m_param;
                compiler::DataValue m_value;

            };

            struct FunctionKey
            {
                const compiler::Function* m_func = nullptr;
                base::Array<FunctionInputArg> m_constantArgs;
                bool entryFunction = false;

                //--

                FunctionKey() = default;
                FunctionKey(const compiler::Function* func);

                uint64_t calcHash() const;
                base::StringBuf buildUniqueName() const;
            };

            struct Function
            {
                FunctionKey m_key;
                base::StringBuf m_name; // build from key
                base::StringBuilder m_preamble;
                base::StringBuilder m_code;
                bool m_finishedCodeGeneration = false;
                bool m_needsForwardDeclaration = false;
                base::HashMap<base::StringID, FunctionLocalVar> m_localVariables;
                base::HashMap<const compiler::DataParameter*, base::StringID> m_localVariablesParamMap;

                Function(const FunctionKey& key)
                    : m_key(key)
                {
                    m_name = key.buildUniqueName();
                }
            };

            base::Array<Function*> m_exportedFunctions;
            base::HashMap<uint64_t, Function*> m_exportedFunctionsMap;
            base::HashSet<const compiler::CompositeType*> m_exportedCompositeTypes;
            base::Queue<Function*> m_exportedFunctionToProcess;
            base::HashMap<uint64_t, base::StringBuf> m_exportedConstants;

            base::Array<UsedInputOutput> m_usedInputs;
            base::Array<UsedInputOutput> m_usedOutputs;
            base::Array<UsedGroupShared> m_usedGroupShared;
            base::Array<UsedVertexInput> m_usedVertexInputs;
            base::Array<InterfaceBlock> m_inputInterfaceBlocks;
            base::Array<InterfaceBlock> m_outputInterfaceBlocks;

            base::HashSet<base::StringID> m_usedDescriptorConstants;
            base::HashSet<base::StringID> m_usedDescriptorResources;
            base::HashSet<base::StringID> m_usedDescriptors;

            base::StringBuilder m_blockPreamble; // GLSL pragma 
            base::StringBuilder m_blockTypes; // type declarations, mostly structures
            base::StringBuilder m_blockConstants; // constant values that were moved outside due to size (mostly arrays)

            base::HashMap<base::StringID, base::StringView<char>> m_nativeFunctionMapping;
            base::HashMap<base::StringID, base::StringView<char>> m_binaryFunctionMapping;
            base::HashMap<base::StringID, base::StringView<char>> m_unaryFunctionMapping;

            typedef void (*TCustomPrintFunction)(GLSL_FUNCTION);
            base::HashMap<base::StringID, TCustomPrintFunction> m_customFunctionPrinters;


            //--

            void exportInputs(base::IFormatStream& f);
            void exportOutputs(base::IFormatStream& f);
            void exportGroupShared(base::IFormatStream& f);
            void exportVertexInputs(base::IFormatStream& f);
            void exportDescriptors(base::IFormatStream& f);
            void exportResults(base::Buffer& outCode, compiler::opcodes::ShaderStageDependencies& outDependencies);

            //--

            Function* generateFunction(const FunctionKey& key);

            //--

            const UsedInputOutput* mapInput(const compiler::DataParameter* param);
            const UsedInputOutput* mapOutput(const compiler::DataParameter* param);
            const UsedInputOutput* mapBuiltin(const compiler::DataParameter* param);
            const UsedVertexInput* mapVertexInput(const compiler::DataParameter* param, base::StringID memberName);
            const UsedGroupShared* mapGroupShared(const compiler::DataParameter* param);

            int mapInputInterfaceBlock(base::StringID name);
            int mapOutputInterfaceBlock(base::StringID name);

            void printFunctionCode(Function& function, const compiler::CodeNode* node, base::IFormatStream& f);
            void printFunctionStatement(Function& function, uint32_t depth, const compiler::CodeNode* node, base::IFormatStream& f);
            void printFunctionSignature(Function& function, base::IFormatStream& f);
            void printFunctionPreamble(const Function& function, base::IFormatStream& f);
            void printArrayCounts(const compiler::ArrayCounts& counts, base::IFormatStream& f);
            void printCompoundName(const compiler::CompositeType& compound, base::IFormatStream& f);
            void printCompoundDeclaration(const compiler::CompositeType& compound, base::IFormatStream& f);
            void printCompoundMembers(const compiler::CompositeType& compound, base::IFormatStream& f);
            void printDataType(const base::parser::Location& location, const compiler::DataType& dataType, base::StringView<char> varName, base::IFormatStream& f);
            void printDataParam(const base::parser::Location& location, Function& func, const compiler::DataParameter* param, base::IFormatStream& f);
            void printDataConstant(const base::parser::Location& location, const compiler::DataValue& param, const compiler::DataType& type, base::IFormatStream& f);
            void printNativeCall(Function& function, const compiler::CodeNode* node, const compiler::INativeFunction* nativeFunc, base::IFormatStream& f);
            void printTextureDeclaration(const compiler::ResourceTableEntry& entry, base::IFormatStream& f);
            void printImageDeclaration(const compiler::ResourceTableEntry& entry, base::IFormatStream& f);

            void printStore(Function& function, const compiler::CodeNode* node, base::IFormatStream& f);
            void printArrayAcccess(Function& function, const compiler::CodeNode* node, base::IFormatStream& f);
            void printMemberAccess(Function& function, const compiler::CodeNode* node, base::IFormatStream& f);

            void printResourceStore(Function& function, const compiler::CodeNode* node, const compiler::CodeNode* resNode, const compiler::CodeNode* indexNode, const compiler::CodeNode* dataNode, base::IFormatStream& f);
            void printGenericStore(Function& function, const compiler::CodeNode* node, base::IFormatStream& f);
            void printGenericArrayAcces(Function& function, const compiler::CodeNode* node, base::IFormatStream& f);
            void printGenericMemberAccess(Function& function, const compiler::CodeNode* node, base::IFormatStream& f);

            //---

            void printGenericFunctionArgs(uint32_t firstArg, Function& function, const compiler::CodeNode* node, base::IFormatStream& f);

            static void PrintFuncSaturate(GLSL_FUNCTION);
            static void PrintFuncAtan2(GLSL_FUNCTION);
            static void PrintFuncMatrixVectorMul(GLSL_FUNCTION);
            static void PrintFuncVectorMatrimMul(GLSL_FUNCTION);
            static void PrintFuncLessThen(GLSL_FUNCTION);
            static void PrintFuncLessEqual(GLSL_FUNCTION);
            static void PrintFuncEqual(GLSL_FUNCTION);
            static void PrintFuncNotEqual(GLSL_FUNCTION);
            static void PrintFuncGreaterEqual(GLSL_FUNCTION);
            static void PrintFuncGreaterThen(GLSL_FUNCTION);

            //static void PrintFuncBufferLoad(GLSL_FUNCTION);
            //static void PrintFuncBufferStore(GLSL_FUNCTION);

            void printAtomicFunctionCore(Function& function, base::StringView<char> functionStem, const compiler::CodeNode* node, base::IFormatStream& f);

            static void PrintFuncAtomicIncrement(GLSL_FUNCTION);
            static void PrintFuncAtomicDecrement(GLSL_FUNCTION);
            static void PrintFuncAtomicAdd(GLSL_FUNCTION);
            static void PrintFuncAtomicSubtract(GLSL_FUNCTION);
            static void PrintFuncAtomicMin(GLSL_FUNCTION);
            static void PrintFuncAtomicMax(GLSL_FUNCTION);
            static void PrintFuncAtomicAnd(GLSL_FUNCTION);
            static void PrintFuncAtomicOr(GLSL_FUNCTION);
            static void PrintFuncAtomicXor(GLSL_FUNCTION);
            static void PrintFuncAtomicExchange(GLSL_FUNCTION);
            static void PrintFuncAtomicCompareSwap(GLSL_FUNCTION);

            static void PrintFuncTexture(GLSL_FUNCTION);
            static void PrintFuncTextureGather(GLSL_FUNCTION);
            static void PrintFuncTextureGatherOffset(GLSL_FUNCTION);
            static void PrintFuncTextureLod(GLSL_FUNCTION);
            static void PrintFuncTextureLodOffset(GLSL_FUNCTION);
            static void PrintFuncTextureBias(GLSL_FUNCTION);
            static void PrintFuncTextureBiasOffset(GLSL_FUNCTION);
            static void PrintFuncTextureSizeLod(GLSL_FUNCTION);

            static void PrintFuncTextureLoadLod(GLSL_FUNCTION);
            static void PrintFuncTextureLoadLodSample(GLSL_FUNCTION);
            static void PrintFuncTextureLoadLodOffset(GLSL_FUNCTION);
            static void PrintFuncTextureLoadLodOffsetSample(GLSL_FUNCTION);

            static void PrintFuncTextureDepthCompare(GLSL_FUNCTION);
            static void PrintFuncTextureDepthCompareOffset(GLSL_FUNCTION);

            static void PrintFuncTexelLoadSample(GLSL_FUNCTION);
            static void PrintFuncTexelLoad(GLSL_FUNCTION);
            static void PrintFuncTexelStore(GLSL_FUNCTION);
            static void PrintFuncTexelStoreSample(GLSL_FUNCTION);
            static void PrintFuncTexelSize(GLSL_FUNCTION);
        };

        //---

        /// generator for the shader opcodes
        class RENDERING_SHADER_GLSL_API GLSLOpcodeGenerator : public compiler::opcodes::IShaderOpcodeGenerator
        {
            RTTI_DECLARE_VIRTUAL_CLASS(GLSLOpcodeGenerator, compiler::opcodes::IShaderOpcodeGenerator);

        public:
            GLSLOpcodeGenerator();
            virtual ~GLSLOpcodeGenerator();

            /// build resource entries to use in this shader
            virtual bool buildResourceBinding(const base::Array<const compiler::ResourceTable*>& resourceTables,
                compiler::opcodes::ShaderResourceBindings& outBinding, base::parser::IErrorReporter& err) const override final;

            /// generate platform dependent opcodes
            /// NOTE: function must be thread safe and reentrant
            virtual bool generateOpcodes(const compiler::opcodes::ShaderOpcodeGenerationContext& context,
                base::Buffer& outData, compiler::opcodes::ShaderStageDependencies& outDependencies, base::parser::IErrorReporter& err) const override final;

        private:
            bool m_emitSymbolNames; // emit the names of generated symbols - makes the shaders much bigger
            bool m_emitLineNumbers; // emit the source code information (file : line number) - makes the shaders HUGHE, only for debug
            bool m_dumpInputShaders; // dump the input flattened shader content
            bool m_dumpOutputShaders; // dump the generated SPIRV shader content

            base::io::AbsolutePath m_dumpDirectory; // where are the shaders dumped
        };

    } // glsl
} // rendering
