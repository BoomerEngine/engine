/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader\stubs #]
***/

#ifndef DECLARE_GPU_SHADER_STUB
	#define DECLARE_GPU_SHADER_STUB(x) struct Stub##x;
#endif

	DECLARE_GPU_SHADER_STUB(File)
	DECLARE_GPU_SHADER_STUB(Attribute)

	DECLARE_GPU_SHADER_STUB(Program)
	DECLARE_GPU_SHADER_STUB(Stage)

	DECLARE_GPU_SHADER_STUB(ScalarTypeDecl)
	DECLARE_GPU_SHADER_STUB(VectorTypeDecl)
	DECLARE_GPU_SHADER_STUB(MatrixTypeDecl)
	DECLARE_GPU_SHADER_STUB(ArrayTypeDecl)
	DECLARE_GPU_SHADER_STUB(StructTypeDecl)

	DECLARE_GPU_SHADER_STUB(SamplerState)
	DECLARE_GPU_SHADER_STUB(RenderStates)
	DECLARE_GPU_SHADER_STUB(GlobalConstant)

	DECLARE_GPU_SHADER_STUB(Struct)
	DECLARE_GPU_SHADER_STUB(StructMember)

	DECLARE_GPU_SHADER_STUB(Descriptor)
	DECLARE_GPU_SHADER_STUB(DescriptorMemberConstantBuffer)
	DECLARE_GPU_SHADER_STUB(DescriptorMemberConstantBufferElement)
	DECLARE_GPU_SHADER_STUB(DescriptorMemberFormatBuffer)
	DECLARE_GPU_SHADER_STUB(DescriptorMemberStructuredBuffer)
	DECLARE_GPU_SHADER_STUB(DescriptorMemberSampledImage)
	DECLARE_GPU_SHADER_STUB(DescriptorMemberImage)
	DECLARE_GPU_SHADER_STUB(DescriptorMemberSampler)
	DECLARE_GPU_SHADER_STUB(DescriptorMemberSampledImageTable) // bindless textures

	DECLARE_GPU_SHADER_STUB(BuiltInVariable)
	DECLARE_GPU_SHADER_STUB(StageInput)
	DECLARE_GPU_SHADER_STUB(StageOutput)
	DECLARE_GPU_SHADER_STUB(SharedMemory)
	DECLARE_GPU_SHADER_STUB(VertexInputElement)
	DECLARE_GPU_SHADER_STUB(VertexInputStream)

	DECLARE_GPU_SHADER_STUB(Function)
	DECLARE_GPU_SHADER_STUB(FunctionParameter)

	DECLARE_GPU_SHADER_STUB(ScopeLocalVariable)

	DECLARE_GPU_SHADER_STUB(OpcodeLoad)
	DECLARE_GPU_SHADER_STUB(OpcodeStore)
	DECLARE_GPU_SHADER_STUB(OpcodeAccessArray)
	DECLARE_GPU_SHADER_STUB(OpcodeAccessMember)
	DECLARE_GPU_SHADER_STUB(OpcodeSwizzle)
	DECLARE_GPU_SHADER_STUB(OpcodeConstant)
	DECLARE_GPU_SHADER_STUB(OpcodeCast)
	DECLARE_GPU_SHADER_STUB(OpcodeResourceRef) // res or res[index] for tables
	DECLARE_GPU_SHADER_STUB(OpcodeResourceLoad) // load(res, addr)
	DECLARE_GPU_SHADER_STUB(OpcodeResourceStore) // store(res, addr, value)
	DECLARE_GPU_SHADER_STUB(OpcodeResourceElement) // res[addr]
	DECLARE_GPU_SHADER_STUB(OpcodeCreateVector)
	DECLARE_GPU_SHADER_STUB(OpcodeCreateMatrix)
	DECLARE_GPU_SHADER_STUB(OpcodeCreateArray)
	DECLARE_GPU_SHADER_STUB(OpcodeDataRef)
	DECLARE_GPU_SHADER_STUB(OpcodeScope)
	DECLARE_GPU_SHADER_STUB(OpcodeVariableDeclaration)
	DECLARE_GPU_SHADER_STUB(OpcodeNativeCall)
	DECLARE_GPU_SHADER_STUB(OpcodeCall)
	DECLARE_GPU_SHADER_STUB(OpcodeIfElse)
	DECLARE_GPU_SHADER_STUB(OpcodeLoop)
	DECLARE_GPU_SHADER_STUB(OpcodeReturn)
	DECLARE_GPU_SHADER_STUB(OpcodeBreak)
	DECLARE_GPU_SHADER_STUB(OpcodeContinue)
	DECLARE_GPU_SHADER_STUB(OpcodeExit)

#undef DECLARE_GPU_SHADER_STUB
