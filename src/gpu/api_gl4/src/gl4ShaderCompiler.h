/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/device/include/renderingShaderStubs.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

//---

class ShaderSharedBindings
{
public:
	ShaderSharedBindings(const shader::StubProgram* program);

	HashMap<const shader::StubDescriptorMember*, short> m_descriptorResourceMapping;
	HashMap<const shader::StubStageInput*, short> m_stageInputMappings;
	HashMap<const shader::StubStageOutput*, short> m_stageOutputMapping;
};

class ShaderCodePrinter : public NoCopy
{
public:
	ShaderCodePrinter(const shader::StubProgram* program, const shader::StubStage* stage, const ShaderSharedBindings& sharedBindings, ShaderFeatureMask supportedFeatures);

	void printCode(StringBuilder& f);

private:
	const shader::StubProgram* m_program = nullptr;
	const shader::StubStage* m_stage = nullptr;

	const ShaderSharedBindings& m_sharedBindings;

	ShaderStage m_stageIndex = ShaderStage::Invalid;

	ShaderFeatureMask m_featureMask;

	HashMap<const IStub*, StringBuf> m_paramNames;

	//--

	void printHeader(StringBuilder& f);
	void printStageAttributes(StringBuilder& f);
	void printVertexInputs(StringBuilder& f);
	void printStructureDeclarations(StringBuilder& f);
	void printGlobalConstants(StringBuilder& f);
	void printResources(StringBuilder& f);
	void printBuiltIns(StringBuilder& f);
	void printStageInputs(StringBuilder& f);
	void printStageOutputs(StringBuilder& f);
	void printSharedMemory(StringBuilder& f);
	void printFunctionsForwardDeclarations(StringBuilder& f);
	void printFunctions(StringBuilder& f);

	void printConstant(StringBuilder& f, const shader::StubTypeDecl* type, const uint32_t*& data);
	void printType(StringBuilder& f, const shader::StubTypeDecl* type, StringView varName = "");

	void printFunction(StringBuilder& f, const shader::StubFunction* func);
	void printHeader(StringBuilder& f, const shader::StubFunction* func);
	void printStatement(StringBuilder& f, const shader::StubOpcode* op, int depth);
	void printVariableDeclaration(StringBuilder& f, const shader::StubOpcodeVariableDeclaration* var);
	void printExpression(StringBuilder& f, const shader::StubOpcode* op);
	void printExpressionStore(StringBuilder& f, const shader::StubOpcodeStore* op);
	void printExpressionNativeCall(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printExpressionCall(StringBuilder& f, const shader::StubOpcodeCall* op);
	void printExpressionDataRef(StringBuilder& f, const shader::StubOpcodeDataRef* op);
	void printExpressionReadSwizzle(StringBuilder& f, const shader::StubOpcodeSwizzle* op);
	void printExpressionAccessArray(StringBuilder& f, const shader::StubOpcodeAccessArray* op);
	void printExpressionAccessMember(StringBuilder& f, const shader::StubOpcodeAccessMember* op);
	void printExpressionCreateVector(StringBuilder& f, const shader::StubOpcodeCreateVector* op);
	void printExpressionCreateMatrix(StringBuilder& f, const shader::StubOpcodeCreateMatrix* op);
	void printExpressionCreateArray(StringBuilder& f, const shader::StubOpcodeCreateArray* op);
	void printExpressionConstant(StringBuilder& f, const shader::StubOpcodeConstant* op);
	void printExpressionCast(StringBuilder& f, const shader::StubOpcodeCast* op);
	void printExpressionResourceLoad(StringBuilder& f, const shader::StubOpcodeResourceLoad* op);
	void printExpressionResourceStore(StringBuilder& f, const shader::StubOpcodeResourceStore* op);
	void printExpressionResourceElement(StringBuilder& f, const shader::StubOpcodeResourceElement* op);
	void printExpressionResourceRef(StringBuilder& f, const shader::StubOpcodeResourceRef* op);

	//--

	typedef void (ShaderCodePrinter::*TCustomPrintFunction)(StringBuilder& f, const shader::StubOpcodeNativeCall* op);

	struct Expr
	{
		const shader::StubOpcode* op = nullptr;
		ShaderCodePrinter* printer = nullptr;

		INLINE void print(IFormatStream& f) const
		{
			printer->printExpression(static_cast<StringBuilder&>(f), op);
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

	HashMap<StringID, NativeFunctionMapping> m_functions;
	void initializeFunctions();

	void printFuncSaturate(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncMod(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncSelect(StringBuilder& f, const shader::StubOpcodeNativeCall* op);

	void printFuncCompareStd(StringBuilder& f, const shader::StubOpcodeNativeCall* op, const char* scalarOp, const char* vectorFunc);
	void printFuncNotEqual(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncEqual(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncGreaterEqual(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncGreaterThen(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncLessEqual(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncLessThen(StringBuilder& f, const shader::StubOpcodeNativeCall* op);

	void printFuncAtomicStd(StringBuilder& f, const shader::StubOpcodeNativeCall* op, StringView stem);
	void printFuncAtomicIncrement(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncAtomicDecrement(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncAtomicAdd(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncAtomicSubtract(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncAtomicMin(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncAtomicMax(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncAtomicOr(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncAtomicAnd(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncAtomicXor(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncAtomicExchange(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncAtomicCompareSwap(StringBuilder& f, const shader::StubOpcodeNativeCall* op);

	void printFuncTextureGatherOffset(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncTextureGather(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncTextureDepthCompare(StringBuilder& f, const shader::StubOpcodeNativeCall* op);
	void printFuncTextureDepthCompareOffset(StringBuilder& f, const shader::StubOpcodeNativeCall* op);

	//--
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
