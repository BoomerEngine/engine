/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader\stubs #]
***/

#pragma once

#include "core/object/include/stub.h"
#include "core/object/include/stubFactory.h"

#include "samplerState.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::shader)

//---

struct Stub;
struct StubTypeDecl;
struct StubOpcode;
struct StubDescriptorMember;

#include "shaderStubsCodes.inl"

//---

enum class StubType : uint8_t
{
	None = 0,

	#define DECLARE_GPU_SHADER_STUB(x) x,
	#include "shaderStubsCodes.inl"
};

//---

struct GPU_DEVICE_API ComponentSwizzle
{
	static const uint32_t MAX_COMPONENTS = 4;

	char mask[MAX_COMPONENTS];

	//--

	ComponentSwizzle(); // undefined
	ComponentSwizzle(StringView mask); // assets if something didn't pass right as it's only for internal use
			
	// is this a valid swizzle ?
	INLINE bool empty() const { return mask[0] == 0; }

	//--

	// get the write mask, returns 0 if the write mask is invalid
	uint8_t writeMask() const;

	// get number of components defined in the swizzle, determines the output type
	uint8_t outputComponents(uint8_t* outList = nullptr) const;

	// get number of input components used, determines the minimum size of input type
	uint8_t inputComponents() const;

	//--

	// get a string representation (for debug and error printing)
	void print(IFormatStream& f) const;
	void write(IStubWriter& f) const;
	void read(IStubReader& f);
};

//---

enum class ScalarType : uint8_t
{
	Void, // a void type

	Half, // 16-bit floating point scalar
	Float, // 32-bit floating point scalar
	Double, // 64-bit floating point scalar

	Int, // 32-bit signed integer scalar
	Uint, // 32-bit unsigned integer scalar
	Int64, // 64-bit signed integer scalar
	Uint64, // 64-bit unsigned integer scalar
	Boolean, // boolean type (represented as 32-bit unsigned value with 0=false !=0=true)
};

//---

struct GPU_DEVICE_API StubLocation
{
	const StubFile* file = nullptr;
	uint32_t line = 0;

	void write(IStubWriter& f) const;
	void read(IStubReader& f);

	void print(IFormatStream& f) const;
};

//---

class GPU_DEVICE_API StubDebugPrinter : public IFormatStream
{
public:
	StubDebugPrinter(IFormatStream& f);

	void enableOutput();

	void printChild(const char* name, const Stub* s);
	void printChildArray(const char* name, const Stub* const* ptr, uint32_t count);
	void printChildRefArray(const char* name, const Stub* const* ptr, uint32_t count);

	template< typename T >
	INLINE void printChildArray(const char* name, const StubPseudoArray<T>& arr)
	{
		printChildArray(name, (const Stub* const*)arr.elems, arr.size());
	}

	template< typename T >
	INLINE void printChildRefArray(const char* name, const StubPseudoArray<T>& arr)
	{
		printChildRefArray(name, (const Stub* const*)arr.elems, arr.size());
	}

	StubDebugPrinter& printRef(const char* name, const Stub* s);

	virtual IFormatStream& append(const char* str, uint32_t len = INDEX_MAX) override final;

private:
	HashMap<const Stub*, int> m_stubs;
	IFormatStream& m_printer;
	bool m_pendingLineEnd = false;
	bool m_outputEnabled = false;
	int m_depth = 0;

	int index(const Stub* stub);
};

//---

struct GPU_DEVICE_API Stub : public IStub
{
	StubLocation location; // saved in some functions

	virtual void write(IStubWriter& f) const = 0;
	virtual void read(IStubReader& f) = 0;
	virtual void dump(StubDebugPrinter& f) const = 0;
	virtual void postLoad();

	//--

	static const StubFactory& Factory();

	//--

	#define DECLARE_GPU_SHADER_STUB(x) \
	virtual Stub##x* as##x() { return nullptr; } \
	virtual const Stub##x* as##x() const { return nullptr; }
	#include "shaderStubsCodes.inl"

};

//---

struct GPU_DEVICE_API StubFile : public Stub
{
	STUB_CLASS(File);

	StringView depotPath;
	uint64_t contentCrc = 0;
	uint64_t contentTimestamp = 0;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubAttribute : public Stub
{
	STUB_CLASS(Attribute);

	StringID name;
	StringView value;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

//---

struct GPU_DEVICE_API StubStage : public Stub
{
	STUB_CLASS(Stage);

	ShaderStage stage = ShaderStage::Invalid; // pipeline stage we are for
	StubPseudoArray<StubTypeDecl> types; // stage types - subset of global types used here used in this stage
	StubPseudoArray<StubStruct> structures; // stage structures - subset of global types used in this stage
	StubPseudoArray<StubStageInput> inputs; // stage inputs - ALL
	StubPseudoArray<StubStageOutput> outputs; // stage outputs - ALL
	StubPseudoArray<StubSharedMemory> sharedMemory; // shared memory (compute mostly)
	StubPseudoArray<StubDescriptorMember> descriptorMembers; // used descriptor members (referenced resources, constant buffers etc)
	StubPseudoArray<StubBuiltInVariable> builtins; // used descriptor members (referenced resources, constant buffers etc)
	StubPseudoArray<StubVertexInputStream> vertexStreams; // vertex shader streams (yeah, special case but convenient to put it here)
	StubPseudoArray<StubGlobalConstant> globalConstants; // big (arrays) global constants used in the shader (they can't be embedded in functions)
	StubPseudoArray<StubSamplerState> samplers; // referenced static samplers
	StubPseudoArray<StubFunction> functions; // all functions in this stage
	StubPseudoArray<StubFunction> functionsRefs; // functions that must be forward declared

	const StubFunction* entryFunction = nullptr; // entry point function (main)

	ShaderFeatureMask featureMask; // local feature mask

	StubStage();

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubProgram : public Stub
{
	STUB_CLASS(Program);

	StringView depotPath; // path to the original file being compiled
	StringView options; // compilation options (defines, material configuration, etc)
	StubPseudoArray<StubFile> files; // all used files
	StubPseudoArray<StubTypeDecl> types; // all know types
	StubPseudoArray<StubStruct> structures; // all know structures
	StubPseudoArray<StubDescriptor> descriptors; // shared descriptors
	StubPseudoArray<StubSamplerState> samplers; // shared static samplers
	StubPseudoArray<StubVertexInputStream> vertexStreams; // vertex streams (forwarded from vertex shader since it's a global state any way)
	StubPseudoArray<StubStage> stages; // shader stages, constant size
	const StubRenderStates* renderStates = nullptr; // custom render states

	ShaderFeatureMask featureMask; // required shader features

	StubProgram();

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubTypeDecl : public Stub
{
	virtual void print(IFormatStream& f) const = 0;
};

//--

struct GPU_DEVICE_API StubScalarTypeDecl : public StubTypeDecl
{
	STUB_CLASS(ScalarTypeDecl);

	ScalarType type;

	virtual void print(IFormatStream& f) const override final;
	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

struct GPU_DEVICE_API StubVectorTypeDecl : public StubTypeDecl
{
	STUB_CLASS(VectorTypeDecl);

	ScalarType type;
	uint8_t componentCount = 0; // 1 for scalar types, 2-4 for vectors

	virtual void print(IFormatStream& f) const override final;
	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

struct GPU_DEVICE_API StubMatrixTypeDecl : public StubTypeDecl
{
	STUB_CLASS(MatrixTypeDecl);

	ScalarType type;
	uint8_t componentCount = 0; // 1 for scalar types, 2-4 for vectors
	uint8_t rowCount = 0; // 1 for vectors/scalars, 2-4 for matrices

	virtual void print(IFormatStream& f) const override final;
	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubArrayTypeDecl : public StubTypeDecl
{
	STUB_CLASS(ArrayTypeDecl);

	uint32_t count = 0; // 0 for unlimited array
	const StubTypeDecl* innerType = nullptr;

	virtual void print(IFormatStream& f) const override final;
	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubStructTypeDecl : public StubTypeDecl
{
	STUB_CLASS(StructTypeDecl);

	const StubStruct* structType = nullptr;

	virtual void print(IFormatStream& f) const override final;
	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubStructMember : public Stub
{
	STUB_CLASS(StructMember);

	StringID name;
	uint16_t index = 0; // index in the parent structure

	uint32_t linearAlignment = 0; // member alignment that was used (or is needed)
	uint32_t linearOffset = 0; // memory offset for this member int the parent structure
	uint32_t linearSize = 0; // physical data size
	uint32_t linearArrayCount = 0; // for arrays this is the number of array elements
	uint32_t linearArrayStride = 0; // for arrays this is the stride of the array

	const StubTypeDecl* type = nullptr;
	StubPseudoArray<StubAttribute> attributes;

	const StubStruct* owner = nullptr;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubStruct : public Stub
{
	STUB_CLASS(Struct);

	StringID name;
	uint32_t size = 0;
	uint32_t alignment = 0;
	StubPseudoArray<StubStructMember> members;
	StubPseudoArray<StubAttribute> attributes;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--		

struct GPU_DEVICE_API StubSamplerState : public Stub
{
	STUB_CLASS(SamplerState);

	int index = 0;
	StringID name;
	StubPseudoArray<StubAttribute> attributes;

	SamplerState state;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};		

//--

struct GPU_DEVICE_API StubRenderStates : public Stub
{
	STUB_CLASS(RenderStates);

	GraphicsRenderStatesSetup states;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubDescriptorMember : public Stub
{
	//STUB_CLASS(DescriptorMember);

	const StubDescriptor* descriptor = nullptr;

	uint8_t index = 0;
	StringID name;
	StubPseudoArray<StubAttribute> attributes;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override {};
};

//--

struct GPU_DEVICE_API StubDescriptorMemberConstantBufferElement : public Stub
{
	STUB_CLASS(DescriptorMemberConstantBufferElement);

	StringID name;

	const StubDescriptorMemberConstantBuffer* constantBuffer = nullptr;

	uint32_t linearAlignment = 0; // member alignment that was used (or is needed)
	uint32_t linearOffset = 0; // memory offset for this member int the parent structure
	uint32_t linearSize = 0; // physical data size
	uint32_t linearArrayCount = 0; // for arrays this is the number of array elements
	uint32_t linearArrayStride = 0; // for arrays this is the stride of the array

	const StubTypeDecl* type = nullptr;
	StubPseudoArray<StubAttribute> attributes;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;

};

//--

struct GPU_DEVICE_API StubDescriptorMemberConstantBuffer : public StubDescriptorMember
{
	STUB_CLASS(DescriptorMemberConstantBuffer);

	uint32_t size = 0;
	StubPseudoArray<StubDescriptorMemberConstantBufferElement> elements;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubDescriptorMemberFormatBuffer : public StubDescriptorMember
{
	STUB_CLASS(DescriptorMemberFormatBuffer);

	ImageFormat format = ImageFormat::UNKNOWN;
	bool writable = false;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubDescriptorMemberStructuredBuffer : public StubDescriptorMember
{
	STUB_CLASS(DescriptorMemberStructuredBuffer);

	const StubStruct* layout = nullptr;
	uint16_t stride = 0;
	bool writable = false;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubDescriptorMemberSampledImage : public StubDescriptorMember
{
	STUB_CLASS(DescriptorMemberSampledImage);

	ImageViewType viewType = ImageViewType::View2D;
	ScalarType scalarType = ScalarType::Float;
	bool depth = false;
	bool multisampled = false;

	const StubDescriptorMemberSampler* dynamicSamplerDescriptorEntry = nullptr;
	const StubSamplerState* staticState = nullptr;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubDescriptorMemberImage : public StubDescriptorMember
{
	STUB_CLASS(DescriptorMemberImage);

	ImageViewType viewType = ImageViewType::View2D;
	ImageFormat format = ImageFormat::UNKNOWN;
	bool writable = false;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubDescriptorMemberSampledImageTable : public StubDescriptorMember
{
	STUB_CLASS(DescriptorMemberSampledImageTable);

	ImageViewType viewType = ImageViewType::View2D;
	bool depth = false;

	const StubDescriptorMemberSampler* dynamicSamplerDescriptorEntry = nullptr;
	const StubSamplerState* staticState = nullptr;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubDescriptorMemberSampler : public StubDescriptorMember
{
	STUB_CLASS(DescriptorMemberSampler);

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};		

//--

struct GPU_DEVICE_API StubDescriptor : public Stub
{
	STUB_CLASS(Descriptor);

	StringID name;

	StubPseudoArray<StubAttribute> attributes;
	StubPseudoArray<StubDescriptorMember> members;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubStageOutput : public Stub
{
	STUB_CLASS(StageOutput);

	StringID name;
	StringID bindingName;
	const StubTypeDecl* type = nullptr; // may be structure or array for GS
	const StubStageInput* nextStageInput = nullptr; // if next stage uses this output as an input this is the link to it

	StubPseudoArray<StubAttribute> attributes;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubStageInput : public Stub
{
	STUB_CLASS(StageInput);

	StringID name;
	StringID bindingName;
	const StubTypeDecl* type = nullptr; // may be structure or array for GS
	const StubStageOutput* prevStageOutput = nullptr; // if previous stage declared this output as an input this is the link to it (usually it's required)

	StubPseudoArray<StubAttribute> attributes;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubSharedMemory : public Stub
{
	STUB_CLASS(SharedMemory);

	StringID name;
	const StubTypeDecl* type = nullptr; // may be structure or array for GS

	StubPseudoArray<StubAttribute> attributes;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubGlobalConstant : public Stub
{
	STUB_CLASS(GlobalConstant);

	const StubTypeDecl* typeDecl = nullptr; // usually array
	uint16_t index = 0;
	ScalarType dataType = ScalarType::Void;
	uint32_t dataSize = 0; // size of stored data (may be many bytes for arrays)
	const void* data = nullptr; // embedded, usually math constant

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

enum class ShaderBuiltIn : uint8_t
{
	Invalid = 0,
	Position,
	PositionIn,
	PointSize,
	PointSizeIn,
	ClipDistance,
	ClipDistanceIn,
	VertexID,
	InstanceID,
	DrawID,
	BaseVertex,
	BaseInstance,
	PatchVerticesIn,
	PrimitiveID,
	PrimitiveIDIn,
	InvocationID,
	TessLevelOuter,
	TessLevelInner,
	TessCoord,
	FragCoord,
	FrontFacing,
	PointCoord,
	SampleID,
	SamplePosition,
	SampleMaskIn,
	SampleMask,
	Target0,
	Target1,
	Target2,
	Target3,
	Target4,
	Target5,
	Target6,
	Target7,
	Depth,
	Layer,
	ViewportIndex,
	NumWorkGroups,
	GlobalInvocationID,
	LocalInvocationID,
	WorkGroupID,
	LocalInvocationIndex,
};

typedef BitFlagsBase<ShaderBuiltIn, uint64_t> ShaderBuiltInMask;

struct GPU_DEVICE_API StubBuiltInVariable : public Stub
{
	STUB_CLASS(BuiltInVariable);

	ShaderBuiltIn builinType;
	const StubTypeDecl* dataType = nullptr; // may be structure or array for GS

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubVertexInputElement : public Stub
{
	STUB_CLASS(VertexInputElement);

	StringID name;

	uint8_t elementIndex = 0; // element index in the stream
	uint16_t elementOffset = 0; // offset in the stream
	uint16_t elementSize = 0; // size in the stream
	ImageFormat elementFormat = ImageFormat::UNKNOWN; // assigned data format (may be more compressed than data type, ie -> RGBA8_UNORM -> vec4)

	const StubVertexInputStream* stream = nullptr; // parent stream
	const StubTypeDecl* type = nullptr; // simple type

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubVertexInputStream : public Stub
{
	STUB_CLASS(VertexInputStream);

	StringID name;

	bool instanced = false;
	uint8_t streamIndex = 0; // stream index
	uint16_t streamSize = 0; // data size
	uint16_t streamStride = 0; // data stride

	StubPseudoArray<StubVertexInputElement> elements;
			
	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubFunction : public Stub
{
	STUB_CLASS(Function);

	StringID name; // folded name, ie. func_crap_23234

	const StubTypeDecl* returnType = nullptr;

	StubPseudoArray<StubAttribute> attributes;
	StubPseudoArray<StubFunctionParameter> parameters;

	const StubOpcode* code = nullptr;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubScopeLocalVariable : public Stub
{
	STUB_CLASS(ScopeLocalVariable);

	StringID name;
	const StubTypeDecl* type = nullptr;
	bool initialized = false;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//--

struct GPU_DEVICE_API StubFunctionParameter : public Stub
{
	STUB_CLASS(FunctionParameter);

	StringID name;
	bool reference = false; // in/out
	const StubTypeDecl* type = nullptr;

	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcode : public Stub
{
	virtual void write(IStubWriter& f) const override;
	virtual void read(IStubReader& f) override;
	virtual void dump(StubDebugPrinter& f) const override {};
};

//---

struct GPU_DEVICE_API StubOpcodeScope : public StubOpcode
{
	STUB_CLASS(OpcodeScope);

	StubPseudoArray<StubOpcode> statements;
	StubPseudoArray<StubScopeLocalVariable> locals; // collected, not printed

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeVariableDeclaration : public StubOpcode
{
	STUB_CLASS(OpcodeVariableDeclaration);

	const StubScopeLocalVariable* var = nullptr;
	const StubOpcode* init = nullptr; // can be empty			

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeLoad : public StubOpcode
{
	STUB_CLASS(OpcodeLoad);

	const StubOpcode* valueReferece = nullptr;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeStore : public StubOpcode
{
	STUB_CLASS(OpcodeStore);

	ComponentSwizzle mask;
	const StubOpcode* lvalue = nullptr;
	const StubOpcode* rvalue = nullptr;
	const StubTypeDecl* type = nullptr;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeConstant : public StubOpcode
{
	STUB_CLASS(OpcodeConstant);

	const StubTypeDecl* typeDecl = nullptr;
	ScalarType dataType = ScalarType::Void;
	uint32_t dataSize = 0; // size of stored data (may be many bytes for arrays)
	const void* data = nullptr; // embedded, usually math constant

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeResourceRef : public StubOpcode
{
	STUB_CLASS(OpcodeResourceRef);

	DeviceObjectViewType type = DeviceObjectViewType::Invalid;
	const StubDescriptorMember* descriptorEntry = nullptr;
	const StubOpcode* index = nullptr; // may be NULL, not null only for table based resources
	uint8_t numAddressComponents = 0;
			
	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeResourceLoad : public StubOpcode
{
	STUB_CLASS(OpcodeResourceLoad);

	const StubOpcodeResourceRef* resourceRef = nullptr;
	const StubOpcode* address = nullptr;
	uint8_t numAddressComponents = 0;
	uint8_t numValueComponents = 0;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeResourceStore : public StubOpcode
{
	STUB_CLASS(OpcodeResourceStore);

	const StubOpcodeResourceRef* resourceRef = nullptr;
	const StubOpcode* address = nullptr;
	const StubOpcode* value = nullptr;
	uint8_t numAddressComponents = 0;
	uint8_t numValueComponents = 0;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeResourceElement : public StubOpcode
{
	STUB_CLASS(OpcodeResourceElement);

	const StubOpcodeResourceRef* resourceRef = nullptr;
	const StubOpcode* address = nullptr;
	uint8_t numAddressComponents = 0;
	uint8_t numValueComponents = 0;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};
		

//---

struct GPU_DEVICE_API StubOpcodeCast : public StubOpcode
{
	STUB_CLASS(OpcodeCast);

	ScalarType generalType = ScalarType::Void;
	const StubTypeDecl* targetType = nullptr;
	const StubOpcode* value = nullptr;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeCreateVector : public StubOpcode
{
	STUB_CLASS(OpcodeCreateVector);

	const StubVectorTypeDecl* typeDecl = nullptr;
	StubPseudoArray<StubOpcode> elements;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeCreateMatrix: public StubOpcode
{
	STUB_CLASS(OpcodeCreateMatrix);

	const StubMatrixTypeDecl* typeDecl = nullptr;
	StubPseudoArray<StubOpcode> elements;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---
		
struct GPU_DEVICE_API StubOpcodeCreateArray : public StubOpcode
{
	STUB_CLASS(OpcodeCreateArray);

	const StubArrayTypeDecl* arrayTypeDecl = nullptr;
	StubPseudoArray<StubOpcode> elements;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeAccessArray : public StubOpcode
{
	STUB_CLASS(OpcodeAccessArray);

	const StubOpcode* arrayOp = nullptr;
	const StubOpcode* indexOp = nullptr; // if null then index is static (compile time constant) and is given directly
	const StubTypeDecl* arrayType = nullptr; // type of the array, usually a proper array but sometimes a resource
	int staticIndex = 0;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeAccessMember : public StubOpcode
{
	STUB_CLASS(OpcodeAccessMember);

	const StubOpcode* value = nullptr;
	const StubStructMember* member = nullptr;
	StringID name;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeSwizzle : public StubOpcode
{
	STUB_CLASS(OpcodeSwizzle);

	const StubOpcode* value = nullptr;
	uint8_t inputComponents = 0;
	uint8_t outputComponents = 0;
	ComponentSwizzle swizzle;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeDataRef : public StubOpcode
{
	STUB_CLASS(OpcodeDataRef);

	const Stub* stub = nullptr; // referenced stub, descriptor member, stage input, stage output, local function variable

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeNativeCall : public StubOpcode
{
	STUB_CLASS(OpcodeNativeCall);

	StringID name;
	const StubTypeDecl* returnType = nullptr;
	StubPseudoArray<StubOpcode> arguments;
	StubPseudoArray<StubTypeDecl> argumentTypes;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeCall : public StubOpcode
{
	STUB_CLASS(OpcodeCall);

	const StubFunction* func = nullptr;
	StubPseudoArray<StubOpcode> arguments;
	StubPseudoArray<StubTypeDecl> argumentTypes;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeIfElse : public StubOpcode
{
	STUB_CLASS(OpcodeIfElse);

	char branchHint = 0; // <0 flatten, >0 branch
	StubPseudoArray<StubOpcode> conditions; //
	StubPseudoArray<StubOpcode> statements; // same count as conditions
	const StubOpcode* elseStatement = nullptr; // else statement if no condition is taken

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeLoop : public StubOpcode
{
	STUB_CLASS(OpcodeLoop);

	char unrollHint = 0; // <0 don't unroll, >0 unroll
	short dependencyLength = 0; // 0 - not specified, <0 - infinite
			
	const StubOpcode* init = nullptr;
	const StubOpcode* condition = nullptr;
	const StubOpcode* increment = nullptr;
	const StubOpcode* body = nullptr;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeReturn : public StubOpcode
{
	STUB_CLASS(OpcodeReturn);

	const StubOpcode* value = nullptr;

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final;
};

//---

struct GPU_DEVICE_API StubOpcodeBreak : public StubOpcode
{
	STUB_CLASS(OpcodeBreak);


	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final {};
};

//---

struct GPU_DEVICE_API StubOpcodeContinue : public StubOpcode
{
	STUB_CLASS(OpcodeContinue);


	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final {};
};

//---

struct GPU_DEVICE_API StubOpcodeExit : public StubOpcode
{
	STUB_CLASS(OpcodeExit);

	virtual void write(IStubWriter& f) const override final;
	virtual void read(IStubReader& f) override final;
	virtual void dump(StubDebugPrinter& f) const override final {};
};

//---

// assemble human readable file name for any kind of dump file related to shader compilation
// ie. canvas_(HDR=1)_1231231.txt
extern GPU_DEVICE_API StringBuf AssembleDumpFileName(const StringView contextName, const StringView contextOptions, const StringView type);

// provide full dump path to a human readable file name for any kind of dump file related to shader compilation
// ie. Z:\projects\engine\.temp\shaders\canvas_(HDR=1)_1231231.txt
extern GPU_DEVICE_API StringBuf AssembleDumpFilePath(const StringView contextName, const StringView contextOptions, const StringView type);

//--

END_BOOMER_NAMESPACE_EX(gpu::shader)
