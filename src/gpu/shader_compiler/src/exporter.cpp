/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler\stubs #]
*/

#include "build.h"
#include "function.h"
#include "program.h"
#include "programInstance.h"
#include "codeLibrary.h"
#include "typeLibrary.h"
#include "typeUtils.h"
#include "functionFolder.h"
#include "exporter.h"
#include "nativeFunction.h"

#include "gpu/device/include/shaderStubs.h"
#include "gpu/device/include/shaderMetadata.h"
#include "core/object/include/stubBuilder.h"
#include "core/object/include/stubLoader.h"
#include "core/parser/include/textToken.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::compiler)

//--

ConfigProperty<bool> cvDumpstubs("Rendering.Shader", "DumpCompiledStubs", true);

//--

struct bunderSetup
{
	static const uint8_t MAX_s = (uint8_t)ShaderStage::MAX;

	struct Entry
	{
		const Function* func = nullptr;
		const ProgramInstance* pi = nullptr;
	};

	parser::Location location;
	Entry stages[MAX_s];
};

//--

static bool Extractbundle(const Exporteds& exports, CodeLibrary& code, bunderSetup& outBundleSetup, parser::IErrorReporter& err)
{
	bool valid = true;

	for (int i = 1; i < (int)ShaderStage::MAX; ++i)
	{
		if (const auto* program = exports.exports[i])
		{
			ProgramConstants constants; // injects ?

			if (const auto* main = program->findFunction("main"_id))
			{
				if (auto* pi = code.createProgramInstance(program->location(), program, constants, err))
				{
					outBundleSetup.location = program->location();
					outBundleSetup.stages[i].pi = pi;
					outBundleSetup.stages[i].func = main;
				}
				else
				{
					err.reportError(program->location(), TempString("Failed to create instance of shader '{}' required for exporting", program->name()));
					valid = false;
				}
			}
			else
			{
				err.reportError(program->location(), TempString("Exported shader '{}' should have main() function :)", program->name()));
				valid = false;
			}
		}
	}

	return valid;
}

static void CollectRenderStates(const Program* p, GraphicsRenderStatesSetup& outSetup)
{
	for (const auto* parent : p->parentPrograms())
		CollectRenderStates(parent, outSetup);

	for (const auto* states : p->staticRenderStates())
		outSetup.apply(states->state());
}

class StubExtractionHelper : public NoCopy
{
public:
	StubExtractionHelper(LinearAllocator& mem, const CodeLibrary& lib)
		: m_builder(mem, shader::Stub::Factory())
		, m_mem(mem)
		, m_lib(lib)
	{
	}

	~StubExtractionHelper()
	{
		clear();
	}

	void clear()
	{
		m_global = GlobalContext();
		for (auto& stage : m_stages)
			stage = StageContext();
	}

	shader::StubProgram* exportProgram(const bunderSetup& bundle, StringView contextPath, StringView contextOptions, parser::IErrorReporter& err)
	{
		clear();

		// build stages
		for (uint32_t i=0; i<NUM_STAGES; ++i)
		{
			const auto& sourceStage = bundle.stages[i];
			if (sourceStage.func)
			{
				FunctionFolder folder(m_mem, const_cast<CodeLibrary&>(m_lib)); // TODO: fix const cast

				// fold (optimize and suck-in constants) the function to it's final form 
				ProgramConstants emptyConstants;
				if (auto* finalFunc = folder.foldFunction(sourceStage.func, sourceStage.pi, emptyConstants, err))
				{
					m_activeStage = &m_stages[i];
					m_activeStage->stage = (ShaderStage)i;
					m_activeStage->sourceEntryFunction = sourceStage.func;
					m_activeStage->m_entryFunction = exportFunction(finalFunc);
				}
				else
				{
					return nullptr;
				}
			}
		}

		// export
		auto* program = m_builder.createStub<shader::StubProgram>();
		program->depotPath = contextPath;
		program->options = contextOptions;
		program->files = m_builder.createArray(m_global.m_fileMap.values());
		program->types = m_builder.createArray(m_global.m_typeMap.values());
		program->structures = m_builder.createArray(m_global.m_compositeMap.values());
		program->descriptors = m_builder.createArray(m_global.m_descriptorMap.values());
		program->vertexStreams = m_builder.createArray(m_global.m_vertexStreams.values());
		program->samplers = m_builder.createArray(m_global.m_staticSamplers.values());

		// stages
		bool valid = true;
		InplaceArray<const shader::StubStage*, 10> stages;
		shader::StubStage* prevStage = nullptr;
		for (uint32_t i = 0; i < NUM_STAGES; ++i)
		{
			shader::StubStage* stage = nullptr;
			if (m_stages[i].m_entryFunction)
			{
				const auto& localStage = m_stages[i];

				stage = m_builder.createStub<shader::StubStage>();
				stage->stage = (ShaderStage)i;
				stage->types = m_builder.createArray(localStage.m_types.keys());
				stage->structures = m_builder.createArray(localStage.m_composites.keys());
				stage->functions = m_builder.createArray(localStage.m_functionMap.values());
				stage->functionsRefs = m_builder.createArray(localStage.m_functionForwardRefs.keys());						
				stage->inputs = m_builder.createArray(localStage.m_stageInputs.keys());
				stage->outputs = m_builder.createArray(localStage.m_stageOutputs.keys());
				stage->sharedMemory = m_builder.createArray(localStage.m_sharedMemory.keys());
				stage->descriptorMembers = m_builder.createArray(localStage.m_descriptorMembers.keys());
				stage->builtins = m_builder.createArray(localStage.m_buildInVariables.keys());						
				stage->vertexStreams = m_builder.createArray(localStage.m_vertexStreams.keys());
				stage->globalConstants = m_builder.createArray(localStage.m_globalConstants.values());
				stage->samplers = m_builder.createArray(localStage.m_staticSamplersRefs.keys());
				stage->entryFunction = localStage.m_entryFunction;

				// flags
				if (localStage.usesControlFlowHints)
					stage->featureMask |= ShaderFeatureBit::ControlFlowHints;
				else if (localStage.usesUAVs && stage->stage == ShaderStage::Pixel)
					stage->featureMask |= ShaderFeatureBit::UAVPixelShader;
				else if (localStage.usesUAVs && (stage->stage != ShaderStage::Pixel && stage->stage != ShaderStage::Compute))
					stage->featureMask |= ShaderFeatureBit::UAVOutsidePixelShader;
				else if (localStage.usesDepthOutput && stage->stage == ShaderStage::Pixel)
					stage->featureMask |= ShaderFeatureBit::DepthModification;
				else if (localStage.usesDiscard && stage->stage == ShaderStage::Pixel)
					stage->featureMask |= ShaderFeatureBit::PixelDiscard;

				// merge program feature flags
				program->featureMask |= stage->featureMask;

				// link inputs with outputs
				if (prevStage != nullptr)
				{
					for (auto* inputParam : stage->inputs)
					{
						const shader::StubStageOutput* prevStageOutput = nullptr;
						for (auto* outputParam : prevStage->outputs)
						{
							if (outputParam->bindingName == inputParam->bindingName)
							{
								prevStageOutput = outputParam;
								break;
							}
						}

						if (prevStageOutput)
						{
							const_cast<shader::StubStageOutput*>(prevStageOutput)->nextStageInput = inputParam;
							const_cast<shader::StubStageInput*>(inputParam)->prevStageOutput = prevStageOutput;
						}
						else
						{
							for (auto it : localStage.m_parametersMap.pairs())
							{
								if (it.value == inputParam)
									err.reportError(it.key->loc, TempString("Current stage {} input parmeter {} (binding: {}) is not declared as output in previous stage",
										(ShaderStage)i, inputParam->name, inputParam->bindingName));
							}

							valid = false;
						}
					}

				}
				else if (!localStage.m_parametersMap.empty())
				{
					for (const auto* param : localStage.m_parametersMap.keys())
					{
						if (param->scope == DataParameterScope::StageInput)
						{
							err.reportError(param->loc, TempString("Current stage {} has no previous stage yet it's using stage input parmeter {}",
								(ShaderStage)i, param->name));
							valid = false;
						}
					}
				}

				prevStage = stage;
				stages.pushBack(stage);
			}
		}

		// do not continue if we are not valid
		if (!valid)
			return nullptr;

		// render states, exported only if we used pixel shader
		static const auto PixelstageIndex = (int)ShaderStage::Pixel;
		if (bundle.stages[PixelstageIndex].func)
		{
			// although render states are exported only if pixel shader is defined we assemble them from ALL programs, in order
			// this way the vertex/geometry shader can define topology, etc.

			GraphicsRenderStatesSetup finalStates;
			for (uint32_t i = 0; i <= PixelstageIndex; ++i)
			{
				if (bundle.stages[i].pi)
				{
					if (const auto* program = bundle.stages[i].pi->program())
						CollectRenderStates(program, finalStates);
				}
			}

			// store only if we indeed collected something
			if (finalStates != GraphicsRenderStatesSetup::DEFAULT_STATES())
			{
				TRACE_SPAM("Final collected render states:\n{}", finalStates);

				auto* stub = m_builder.createStub<shader::StubRenderStates>();
				stub->states = finalStates;
				program->renderStates = stub;
			}
		}

		program->stages = m_builder.createArray(stages);
		return program;
	}

private:
	StubBuilder m_builder;
	LinearAllocator& m_mem;
	const CodeLibrary& m_lib;

	struct GlobalContext
	{
		HashMap<StringView, const shader::StubFile*> m_fileMap;
		HashMap<DataType, shader::StubTypeDecl*> m_typeMap;
		HashMap<const CompositeType*, shader::StubStruct*> m_compositeMap;
		HashMap<const ResourceTable*, shader::StubDescriptor*> m_descriptorMap;
		HashMap<const DataParameter*, shader::StubVertexInputStream*> m_vertexStreams;
		HashMap<const StaticSampler*, shader::StubSamplerState*> m_staticSamplers;
	};

	struct StageContext
	{
		ShaderStage stage;
		const Function* sourceEntryFunction = nullptr;

		HashMap<const Function*, shader::StubFunction*> m_functionMap;
		HashSet<const shader::StubFunction*> m_functionForwardRefs;
		HashSet<const shader::StubTypeDecl*> m_types;
		HashSet<const shader::StubStruct*> m_composites;
		HashSet<const shader::StubDescriptor*> m_descriptors;
		HashSet<const shader::StubDescriptorMember*> m_descriptorMembers;
		HashSet<const shader::StubStageInput*> m_stageInputs;
		HashSet<const shader::StubStageOutput*> m_stageOutputs;
		HashSet<const shader::StubSharedMemory*> m_sharedMemory;
		HashSet<const shader::StubBuiltInVariable*> m_buildInVariables;
		HashSet<const shader::StubVertexInputStream*> m_vertexStreams;
		HashSet<const shader::StubSamplerState*> m_staticSamplersRefs;
		HashMap<uint64_t, const shader::StubGlobalConstant*> m_globalConstants;
		HashMap<const DataParameter*, const shader::Stub*> m_parametersMap; // auto globals
		const shader::StubFunction* m_entryFunction = nullptr;

		HashMap<const Function*, shader::StubFunction*> m_tempFunctionMap;
		Array<const Function*> m_functionExportStack;
		HashMap<StringID, uint32_t> m_functionNameCounter;

		bool usesControlFlowHints = false;
		bool usesDiscard = false;
		bool usesDepthOutput = false;
		bool usesUAVs = false;
	};

	static const auto NUM_STAGES = (uint32_t)ShaderStage::MAX;
	StageContext m_stages[NUM_STAGES];

	StageContext* m_activeStage = nullptr;
	GlobalContext m_global;

	//--

	shader::StubLocation exportLocation(const parser::Location& location)
	{
		shader::StubLocation ret;
		ret.line = location.line();

		if (!location.contextName().empty() && !m_global.m_fileMap.find(location.contextName(), ret.file))
		{
			auto* file = m_builder.createStub<shader::StubFile>();
			file->depotPath = location.contextName();

			m_global.m_fileMap[location.contextName()] = file;
			ret.file = file;
		}

		return ret;
	}

	StubPseudoArray<shader::StubAttribute> exportAttributes(const AttributeList& sourceAttributes)
	{
		InplaceArray<const shader::StubAttribute*, 10> attributes;

		for (const auto& sourceAttr : sourceAttributes.attributes.pairs())
		{
			auto* attr = m_builder.createStub<shader::StubAttribute>();
			attr->name = sourceAttr.key;
			attr->value = sourceAttr.value;
			attributes.pushBack(attr);
		}

		return m_builder.createArray(attributes);
	}

	static shader::ScalarType MapBaseType(BaseType type)
	{
		switch (type)
		{
			case BaseType::Void: return shader::ScalarType::Void;
			case BaseType::Boolean: return shader::ScalarType::Boolean;
			case BaseType::Float: return shader::ScalarType::Float;
			case BaseType::Int: return shader::ScalarType::Int;
			case BaseType::Uint: return shader::ScalarType::Uint;
		}

		ASSERT(!"Unknown type");
		return shader::ScalarType::Void;
	}

	shader::StubStructMember* exportStructMember(const CompositeType::Member& member)
	{
		auto* ret = m_builder.createStub<shader::StubStructMember>();
		ret->name = member.name;
		ret->attributes = exportAttributes(member.attributes);
		ret->location = exportLocation(member.location);
		ret->linearAlignment = member.layout.linearAlignment;
		ret->linearOffset = member.layout.linearOffset;
		ret->linearSize = member.layout.linearSize;
		ret->linearArrayCount = member.layout.linearArrayCount;
		ret->linearArrayStride = member.layout.linearArrayStride;
		ret->type = exportDataType(member.type);
		return ret;
	}

	const shader::StubStruct* exportCompositeStruct(const CompositeType* composite)
	{
		ASSERT(composite);
		ASSERT(composite->hint() == CompositeTypeHint::User);

		shader::StubStruct* ret = nullptr;
		if (m_global.m_compositeMap.find(composite, ret))
		{
			m_activeStage->m_composites.insert(ret);
			return ret;
		}

		ret = m_builder.createStub<shader::StubStruct>();
		ret->name = composite->name();
		//ret->attributes = std::move(exportAttributes(composite->atti))
		//ret->location = exportLocation(composite->)
		ret->size = composite->linearSize();
		ret->alignment = composite->linearAlignment();
				
		InplaceArray<const shader::StubStructMember*, 20> members;
		for (const auto& sourceMember : composite->members())
		{
			if (auto* member = exportStructMember(sourceMember))
			{
				member->owner = ret;
				members.pushBack(member);
			}
		}
		ret->members = std::move(m_builder.createArray(members));

		m_global.m_compositeMap[composite] = ret;
		m_activeStage->m_composites.insert(ret);
		return ret;
	}

	shader::StubTypeDecl* exportCompositeType(const CompositeType* composite)
	{
		ASSERT(composite);

		const DataType compositeType(composite);

		if (composite->hint() == CompositeTypeHint::VectorType)
		{
			auto* stub = m_builder.createStub<shader::StubVectorTypeDecl>();
			stub->type = MapBaseType(ExtractBaseType(compositeType));
			stub->componentCount = ExtractComponentCount(compositeType);
			return stub;
		}
		else if (composite->hint() == CompositeTypeHint::MatrixType)
		{
			auto* stub = m_builder.createStub<shader::StubMatrixTypeDecl>();
			stub->type = MapBaseType(ExtractBaseType(compositeType));
			stub->componentCount = ExtractComponentCount(compositeType);
			stub->rowCount = ExtractRowCount(compositeType);
			return stub;
		}
		else
		{
			auto* stub = m_builder.createStub<shader::StubStructTypeDecl>();
			stub->structType = exportCompositeStruct(composite);
			return stub;
		}
	}

	const shader::StubTypeDecl* exportDataType(const DataType& type)
	{
		shader::StubTypeDecl* ret = nullptr;
		if (m_global.m_typeMap.find(type, ret))
		{
			m_activeStage->m_types.insert(ret);
			return ret;
		}

		if (type.isArray())
		{
			auto* stub = m_builder.createStub<shader::StubArrayTypeDecl>();
			stub->count = type.arrayCounts().outermostCount();

			const auto innerArrayCount = type.arrayCounts().innerCounts();
			const auto innerArrayType = type.applyArrayCounts(innerArrayCount);
			stub->innerType = exportDataType(innerArrayType);

			ret = stub;
		}
		else if (type.isComposite())
		{
			ret = exportCompositeType(&type.composite());
		}
		else if (type.isResource())
		{
			return nullptr;
		}
		else
		{
			auto* stub = m_builder.createStub<shader::StubScalarTypeDecl>();
			stub->type = MapBaseType(type.baseType());
			ret = stub;
		}

		m_global.m_typeMap[type] = ret;
		m_activeStage->m_types.insert(ret);
		return ret;
	}

	const shader::StubSamplerState* exportStaticSampler(const StaticSampler* state)
	{
		shader::StubSamplerState* ret = nullptr;
		if (m_global.m_staticSamplers.find(state, ret))
			return ret;

		ret = m_builder.createStub<shader::StubSamplerState>();
		ret->index = m_global.m_staticSamplers.size();
		ret->name = state->name();
		ret->state = state->state();
		ret->state.label = StringBuf(state->name());
		// TODO: attributes ?

		m_global.m_staticSamplers[state] = ret;
		return ret;
	}

	const shader::StubFunctionParameter* exportFunctionParameter(const DataParameter* param)
	{
		auto* ret = m_builder.createStub<shader::StubFunctionParameter>();
		ret->name = param->name;
		ret->reference = param->attributes.has("out"_id);
		ret->type = exportDataType(param->dataType);

		m_activeStage->m_parametersMap[param] = ret;
		return ret;
	}

	const shader::StubFunction* exportFunction(const Function* function)
	{
		shader::StubFunction* ret = nullptr;
		if (m_activeStage->m_functionMap.find(function, ret))
			return ret;

		if (m_activeStage->m_tempFunctionMap.find(function, ret))
		{
			const auto* top = m_activeStage->m_functionExportStack.back();
			if (top != function)
				m_activeStage->m_functionForwardRefs.insert(ret); // function must be forward declared
			return ret;
		}				

		ret = m_builder.createStub<shader::StubFunction>();
		ret->location = exportLocation(function->location());
		ret->attributes = std::move(exportAttributes(function->attributes()));
		ret->returnType = exportDataType(function->returnType());

		const auto nameCounter = m_activeStage->m_functionNameCounter[function->name()] + 1;
		if (nameCounter == 1)
			ret->name = function->name();
		else 
			ret->name = StringID(TempString("{}_{}", function->name(), nameCounter));
		m_activeStage->m_functionNameCounter[function->name()] = nameCounter;

		InplaceArray<const shader::StubFunctionParameter*, 10> parameters;
		for (const auto* sourceParam : function->inputParameters())
			if (!function->staticParameters().contains(sourceParam->name))
				parameters.pushBack(exportFunctionParameter(sourceParam));
		ret->parameters = m_builder.createArray(parameters);

		{
			m_activeStage->m_tempFunctionMap[function] = ret;
			m_activeStage->m_functionExportStack.pushBack(function);

			ret->code = exportOpcode(&function->code());

			m_activeStage->m_functionExportStack.popBack();
			m_activeStage->m_tempFunctionMap.remove(function);
		}

		m_activeStage->m_functionMap[function] = ret;
		return ret;
	}			

	StubPseudoArray<shader::StubOpcode> exportChildOpcodes(const CodeNode* node)
	{
		InplaceArray<const shader::StubOpcode*, 64> opcodes;
		for (const auto* child : node->children())
			if (const auto* op = exportOpcode(child))
				opcodes.pushBack(op);

		return m_builder.createArray(opcodes);
	}

	static shader::ComponentSwizzle exportMask(ComponentMask mask)
	{
		shader::ComponentSwizzle ret;

		for (uint32_t i = 0; i < 4; ++i)
		{
			switch (mask.componentSwizzle(i))
			{
				case ComponentMaskBit::X: ret.mask[i] = 'x'; break;
				case ComponentMaskBit::Y: ret.mask[i] = 'y'; break;
				case ComponentMaskBit::Z: ret.mask[i] = 'z'; break;
				case ComponentMaskBit::W: ret.mask[i] = 'w'; break;

				case ComponentMaskBit::One:
				case ComponentMaskBit::Zero:
					ASSERT(!"Unsupported mask bit!");
					break;
			}
		}

		return ret;
	}
			
	const shader::StubOpcode* conditionalCreateSwizzledNode(uint32_t maxInputComponents, const shader::StubOpcode* input, const Array<ComponentMaskBit>& mask, BaseType scalarType)
	{
		static const char ComponentNames[4] = { 'x','y','z','w' };
		static const ComponentMaskBit ComponentBits[4] = { ComponentMaskBit::X, ComponentMaskBit::Y, ComponentMaskBit::Z, ComponentMaskBit::W };				

		if (mask.empty())
			return nullptr;

		// check for identity mask - if we have it don't export it
		bool hasIdentityMask = true;
		for (uint32_t i = 0; i < mask.size(); i++)
		{
			if (mask[i] != ComponentBits[i])
			{
				hasIdentityMask = false;
				break;
			}
		}

		// identity masks (ie. .x, .xy, .xyz, .xyzw) can be simplified to either nothing or simple cast
		if (hasIdentityMask)
		{
			// all components are consumed, no swizzle, no cast
			if (maxInputComponents == mask.size())
				return input;

			// use the vector constructor to contract
			if (mask.size() > 1)
			{
				InplaceArray<const shader::StubOpcode*, 1 > parts;
				parts.pushBack(input);

				// determine the base type
				auto* op = m_builder.createStub<shader::StubOpcodeCreateVector>();
				op->typeDecl = exportDataType(m_lib.typeLibrary().simpleCompositeType(scalarType, mask.size()))->asVectorTypeDecl();
				ASSERT(op->typeDecl);
				op->elements = m_builder.createArray(parts);
				return op;
			}
		}

		// create mask from components
		auto* op = m_builder.createStub<shader::StubOpcodeSwizzle>();
		op->inputComponents = maxInputComponents;
		op->outputComponents = mask.size();
		op->value = input;

		for (uint32_t i = 0; i < mask.size(); i++)
		{
			switch (mask[i])
			{
				case ComponentMaskBit::X: op->swizzle.mask[i] = 'x'; break;
				case ComponentMaskBit::Y: op->swizzle.mask[i] = 'y'; break;
				case ComponentMaskBit::Z: op->swizzle.mask[i] = 'z'; break;
				case ComponentMaskBit::W: op->swizzle.mask[i] = 'w'; break;

				case ComponentMaskBit::NotDefined:
				case ComponentMaskBit::One:
				case ComponentMaskBit::Zero:
					ASSERT(!"Unsupported mask bit");
					break;
			}
		}

		return op;
	}

	const shader::StubOpcode* exportReadSwizzle(const CodeNode* node, ComponentMask mask)
	{
		const auto* value = exportOpcode(node->children()[0]);

		const auto resultBaseType = ExtractBaseType(node->dataType());
		const auto& inputDataType = node->children()[0]->dataType();
		const auto inputComponents = inputDataType.computeScalarComponentCount();
		const auto inputBaseType = ExtractBaseType(inputDataType);

		logging::Log::Print(logging::OutputLevel::Warning, node->location().contextName().c_str(), node->location().line(), "", "",
			TempString("Swizzle {} of {} to {}. {} -> {} components", mask, inputDataType, node->dataType(), inputComponents, mask.numberOfComponentsProduced()));

		// gather swizzle parts
		InplaceArray<const shader::StubOpcode*, 4> parts;
		InplaceArray<ComponentMaskBit, 4> swizzleSoFar;
		{
			for (uint32_t i = 0; i < mask.numComponents(); ++i)
			{
				auto field = mask.componentSwizzle(i);
				ASSERT(field != ComponentMaskBit::NotDefined);

				if (field == ComponentMaskBit::Zero || field == ComponentMaskBit::One)
				{
					if (const auto* part = conditionalCreateSwizzledNode(inputComponents, value, swizzleSoFar, inputBaseType))
						parts.pushBack(part);

					swizzleSoFar.reset();

					auto* part = m_builder.createStub<shader::StubOpcodeConstant>();
					part->typeDecl = exportDataType(DataType(resultBaseType)); // scalar type
					part->dataType = MapBaseType(resultBaseType);
					part->dataSize = 4;
					part->data = m_builder.createData(part->dataSize);
					parts.pushBack(part);

					if (field == ComponentMaskBit::Zero)
					{
						*(uint32_t*)part->data = 0;
					}
					else if (field == ComponentMaskBit::One && resultBaseType == BaseType::Float)
					{
						*(float*)part->data = 1.0f;
					}
					else if (field == ComponentMaskBit::One)
					{
						*(uint32_t*)part->data = 1;
					}
				}
				else
				{
					swizzleSoFar.pushBack(field);
				}
			}

			// flush - we might have no zero/one swizzle
			if (auto* part = conditionalCreateSwizzledNode(inputComponents, value, swizzleSoFar, inputBaseType))
				parts.pushBack(part);
		}

		// single part ?
		if (parts.size() == 1)
			return parts[0];

		// create via composition
		auto* op = m_builder.createStub<shader::StubOpcodeCreateVector>();
		op->typeDecl = exportDataType(node->dataType())->asVectorTypeDecl(); // vec3, etc
		op->elements = m_builder.createArray(parts);
		return op;
	}

	shader::StubDescriptorMember* exportDescriptorMember(const ResourceTableEntry& sourceMember)
	{
		const auto& resourceType = sourceMember.m_type.resource();

		switch (resourceType.type)
		{
			case DeviceObjectViewType::ConstantBuffer:
			{
				auto* op = m_builder.createStub<shader::StubDescriptorMemberConstantBuffer>();
				op->size = resourceType.resolvedLayout->linearSize();

				InplaceArray<const shader::StubDescriptorMemberConstantBufferElement*, 20> elements;
				for (const auto& member : resourceType.resolvedLayout->members())
				{
					auto* element = m_builder.createStub<shader::StubDescriptorMemberConstantBufferElement>();
					element->constantBuffer = op;
					element->name = member.name;
					element->location = exportLocation(member.location);
					element->type = exportDataType(member.type);
					element->attributes = exportAttributes(member.attributes);
					element->linearAlignment = member.layout.linearAlignment;
					element->linearOffset = member.layout.linearOffset;
					element->linearSize = member.layout.linearSize;
					element->linearArrayCount = member.layout.linearArrayCount;
					element->linearArrayStride = member.layout.linearArrayStride;
					elements.pushBack(element);
				}

				op->elements = m_builder.createArray(elements);
				return op;
			}

			case DeviceObjectViewType::BufferStructured:
			case DeviceObjectViewType::BufferStructuredWritable:
			{
				auto* op = m_builder.createStub<shader::StubDescriptorMemberStructuredBuffer>();
				op->writable = (resourceType.type == DeviceObjectViewType::BufferStructuredWritable);
				op->layout = exportCompositeStruct(resourceType.resolvedLayout);
				op->stride = op->layout->size; // TODO: add custom "stride" attribute
				return op;
			}

			case DeviceObjectViewType::Buffer:
			case DeviceObjectViewType::BufferWritable:
			{
				auto* op = m_builder.createStub<shader::StubDescriptorMemberFormatBuffer>();
				op->writable = (resourceType.type == DeviceObjectViewType::BufferWritable);
				op->format = resourceType.resolvedFormat;
				return op;
			}

			case DeviceObjectViewType::Sampler:
			{
				auto* op = m_builder.createStub<shader::StubDescriptorMemberSampler>();
				return op;
			}

			case DeviceObjectViewType::SampledImage:
			{
				ASSERT(resourceType.attributes.has("sampler"_id));

				auto* op = m_builder.createStub<shader::StubDescriptorMemberSampledImage>();
				op->scalarType = MapBaseType(resourceType.sampledImageFlavor);
				op->viewType = resourceType.resolvedViewType;
				op->multisampled = resourceType.multisampled;
				op->depth = resourceType.depth;

				if (sourceMember.m_staticSampler != nullptr)
					op->staticState = exportStaticSampler(sourceMember.m_staticSampler);

				return op;
			}

			case DeviceObjectViewType::Image:
			case DeviceObjectViewType::ImageWritable:
			{
				ASSERT(resourceType.attributes.has("nosampler"_id) || resourceType.attributes.has("uav"_id));
				ASSERT(!resourceType.attributes.has("sampler"_id));

				auto* op = m_builder.createStub<shader::StubDescriptorMemberImage>();
				op->viewType = resourceType.resolvedViewType;
				op->format = resourceType.resolvedFormat;
				op->writable = (resourceType.type == DeviceObjectViewType::ImageWritable);
				return op;
			}
		}

		ASSERT(!"Unknown buffer type");
		return nullptr;
	}

	const shader::StubDescriptor* exportDescriptor(const ResourceTable* table)
	{
		ASSERT(table != nullptr);

		shader::StubDescriptor* ret = nullptr;
		if (m_global.m_descriptorMap.find(table, ret))
		{
			m_activeStage->m_descriptors.insert(ret);
			return ret;
		}

		InplaceArray<shader::StubDescriptorMember*, 20> members;
		members.reserve(table->members().size());

		ret = m_builder.createStub<shader::StubDescriptor>();

		for (uint32_t i : table->members().indexRange())
		{
			const auto& sourceMember = table->members()[i];

			auto* member = exportDescriptorMember(sourceMember);
			member->name = sourceMember.m_name;
			member->location = exportLocation(sourceMember.m_location);
			member->attributes = exportAttributes(sourceMember.m_attributes);
			member->index = i;
			member->descriptor = ret;
			members.pushBack(member);
		}

		// LINK local sampler
		for (uint32_t i : table->members().indexRange())
		{
			const auto& sourceMember = table->members()[i];
			if (sourceMember.m_localSampler != -1)
			{
				auto* sourceSampler = members[sourceMember.m_localSampler]->asDescriptorMemberSampler();
				ASSERT(sourceSampler);

				if (auto* target = members[i]->asDescriptorMemberSampledImage())
					target->dynamicSamplerDescriptorEntry = sourceSampler;

				else if (auto* target = members[i]->asDescriptorMemberSampledImageTable())
					target->dynamicSamplerDescriptorEntry = sourceSampler;
			}
		}
				
		ret->name = table->name();
		ret->attributes = exportAttributes(table->attributes());
		ret->members = m_builder.createArray(members);

		m_global.m_descriptorMap[table] = ret;
		m_activeStage->m_descriptors.insert(ret);
		return ret;
	}

	const shader::Stub* exportGlobalParamReference(const DataParameter* param)
	{
		const auto* desc = exportDescriptor(param->resourceTable);
		DEBUG_CHECK_RETURN_V(desc, nullptr);

		ASSERT(param->resourceTableEntry);
		for (const auto* member : desc->members)
		{
			if (member->name == param->resourceTableEntry->m_name)
			{
				m_activeStage->m_descriptorMembers.insert(member); // keep track of actually used descriptor members

				if (auto* image = member->asDescriptorMemberSampledImageTable())
				{
					if (image->dynamicSamplerDescriptorEntry)
						m_activeStage->m_descriptorMembers.insert(image->dynamicSamplerDescriptorEntry);
					else if (image->staticState)
						m_activeStage->m_staticSamplersRefs.insert(image->staticState);
				}
				else if (auto* image = member->asDescriptorMemberSampledImage())
				{
					if (image->dynamicSamplerDescriptorEntry)
						m_activeStage->m_descriptorMembers.insert(image->dynamicSamplerDescriptorEntry);
					else if (image->staticState)
						m_activeStage->m_staticSamplersRefs.insert(image->staticState);
				}

				else if (auto* image = member->asDescriptorMemberImage())
				{
					if (image->writable)
						m_activeStage->usesUAVs = true;
				}
				else if (auto* buffer = member->asDescriptorMemberFormatBuffer())
				{
					if (buffer->writable)
						m_activeStage->usesUAVs = true;
				}
				else if (auto* buffer = member->asDescriptorMemberStructuredBuffer())
				{
					if (buffer->writable)
						m_activeStage->usesUAVs = true;
				}

				if (const auto* cb = member->asDescriptorMemberConstantBuffer())
				{
					ASSERT(param->resourceTableCompositeEntry);
					for (const auto* element : cb->elements)
						if (element->name == param->resourceTableCompositeEntry->name)
							return element;

					ASSERT(!"Unknown constant buffer element");
					return nullptr;
				}
				else
				{
					ASSERT(!param->resourceTableCompositeEntry);
					return member;
				}
			}
		}

		ASSERT(!"Unknown member");
		return nullptr;
	}

	const shader::StubDescriptorMember* exportResourceRef(StringView name)
	{
		ASSERT(name.beginsWith("res:"));

		auto descriptorName = name.afterFirst("res:").beforeFirst(".");
		ASSERT(descriptorName);

		auto resourceName = name.afterFirst(".");
		ASSERT(resourceName);

		const auto* desc = m_lib.typeLibrary().findResourceTable(StringID::Find(descriptorName));
		ASSERT(desc);

		const auto* exportedDesc = exportDescriptor(desc);
		ASSERT(exportedDesc);

		for (auto* member : exportedDesc->members)
		{
			if (member->name == resourceName)
			{
				m_activeStage->m_descriptorMembers.insert(member); // remember that we used this entry

				if (auto* image = member->asDescriptorMemberSampledImageTable())
				{
					if (image->dynamicSamplerDescriptorEntry)
						m_activeStage->m_descriptorMembers.insert(image->dynamicSamplerDescriptorEntry);
					else if (image->staticState)
						m_activeStage->m_staticSamplersRefs.insert(image->staticState);
				}
				else if (auto* image = member->asDescriptorMemberSampledImage())
				{
					if (image->dynamicSamplerDescriptorEntry)
						m_activeStage->m_descriptorMembers.insert(image->dynamicSamplerDescriptorEntry);
					else if (image->staticState)
						m_activeStage->m_staticSamplersRefs.insert(image->staticState);
				}

				return member;
			}
		}

		ASSERT(!"Unknown resource in descriptor");
		return nullptr;

	}

	const shader::StubVertexInputStream* exportVertexStream(const DataParameter* param)
	{
		ASSERT(param && param->scope == DataParameterScope::VertexInput);

		shader::StubVertexInputStream* ret = nullptr;
		if (m_global.m_vertexStreams.find(param, ret))
		{
			m_activeStage->m_vertexStreams.insert(ret);
			return ret;
		}

		const auto& composite = param->dataType.composite();
		ASSERT(composite.packingRules() == CompositePackingRules::Vertex);

		ret = m_builder.createStub<shader::StubVertexInputStream>();
		ret->name = param->attributes.valueOrDefault("binding"_id, param->dataType.composite().name().view());
		ret->instanced = param->attributes.has("instanced"_id);
		ret->streamIndex = m_global.m_vertexStreams.size(); // TODO
		ret->streamSize = composite.linearSize();
		ret->streamStride = composite.linearSize(); // TODO
		TRACE_SPAM("Exporting vertex stream '{}', index {}, size {}, stride {}", ret->name, ret->streamIndex, ret->streamStride, ret->streamSize);

		InplaceArray<shader::StubVertexInputElement*, 20> elements;
		elements.reserve(composite.members().size());

		for (const auto& member : composite.members())
		{
			auto* op = m_builder.createStub<shader::StubVertexInputElement>();
			op->stream = ret;
			op->name = member.name;
			op->type = exportDataType(member.type);
			op->elementFormat = member.layout.dataFormat;
			op->elementOffset = member.layout.linearOffset;
			op->elementSize = member.layout.linearSize;
			op->elementIndex = elements.size();

			TRACE_SPAM("Exporting vertex stream element '{}', index {}, format {}, offset {}, size {}",
				op->name, op->elementIndex, op->elementFormat, op->elementOffset, op->elementSize);

			elements.pushBack(op);
		}

		ret->elements = m_builder.createArray(elements);

		m_global.m_vertexStreams[param] = ret;
		m_activeStage->m_vertexStreams.insert(ret);
		return ret;
	}

	static uint32_t DetermineGeometryvertexInputCount(const Function* entryFunction)
	{
		auto input = entryFunction->attributes().valueOrDefault("input"_id, "");
		if (input == "points")
			return 1;
		else if (input == "lines")
			return 2;
		else if (input == "lines_adjacency")
			return 4;
		else if (input == "triangles")
			return 3;
		else if (input == "triangles_adjacency")
			return 6;

		ASSERT(!"Invalid geometry shader parameters");
		return 0;
	}

	const shader::Stub* exportDataParamReference(const DataParameter* param)
	{
		const shader::Stub* ret = nullptr;
		if (m_activeStage->m_parametersMap.find(param, ret))
			return ret;

		switch (param->scope)
		{
			case DataParameterScope::StaticConstant:
			case DataParameterScope::GlobalConst:
			case DataParameterScope::ScopeLocal:
			case DataParameterScope::FunctionInput:
				ASSERT_EX(false, TempString("Parameter '{}', scope {} should be resovled already", param->name, param->scope));
				break;

			case DataParameterScope::GlobalParameter:
				ret = exportGlobalParamReference(param);
				break;

			case DataParameterScope::GlobalBuiltin:
			{
				auto* op = m_builder.createStub<shader::StubBuiltInVariable>();
				op->builinType = param->builtInVariable;
				op->dataType = exportDataType(param->dataType);
				m_activeStage->m_buildInVariables.insert(op);

				if (op->builinType == shader::ShaderBuiltIn::Depth)
					m_activeStage->usesDepthOutput = true;

				ret = op;
				break;
			}

			case DataParameterScope::VertexInput:
				ret = exportVertexStream(param);
				break;

			case DataParameterScope::StageInput:
			{
				auto* op = m_builder.createStub<shader::StubStageInput>();
				op->name = param->name;
				op->bindingName = StringID(param->attributes.valueOrDefault("binding"_id, param->name.view()));

				if (m_activeStage->stage == ShaderStage::Geometry)
				{
					ASSERT(param->dataType.isArray());

					// inject the count for array input parameters that is based on the primitive type
					const auto vertexCount = DetermineGeometryvertexInputCount(m_activeStage->sourceEntryFunction);
					const auto dataType = param->dataType.removeArrayCounts().applyArrayCounts(vertexCount);
					op->type = exportDataType(dataType);
				}
				else
				{
					op->type = exportDataType(param->dataType);
				}

				op->attributes = exportAttributes(param->attributes);
				m_activeStage->m_stageInputs.insert(op);
				ret = op;
				break;
			}
					
			case DataParameterScope::StageOutput:
			{
				auto* op = m_builder.createStub<shader::StubStageOutput>();
				op->name = param->name;
				op->bindingName = StringID(param->attributes.valueOrDefault("binding"_id, param->name.view()));
				op->type = exportDataType(param->dataType);
				op->attributes = exportAttributes(param->attributes);
				m_activeStage->m_stageOutputs.insert(op);
				ret = op;
				break;
			}

			case DataParameterScope::GroupShared:
			{
				auto* op = m_builder.createStub<shader::StubSharedMemory>();
				op->name = param->name;
				op->type = exportDataType(param->dataType);
				op->attributes = exportAttributes(param->attributes);
				m_activeStage->m_sharedMemory.insert(op);
				ret = op;
				break;
			}
		}

		m_activeStage->m_parametersMap[param] = ret;
		return ret;
	}

	static void HashTypedData(CRC64& ptr, const DataType& type, const DataValue& value)
	{
		const auto totalComponents = type.computeScalarComponentCount();
		for (uint32_t i = 0; i < totalComponents; ++i)
			ptr << value.component(i).valueNumerical();
	}

	static void WriteTypedData(void* ptr, const DataType& type, const DataValue& value)
	{
		const auto baseType = ExtractBaseType(type);
		const auto totalComponents = type.computeScalarComponentCount();

		auto* writePtr = (float*)ptr;
		for (uint32_t i = 0; i < totalComponents; ++i, ++writePtr)
		{
			switch (baseType)
			{
			case BaseType::Float:
				*writePtr = value.component(i).valueFloat();
				break;

			case BaseType::Uint:
				*(uint32_t*)writePtr = value.component(i).valueUint();
				break;

			case BaseType::Int:
				*(int*)writePtr = value.component(i).valueInt();
				break;

			case BaseType::Boolean:
				*(uint32_t*)writePtr = value.component(i).valueBool();
				break;

			default:
				ASSERT(!"Invalid base type");
			}
		}
	}

	static uint32_t CalcDataSize(const DataType& type)
	{
		const auto baseType = ExtractBaseType(type);
		const auto totalComponents = type.computeScalarComponentCount();

		switch (baseType)
		{
		case BaseType::Float:
		case BaseType::Uint:
		case BaseType::Int:
		case BaseType::Boolean:
			return totalComponents * sizeof(float);

		default:
			ASSERT(!"Invalid base type");
			return 0;
		}
	}

	const shader::StubGlobalConstant* exportGlobalConstant(const DataType& type, const DataValue& value)
	{
		CRC64 crc;
		HashTypedData(crc, type, value);

		const shader::StubGlobalConstant* ret = nullptr;
		if (m_activeStage->m_globalConstants.find(crc.crc(), ret))
			return ret;

		auto* op = m_builder.createStub<shader::StubGlobalConstant>();
		op->index = m_activeStage->m_globalConstants.size();
		op->typeDecl = exportDataType(type);
		op->dataType = MapBaseType(ExtractBaseType(type));
		op->dataSize = CalcDataSize(type);

		void* data = m_builder.createData(op->dataSize);
		WriteTypedData(data, type, value);
		op->data = data;
				
		m_activeStage->m_globalConstants[crc.crc()] = op;
		return op;
	}

	const shader::StubOpcodeConstant* exportLocalConstant(const DataType& type, const DataValue& value)
	{
		auto* op = m_builder.createStub<shader::StubOpcodeConstant>();
		op->typeDecl = exportDataType(type);
		op->dataType = MapBaseType(ExtractBaseType(type));
		op->dataSize = CalcDataSize(type);

		void* data = m_builder.createData(op->dataSize);
		WriteTypedData(data, type, value);
		op->data = data;

		return op;
	}

	static bool ResourceSupportsElementRefToStoreConversion(const shader::StubOpcodeResourceLoad* op)
	{
		const auto resType = op->resourceRef->type;
		switch (resType)
		{
			case DeviceObjectViewType::BufferWritable:
			case DeviceObjectViewType::ImageWritable:
				return true;
		}

		return false;
	}

	static bool ResourceSupportsResourceLoad(const shader::StubOpcodeResourceRef* op)
	{
		switch (op->type)
		{
			case DeviceObjectViewType::Buffer:
			case DeviceObjectViewType::Image:
			case DeviceObjectViewType::BufferWritable:
			case DeviceObjectViewType::ImageWritable:
			case DeviceObjectViewType::SampledImage:
				return true;
		}

		return false;
	}

	static bool ResourceSupportsResourceElement(const shader::StubOpcodeResourceRef* op)
	{
		switch (op->type)
		{
		case DeviceObjectViewType::BufferStructured:
		case DeviceObjectViewType::BufferStructuredWritable:
			return true;
		}

		return false;
	}

	const shader::StubOpcode* exportOpcode(const CodeNode* node)
	{
		if (!node)
			return nullptr;

		switch (node->opCode())
		{
			case OpCode::Scope:
			{
				auto* op = m_builder.createStub<shader::StubOpcodeScope>();

				InplaceArray<const shader::StubScopeLocalVariable*, 20> localVars;
				localVars.reserve(node->declarations().size());

				for (const auto* param : node->declarations())
				{
					DEBUG_CHECK(param->scope == DataParameterScope::ScopeLocal);
					if (param->scope == DataParameterScope::ScopeLocal)
					{
						auto* stub = m_builder.createStub<shader::StubScopeLocalVariable>();
						stub->location = exportLocation(param->loc);
						stub->name = param->name;
						stub->type = exportDataType(param->dataType);
						localVars.pushBack(stub);
						m_activeStage->m_parametersMap[param] = stub;
						//TRACE_INFO("Resolved local param '{}' {}", param->name, param);
					}
				}

				op->locals = m_builder.createArray(localVars);
				op->statements = std::move(exportChildOpcodes(node));
				return op;
			}

			case OpCode::Store:
			{
				auto* lValue = exportOpcode(node->children()[0]);
				if (const auto* resElement = lValue->asOpcodeResourceLoad())
				{
					if (ResourceSupportsElementRefToStoreConversion(resElement))
					{
						auto* op = m_builder.createStub<shader::StubOpcodeResourceStore>();
						op->resourceRef = resElement->resourceRef;
						op->address = resElement->address;
						op->numAddressComponents = resElement->numAddressComponents;
						op->numValueComponents = resElement->numValueComponents;
						op->value = exportOpcode(node->children()[1]);
						return op;
					}
				}

				// keep as generic store
				auto* op = m_builder.createStub<shader::StubOpcodeStore>();
				op->mask = exportMask(node->extraData().m_mask);
				op->lvalue = lValue;
				op->rvalue = exportOpcode(node->children()[1]);
				op->type = exportDataType(node->children()[1]->dataType());
				return op;
			}

			case OpCode::VariableDecl:
			{
				ASSERT(node->extraData().m_paramRef);
				ASSERT(node->extraData().m_paramRef->scope == DataParameterScope::ScopeLocal);

				const shader::Stub* stub = nullptr;
				m_activeStage->m_parametersMap.find(node->extraData().m_paramRef, stub);
				ASSERT(stub != nullptr);

				auto* localVar = const_cast<shader::StubScopeLocalVariable*>(stub->asScopeLocalVariable());
				ASSERT(localVar != nullptr);
				ASSERT(!localVar->initialized);

				// export as variable declaration
				auto* op = m_builder.createStub<shader::StubOpcodeVariableDeclaration>();
				op->var = localVar;

				// export initialization code
				if (node->children().size() == 1)
				{
					localVar->initialized = true;
					op->init = exportOpcode(node->children()[0]);
				}

				return op;
			}

			case OpCode::Load: // loads value from dereferenced reference:requires a reference
			{
				auto* op = m_builder.createStub<shader::StubOpcodeLoad>();
				op->valueReferece = exportOpcode(node->children()[0]);
				return op;
			}

			case OpCode::Const:// literal value:stored as float/int union
			{
				const auto baseType = ExtractBaseType(node->dataType());
				if (baseType == BaseType::Resource)
				{
					ASSERT(node->dataValue().isWholeValueDefined() && node->dataValue().isScalar() && node->dataValue().component(0).isName());

					const auto resourceName = node->dataValue().component(0).name();
					TRACE_INFO("Resource name: '{}'", resourceName);

					auto* resourceRef = exportResourceRef(resourceName.view());
					ASSERT(resourceRef);

					auto* op = m_builder.createStub<shader::StubOpcodeResourceRef>();
					op->type = node->dataType().resource().type;
					ASSERT(op->type != DeviceObjectViewType::Invalid);
					op->descriptorEntry = resourceRef;
					return op;
				}
				else 
				{
					const auto totalComponents = node->dataType().computeScalarComponentCount();
					if (totalComponents > 4)
					{
						auto* op = m_builder.createStub<shader::StubOpcodeDataRef>();
						op->stub = exportGlobalConstant(node->dataType(), node->dataValue());
						return op;
					}
					else
					{
						return exportLocalConstant(node->dataType(), node->dataValue());
					}
				}
			}

			case OpCode::Call:// call to a shader defined function
			{
				const auto* func = node->extraData().m_finalFunctionRef;
				ASSERT(func);

				ASSERT(node->children().size() >= 1);
				ASSERT(node->children()[0]->opCode() == OpCode::FuncRef);

				auto op = m_builder.createStub<shader::StubOpcodeCall>();
				op->func = exportFunction(func);

				InplaceArray<const shader::StubOpcode*, 10> finalArguments;
				InplaceArray<const shader::StubTypeDecl*, 10> finalArgumentTypes;
				finalArguments.reserve(node->children().size());
				finalArgumentTypes.reserve(node->children().size());

				ASSERT((node->children().size() - 1) == func->inputParameters().size()); // they should still match
				for (auto i : func->inputParameters().indexRange())
				{
					const auto* arg = func->inputParameters()[i];
					const auto* childNode = node->children()[i+1];

					if (!func->staticParameters().contains(arg->name))
					{
						finalArguments.pushBack(exportOpcode(childNode));
						finalArgumentTypes.pushBack(exportDataType(childNode->dataType()));
					}
					else
					{
						ASSERT(childNode->dataValue().isWholeValueDefined());
					}
				}

				op->arguments = m_builder.createArray(finalArguments);
				op->argumentTypes = m_builder.createArray(finalArgumentTypes);
				return op;
			}

			case OpCode::NativeCall:// call to a native function
			{
				auto op = m_builder.createStub<shader::StubOpcodeNativeCall>();
				op->name = StringID(node->extraData().m_nativeFunctionRef->cls()->findMetadataRef<FunctionNameMetadata>().name());
				op->returnType = exportDataType(node->dataType());

				InplaceArray<const shader::StubOpcode*, 10> finalArguments;
				InplaceArray<const shader::StubTypeDecl*, 10> finalArgumentTypes;
				finalArguments.reserve(node->children().size());
				finalArgumentTypes.reserve(node->children().size());

				for (const auto* childNode : node->children())
				{
					finalArguments.pushBack(exportOpcode(childNode));
					finalArgumentTypes.pushBack(exportDataType(childNode->dataType()));
				}

				op->arguments = m_builder.createArray(finalArguments);
				op->argumentTypes = m_builder.createArray(finalArgumentTypes);
				return op;
			}

			case OpCode::ParamRef:// reference to a data parameter
			{
				auto op = m_builder.createStub<shader::StubOpcodeDataRef>();
				op->stub = exportDataParamReference(node->extraData().m_paramRef);
				return op;
			}
					 
			case OpCode::AccessMember:
			{
				const auto* structNode = node->children()[0];
				const auto memberName = node->extraData().m_name;
				if (structNode->opCode() == compiler::OpCode::ParamRef)
				{
					const auto* structParam = structNode->extraData().m_paramRef;
					DEBUG_CHECK(nullptr != structParam);

					if (structParam->scope == compiler::DataParameterScope::VertexInput)
					{
						if (const auto* vertexStream = exportVertexStream(structParam))
						{
							for (const auto* element : vertexStream->elements)
							{
								if (element->name == memberName)
								{
									auto op = m_builder.createStub<shader::StubOpcodeDataRef>();
									op->stub = element;
									return op;
								}
							}
						}

						ASSERT_EX(false, TempString("Unknown vertex stream member '{}'", memberName));
					}
				}

				auto op = m_builder.createStub<shader::StubOpcodeAccessMember>();
				op->value = exportOpcode(structNode);
				op->name = memberName;

				// link to the structure member
				if (structNode->dataType().isComposite())
				{
					const auto& composite = structNode->dataType().composite();
					if (const auto* structType = exportCompositeStruct(&composite))
					{
						for (const auto* member : structType->members)
						{
							if (member->name == memberName)
							{
								op->member = member;
								break;
							}
						}
					}
				}

				return op;
			}

			case OpCode::AccessArray:
			{
				auto context = exportOpcode(node->children()[0]);

				// convert to resource load or resource element
				if (const auto* resourceRef = context->asOpcodeResourceRef())
				{
					if (ResourceSupportsResourceLoad(resourceRef))
					{
						auto op = m_builder.createStub<shader::StubOpcodeResourceLoad>();
						op->resourceRef = resourceRef;
						op->address = exportOpcode(node->children()[1]);
						op->numAddressComponents = ExtractComponentCount(node->children()[1]->dataType());
						op->numValueComponents = ExtractComponentCount(node->dataType());
						return op;
					}
					else if (ResourceSupportsResourceElement(resourceRef))
					{
						auto op = m_builder.createStub<shader::StubOpcodeResourceElement>();
						op->resourceRef = resourceRef;
						op->address = exportOpcode(node->children()[1]);
						op->numAddressComponents = ExtractComponentCount(node->children()[1]->dataType());
						op->numValueComponents = ExtractComponentCount(node->dataType());
						return op;
					}
				}

				// generic array access
				auto op = m_builder.createStub<shader::StubOpcodeAccessArray>();
				op->arrayOp = context;
				op->arrayType = exportDataType(node->dataType());

				if (node->children()[1]->opCode() == OpCode::Const && node->children()[1]->dataType().isNumericalScalar())
				{
					op->indexOp = nullptr;
					op->staticIndex = node->children()[1]->dataValue().component(0).valueInt();
				}
				else
				{
					op->indexOp = exportOpcode(node->children()[1]);
					op->staticIndex = 0;
				}
				return op;
			}

			case OpCode::ReadSwizzle: // read swizzle (extracted from member access for simplicity)
				return exportReadSwizzle(node, node->extraData().m_mask);

			case OpCode::Cast:// data cast
			{
				auto op = m_builder.createStub<shader::StubOpcodeCast>();
				switch (node->extraData().m_castMode)
				{
				case TypeMatchTypeConv::ConvToBool:
					op->generalType = shader::ScalarType::Boolean;
					break;

				case TypeMatchTypeConv::ConvToFloat:
					op->generalType = shader::ScalarType::Float;
					break;

				case TypeMatchTypeConv::ConvToInt:
					op->generalType = shader::ScalarType::Int;
					break;

				case TypeMatchTypeConv::ConvToUint:
					op->generalType = shader::ScalarType::Uint;
					break;
				}

				op->targetType = exportDataType(node->dataType());
				op->value = exportOpcode(node->children()[0]);
				return op;
			}

			case OpCode::CreateMatrix:// matrix constructor
			{
				auto op = m_builder.createStub<shader::StubOpcodeCreateMatrix>();
				op->typeDecl = exportDataType(node->dataType())->asMatrixTypeDecl();
				op->elements = std::move(exportChildOpcodes(node));
				return op;
			}

			case OpCode::CreateVector:// vector constructor
			{
				auto op = m_builder.createStub<shader::StubOpcodeCreateVector>();
				op->typeDecl = exportDataType(node->dataType())->asVectorTypeDecl();
				op->elements = std::move(exportChildOpcodes(node));
				return op;
			}

			case OpCode::CreateArray:// array constructor
			{
				auto op = m_builder.createStub<shader::StubOpcodeCreateArray>();
				op->arrayTypeDecl = exportDataType(node->dataType())->asArrayTypeDecl();
				op->elements = std::move(exportChildOpcodes(node));
				return op;
			}

			case OpCode::Nop:// do-nothing plug
				return nullptr;

			case OpCode::Loop:// repeats code while condition occurs
			{
				auto* op = m_builder.createStub<shader::StubOpcodeLoop>();
				op->init = exportOpcode(node->children()[0]);
				op->condition = exportOpcode(node->children()[1]);
				op->increment = exportOpcode(node->children()[2]);
				op->body = exportOpcode(node->children()[3]);

				for (const auto& attr : node->extraData().m_attributesMap)
				{
					if (attr.key == "unroll")
						op->unrollHint = 1;
					else if (attr.key == "dont_unroll")
						op->unrollHint = -1;
					else if (attr.key == "loop")
						op->unrollHint = 1;
					else if (attr.key == "dependency_infinite")
						op->dependencyLength = -1;
					else if (attr.key == "dependency_length")
						attr.value.match(op->dependencyLength);
				}

				if (op->unrollHint || op->dependencyLength)
					m_activeStage->usesControlFlowHints = true;
						
				return op;
			}

			case OpCode::Break:// breaks the innermost while loop
				return m_builder.createStub<shader::StubOpcodeBreak>();

			case OpCode::Continue:// continues the innermost while loop
				return m_builder.createStub<shader::StubOpcodeContinue>();

			case OpCode::Exit: // "exit" the shader without writing anything (discard)
				m_activeStage->usesDiscard = true;
				return m_builder.createStub<shader::StubOpcodeExit>();

			case OpCode::Return: // return from function
			{
				auto* op = m_builder.createStub<shader::StubOpcodeReturn>();
				if (!node->children().empty())
					op->value = exportOpcode(node->children()[0]);
				else
					op->value = nullptr;
				return op;
			}

			case OpCode::IfElse:// if-else condition 
			{
				InplaceArray<const shader::StubOpcode*, 10> conditions;
				InplaceArray<const shader::StubOpcode*, 10> statements;

				const auto numBranches = node->children().size() / 2;
				conditions.reserve(numBranches);
				statements.reserve(numBranches);

				for (uint32_t i = 0; i < numBranches; ++i)
				{
					conditions.pushBack(exportOpcode(node->children()[i * 2 + 0]));
					statements.pushBack(exportOpcode(node->children()[i * 2 + 1]));
				}

				auto* op = m_builder.createStub<shader::StubOpcodeIfElse>();
				op->conditions = m_builder.createArray(conditions);
				op->statements = m_builder.createArray(statements);

				if (node->children().size() > (numBranches * 2))
					op->elseStatement = exportOpcode(node->children().back());
				else
					op->elseStatement = nullptr;

				for (const auto& attr : node->extraData().m_attributesMap)
				{
					if (attr.key == "flatten")
						op->branchHint = -1;
					else if (attr.key == "dont_flatten")
						op->branchHint = 1;
					else if (attr.key == "branch")
						op->branchHint = 1;
				}

				if (op->branchHint)
					m_activeStage->usesControlFlowHints = true;

				return op;
			}

			//--

			// opcodes that should not appear
			case OpCode::Ident:// unresolved identifier:muted to FuncRef or ParamRef
			case OpCode::This:// current program instance
			case OpCode::ProgramInstanceParam:// program instance initialization variable constructor
			case OpCode::ProgramInstance:// program instance constructor
			case OpCode::ResourceTable:// a resource table type
			case OpCode::FuncRef:// reference to a function
			case OpCode::ListElement:// list should be consume by now
				ASSERT(!"Unexpected opcode");
				return nullptr;

		}

		ASSERT(!"Invalid code node to export");
		return nullptr;
	}
};

//--

static void DumpShader(const shader::StubProgram* program, const AssembledShader& data, StringView contextPath, StringView contextOptions)
{
	StringBuilder txt;
	{
		shader::StubDebugPrinter printer(txt);

		program->dump(printer); // map
		printer.enableOutput();

		printer << "------------------------------------------\n";
		printer << "-- SHADER                               --\n";
		printer << "------------------------------------------\n\n";
		program->dump(printer); // print

		printer << "\n------------------------------------------\n";
		printer << "-- METADATA                             --\n";
		printer << "------------------------------------------\n\n";
		data.metadata->print(printer);
	}

	const auto targetPath = shader::AssembleDumpFilePath(contextPath, contextOptions, "stubs");
	SaveFileFromString(targetPath, txt.view());

	TRACE_INFO("Saved shader dump to '{}'", targetPath);
}

extern bool Assemblestubs(LinearAllocator& mem, CodeLibrary& lib, AssembledShader& outAssembledShader, StringView contextPath, StringView contextOptions, parser::IErrorReporter& err)
{
	// extract s
	bunderSetup bundle;
	if (!Extractbundle(lib.exports(), lib, bundle, err))
		return false;

	// create extractor, will help with building the stubs
	StubExtractionHelper helper(mem, lib);
	if (const auto* program = helper.exportProgram(bundle, contextPath, contextOptions, err))
	{
		// pack blob
		outAssembledShader.blob = program->pack(1);

		// export metadata
		const auto key = CRC64().append(outAssembledShader.blob.data(), outAssembledShader.blob.size()).crc();
		outAssembledShader.metadata = ShaderMetadata::BuildFromStubs(program, key);

		// dump if requested
		if (cvDumpstubs.get())
			DumpShader(program, outAssembledShader, contextPath, contextOptions);

		return true;
	}

	// not exported
	return false;
}

//--

END_BOOMER_NAMESPACE_EX(gpu::compiler)
