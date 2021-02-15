/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/device/include/renderingShaderStubs.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//---

			class ShaderSharedBindings
			{
			public:
				ShaderSharedBindings(const shader::StubProgram* program);

				base::HashMap<const shader::StubDescriptorMember*, short> m_descriptorResourceMapping;
				base::HashMap<const shader::StubStageInput*, short> m_stageInputMappings;
				base::HashMap<const shader::StubStageOutput*, short> m_stageOutputMapping;
			};

			class ShaderCodePrinter : public base::NoCopy
			{
			public:
				ShaderCodePrinter(const shader::StubProgram* program, const shader::StubStage* stage, const ShaderSharedBindings& sharedBindings, ShaderFeatureMask supportedFeatures);

				void printCode(base::StringBuilder& f);

			private:
				const shader::StubProgram* m_program = nullptr;
				const shader::StubStage* m_stage = nullptr;

				const ShaderSharedBindings& m_sharedBindings;

				ShaderStage m_stageIndex = ShaderStage::Invalid;

				ShaderFeatureMask m_featureMask;

				base::HashMap<const base::IStub*, base::StringBuf> m_paramNames;

				//--

				void printHeader(base::StringBuilder& f);
				void printStageAttributes(base::StringBuilder& f);
				void printVertexInputs(base::StringBuilder& f);
				void printStructureDeclarations(base::StringBuilder& f);
				void printGlobalConstants(base::StringBuilder& f);
				void printResources(base::StringBuilder& f);
				void printBuiltIns(base::StringBuilder& f);
				void printStageInputs(base::StringBuilder& f);
				void printStageOutputs(base::StringBuilder& f);
				void printSharedMemory(base::StringBuilder& f);
				void printFunctionsForwardDeclarations(base::StringBuilder& f);
				void printFunctions(base::StringBuilder& f);

				void printConstant(base::StringBuilder& f, const shader::StubTypeDecl* type, const uint32_t*& data);
				void printType(base::StringBuilder& f, const shader::StubTypeDecl* type, base::StringView varName = "");

				void printFunction(base::StringBuilder& f, const shader::StubFunction* func);
				void printHeader(base::StringBuilder& f, const shader::StubFunction* func);
				void printStatement(base::StringBuilder& f, const shader::StubOpcode* op, int depth);
				void printVariableDeclaration(base::StringBuilder& f, const shader::StubOpcodeVariableDeclaration* var);
				void printExpression(base::StringBuilder& f, const shader::StubOpcode* op);
				void printExpressionStore(base::StringBuilder& f, const shader::StubOpcodeStore* op);
				void printExpressionNativeCall(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printExpressionCall(base::StringBuilder& f, const shader::StubOpcodeCall* op);
				void printExpressionDataRef(base::StringBuilder& f, const shader::StubOpcodeDataRef* op);
				void printExpressionReadSwizzle(base::StringBuilder& f, const shader::StubOpcodeSwizzle* op);
				void printExpressionAccessArray(base::StringBuilder& f, const shader::StubOpcodeAccessArray* op);
				void printExpressionAccessMember(base::StringBuilder& f, const shader::StubOpcodeAccessMember* op);
				void printExpressionCreateVector(base::StringBuilder& f, const shader::StubOpcodeCreateVector* op);
				void printExpressionCreateMatrix(base::StringBuilder& f, const shader::StubOpcodeCreateMatrix* op);
				void printExpressionCreateArray(base::StringBuilder& f, const shader::StubOpcodeCreateArray* op);
				void printExpressionConstant(base::StringBuilder& f, const shader::StubOpcodeConstant* op);
				void printExpressionCast(base::StringBuilder& f, const shader::StubOpcodeCast* op);
				void printExpressionResourceLoad(base::StringBuilder& f, const shader::StubOpcodeResourceLoad* op);
				void printExpressionResourceStore(base::StringBuilder& f, const shader::StubOpcodeResourceStore* op);
				void printExpressionResourceElement(base::StringBuilder& f, const shader::StubOpcodeResourceElement* op);
				void printExpressionResourceRef(base::StringBuilder& f, const shader::StubOpcodeResourceRef* op);

				//--

				typedef void (ShaderCodePrinter::*TCustomPrintFunction)(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);

				struct Expr
				{
					const shader::StubOpcode* op = nullptr;
					ShaderCodePrinter* printer = nullptr;

					INLINE void print(base::IFormatStream& f) const
					{
						printer->printExpression(static_cast<base::StringBuilder&>(f), op);
					}
				};

				INLINE Expr Arg(const shader::StubOpcode* op_)
				{
					Expr ret;
					ret.op = op_;
					ret.printer = this;
					return ret;
				}

				struct NativeFunctionMapping
				{
					const char* name = nullptr;
					TCustomPrintFunction func = nullptr;
					bool op = false;

					INLINE NativeFunctionMapping() {};
					INLINE NativeFunctionMapping(const char* func) : name(func), op(strlen(func) <= 2) {};
					INLINE NativeFunctionMapping(TCustomPrintFunction ptr) : func(ptr) {};
				};

				base::HashMap<base::StringID, NativeFunctionMapping> m_functions;
				void initializeFunctions();

				void printFuncSaturate(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncMod(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncSelect(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);

				void printFuncCompareStd(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op, const char* scalarOp, const char* vectorFunc);
				void printFuncNotEqual(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncEqual(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncGreaterEqual(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncGreaterThen(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncLessEqual(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncLessThen(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);

				void printFuncAtomicStd(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op, base::StringView stem);
				void printFuncAtomicIncrement(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncAtomicDecrement(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncAtomicAdd(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncAtomicSubtract(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncAtomicMin(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncAtomicMax(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncAtomicOr(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncAtomicAnd(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncAtomicXor(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncAtomicExchange(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncAtomicCompareSwap(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);

				void printFuncTextureGatherOffset(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncTextureGather(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncTextureDepthCompare(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);
				void printFuncTextureDepthCompareOffset(base::StringBuilder& f, const shader::StubOpcodeNativeCall* op);

				//--
			};

			//--

		} // gl4
    } // api
} // rendering

