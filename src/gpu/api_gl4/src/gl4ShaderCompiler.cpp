/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "gl4ShaderCompiler.h"

#define ARG(x) Arg(op->arguments[x])

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

//--

ShaderSharedBindings::ShaderSharedBindings(const shader::StubProgram* program)
{
	// binding slot assignment
	// NOTE: MUST MATCH THE ONE IN GLSL CODE GENERATOR!!!!
	uint8_t numUniformBuffers = 0;
	uint8_t numStorageBuffers = 0;
	uint8_t numImages = 0;
	uint8_t numTextures = 0;
	uint8_t numSamplers = 0;
	for (const auto* descriptor : program->descriptors)
	{
		for (const auto* member : descriptor->members)
		{
			if (member->asDescriptorMemberSampler())
				continue; // samplers are not found directly

			if (const auto* obj = member->asDescriptorMemberConstantBuffer())
				m_descriptorResourceMapping[obj] = numUniformBuffers++;
			else if (const auto* obj = member->asDescriptorMemberFormatBuffer())
				m_descriptorResourceMapping[obj] = numImages++;
			else if (const auto* obj = member->asDescriptorMemberStructuredBuffer())
				m_descriptorResourceMapping[obj] = numStorageBuffers++;
			else if (const auto* obj = member->asDescriptorMemberSampledImage())
				m_descriptorResourceMapping[obj] = numTextures++;
			else if (const auto* obj = member->asDescriptorMemberImage())
				m_descriptorResourceMapping[obj] = numImages++;
			else if (const auto* obj = member->asDescriptorMemberSampledImageTable())
				m_descriptorResourceMapping[obj] = numUniformBuffers++;
			else
				ASSERT(!"Unknown descrptor type");
		}
	}

	// bind outputs to slots
	for (const auto* stage : program->stages)
	{
		// TODO: sort to allow better reuse of vertex shaders or sth ?

		char localOutputIndex = 0;
		for (const auto* stageOutputs : stage->outputs)
			m_stageOutputMapping[stageOutputs] = localOutputIndex++;
	}

	// connect stage inputs
	for (const auto* stage : program->stages)
	{
		for (const auto* stageInput : stage->inputs)
		{
			ASSERT(stageInput->prevStageOutput);

			short outputLayoutIndex = -1;
			m_stageOutputMapping.find(stageInput->prevStageOutput, outputLayoutIndex);
			ASSERT(outputLayoutIndex != -1);

			m_stageInputMappings[stageInput] = outputLayoutIndex;
		}
	}
}

//--

ShaderCodePrinter::ShaderCodePrinter(const shader::StubProgram* program, const shader::StubStage* stage, const ShaderSharedBindings& sharedBindings, ShaderFeatureMask supportedFeatures)
	: m_program(program)
	, m_stage(stage)
	, m_stageIndex(stage->stage)
	, m_sharedBindings(sharedBindings)
	, m_featureMask(supportedFeatures)
{
	m_functions["ddx"_id] = "dFdx";
	m_functions["ddx_coarse"_id] = "dFdxCoarse";
	m_functions["ddx_fine"_id] = "dFdxFine";
	m_functions["ddy"_id] = "dFdy";
	m_functions["ddy_coarse"_id] = "dFdyCoarse";
	m_functions["ddy_fine"_id] = "dFdyFine";
	m_functions["fwidth"_id] = "fwidth";

	m_functions["frac"_id] = "fract";
	m_functions["lerp"_id] = "mix";
	m_functions["mad"_id] = "fma";
	m_functions["atan2"_id] = "atan";

	m_functions["__mul"_id] = "*";
	m_functions["__vsmul"_id] = "*";
	m_functions["__svmul"_id] = "*";
	m_functions["__msmul"_id] = "*";
	m_functions["__smmul"_id] = "*";
	m_functions["__mvmul"_id] = "*";
	m_functions["__vmmul"_id] = "*";
	m_functions["__mmmul"_id] = "*";
	m_functions["__add"_id] = "+";
	m_functions["__sub"_id] = "-";
	m_functions["__div"_id] = "/";
	m_functions["__logicAnd"_id] = "&&";
	m_functions["__logicOr"_id] = "||";
	m_functions["__xor"_id] = "^";
	m_functions["__or"_id] = "|";
	m_functions["__and"_id] = "&";
	m_functions["__shl"_id] = "<<";
	m_functions["__shr"_id] = ">>";
	m_functions["__neg"_id] = "-";
	m_functions["__logicalNot"_id] = "!";
	m_functions["__not"_id] = "~";

	m_functions["saturate"_id] = &ShaderCodePrinter::printFuncSaturate;
	m_functions["__select"_id] = &ShaderCodePrinter::printFuncSelect;
	m_functions["__mod"_id] = &ShaderCodePrinter::printFuncMod;				
	m_functions["__neq"_id] = &ShaderCodePrinter::printFuncNotEqual;
	m_functions["__eq"_id] = &ShaderCodePrinter::printFuncEqual;
	m_functions["__ge"_id] = &ShaderCodePrinter::printFuncGreaterEqual;
	m_functions["__gt"_id] = &ShaderCodePrinter::printFuncGreaterThen;
	m_functions["__le"_id] = &ShaderCodePrinter::printFuncLessEqual;
	m_functions["__lt"_id] = &ShaderCodePrinter::printFuncLessThen;

	m_functions["imageLoadSample"_id] = "imageLoad";
	m_functions["imageStoreSample"_id] = "imageStore";

	m_functions["atomicIncrement"_id] = &ShaderCodePrinter::printFuncAtomicIncrement;
	m_functions["atomicDecrement"_id] = &ShaderCodePrinter::printFuncAtomicDecrement;
	m_functions["atomicAdd"_id] = &ShaderCodePrinter::printFuncAtomicAdd;
	m_functions["atomicSubtract"_id] = &ShaderCodePrinter::printFuncAtomicSubtract;
	m_functions["atomicMin"_id] = &ShaderCodePrinter::printFuncAtomicMin;
	m_functions["atomicMax"_id] = &ShaderCodePrinter::printFuncAtomicMax;
	m_functions["atomicOr"_id] = &ShaderCodePrinter::printFuncAtomicOr;
	m_functions["atomicAnd"_id] = &ShaderCodePrinter::printFuncAtomicAnd;
	m_functions["atomicXor"_id] = &ShaderCodePrinter::printFuncAtomicXor;
	m_functions["atomicExchange"_id] = &ShaderCodePrinter::printFuncAtomicExchange;
	m_functions["atomicCompSwap"_id] = &ShaderCodePrinter::printFuncAtomicCompareSwap;

	m_functions["texture"_id] = "texture";
	m_functions["textureGatherOffset"_id] = &ShaderCodePrinter::printFuncTextureGatherOffset;
	m_functions["textureGather"_id] = &ShaderCodePrinter::printFuncTextureGather;
	m_functions["textureLod"_id] = "textureLod";
	m_functions["textureLodOffset"_id] = "textureLodOffset";
	m_functions["textureBias"_id] = "texture";
	m_functions["textureBiasOffset"_id] = "textureOffset";
	m_functions["textureSizeLod"_id] = "textureSize";
	m_functions["textureLoadLod"_id] = "texelFetch";
	m_functions["textureLoadLodOffset"_id] = "texelFetchOffset";
	m_functions["textureLoadSample"_id] = "texelFetch";
	m_functions["textureLoadSampleOffset"_id] = "texelFetchOffset";
	m_functions["textureDepthCompare"_id] = &ShaderCodePrinter::printFuncTextureDepthCompare;
	m_functions["textureDepthCompareOffset"_id] = &ShaderCodePrinter::printFuncTextureDepthCompareOffset;
}

void ShaderCodePrinter::printCode(StringBuilder& f)
{
	printHeader(f);
	printStageAttributes(f);
	printVertexInputs(f);
	printStructureDeclarations(f);
	printGlobalConstants(f);
	printResources(f);
	printBuiltIns(f);
	printStageInputs(f);
	printStageOutputs(f);
	printSharedMemory(f);
	printFunctions(f);
}

static StringView GetAttribute(const StubPseudoArray<shader::StubAttribute>& attributes, StringID key, StringView defaultValue)
{
	for (const auto* attr : attributes)
		if (attr->name == key)
			return attr->value;

	return defaultValue;
}

static bool HasAttribute(const StubPseudoArray<shader::StubAttribute>& attributes, StringID key)
{
	for (const auto* attr : attributes)
		if (attr->name == key)
			return true;

	return false;
}

static int GetAttributeInt(const StubPseudoArray<shader::StubAttribute>& attributes, StringID key, int defaultValue)
{
	int ret = defaultValue;

	for (const auto* attr : attributes)
	{
		if (attr->name == key)
		{
			attr->value.match(ret);
			break;
		}
	}

	return ret;
}

void ShaderCodePrinter::printHeader(StringBuilder& f)
{
	uint32_t linesSoFar = 4;

	f << "// Automatically Generated Shader, do not edit\n";
	if (!m_program->depotPath.empty())
	{
		f.appendf("// Source Path: {}\n", m_program->depotPath);
		linesSoFar += 1;
	}
	if (!m_program->options.empty())
	{
		f.appendf("// Source Options: {}\n", m_program->options);
		linesSoFar += 1;	
	}

	f << "\n";
	f << "#version 460\n";
	if (m_featureMask.test(ShaderFeatureBit::ControlFlowHints))
		f << "#extension GL_EXT_control_flow_attributes : require\n";

	f << "\n";

	f << "// Shader is based on following files:\n";
	for (const auto* file : m_program->files)
		f << "// File: '" << file->depotPath << "'\n";

	if (!m_program->files.empty())
		f << "\n";

	if (m_program->renderStates)
	{
		f << "/* Expected render states:\n";
		m_program->renderStates->states.print(f);
		f << "*/\n\n";
	}
}

void ShaderCodePrinter::printStageAttributes(StringBuilder& f)
{
	if (m_stageIndex == ShaderStage::Compute)
	{
		int sizeX = GetAttributeInt(m_stage->entryFunction->attributes, "local_size_x"_id, 8);
		int sizeY = GetAttributeInt(m_stage->entryFunction->attributes, "local_size_y"_id, 8);
		int sizeZ = GetAttributeInt(m_stage->entryFunction->attributes, "local_size_z"_id, 1);
		f.appendf("layout(local_size_x={}, local_size_y={}, local_size_z={}) in;\n", sizeX, sizeY, sizeZ);
		f << "\n";
	}
	else if (m_stageIndex == ShaderStage::Geometry)
	{
		auto input = GetAttribute(m_stage->entryFunction->attributes, "input"_id, "");
		auto output = GetAttribute(m_stage->entryFunction->attributes, "output"_id, "");
		auto maxVertices = GetAttributeInt(m_stage->entryFunction->attributes, "max_vertices"_id, -1); 

		if (input == "points")
			f.append("layout(points) in;\n");
		else if (input == "lines")
			f.append("layout(lines) in;\n");
		else if (input == "lines_adjacency")
			f.append("layout(lines_adjacency) in;\n");
		else if (input == "triangles")
			f.append("layout(triangles) in;\n");
		else if (input == "triangles_adjacency")
			f.append("layout(triangles_adjacency) in;\n");

		if (output == "points")
			f.appendf("layout(points, max_vertices={}) out;\n", maxVertices);
		else if (output == "line_strip")
			f.appendf("layout(line_strip, max_vertices={}) out;\n", maxVertices);
		else if (output == "triangle_strip")
			f.appendf("layout(triangle_strip, max_vertices={}) out;\n", maxVertices);

		auto numInvocations = GetAttributeInt(m_stage->entryFunction->attributes, "invocations"_id, 1);
		if (numInvocations > 1)
			f.appendf("layout(invocations={}) in;\n", numInvocations);

		f << "\n";
	}
	else if (m_stageIndex == ShaderStage::Pixel)
	{
		if (HasAttribute(m_stage->entryFunction->attributes, "early_fragment_tests"_id))
			f.append("layout(early_fragment_tests) in;\n");
	}
}

void ShaderCodePrinter::printType(StringBuilder& f, const shader::StubTypeDecl* type, StringView varName)
{
	InplaceArray<int, 4> arrayCounts;
	while (const auto* arrayType = type->asArrayTypeDecl())
	{
		arrayCounts.pushBack(arrayType->count);
		type = arrayType->innerType;
	}

	if (const auto* scalar = type->asScalarTypeDecl())
	{
		switch (scalar->type)
		{
			case shader::ScalarType::Void: f << "void"; break;
			case shader::ScalarType::Half: f << "lowp float"; break;
			case shader::ScalarType::Float: f << "float"; break;
			case shader::ScalarType::Double: f << "double"; break;
			case shader::ScalarType::Int: f << "int"; break;
			case shader::ScalarType::Uint: f << "uint"; break;
			case shader::ScalarType::Int64: f << "int64"; break;
			case shader::ScalarType::Uint64: f << "uint64"; break;
			case shader::ScalarType::Boolean: f << "bool"; break;
			default: ASSERT(!"Unknown type");
		}
	}
	else if (const auto* vector = type->asVectorTypeDecl())
	{
		const char* prefix = "";
		switch (vector->type)
		{
			case shader::ScalarType::Int: prefix = "i"; break;
			case shader::ScalarType::Uint: prefix = "u"; break;
			case shader::ScalarType::Boolean: prefix = "b"; break;
			case shader::ScalarType::Double: prefix = "d"; break;
		}

		f.appendf("{}vec{}", prefix, vector->componentCount);
	}
	else if (const auto* matrix = type->asMatrixTypeDecl())
	{
		const char* prefix = "";
		switch (matrix->type)
		{
			case shader::ScalarType::Double: prefix = "d"; break;
		}

		if (matrix->componentCount == matrix->rowCount)
			f.appendf("{}mat{}", prefix, matrix->componentCount);
		else
			f.appendf("{}mat{}x{}", prefix, matrix->componentCount, matrix->rowCount);
	}
	else if (const auto* compound = type->asStructTypeDecl())
	{
		f << compound->structType->name;
	}
	else
	{
		ASSERT(!"Unsupported type");
	}

	if (varName)
		f << " " << varName;

	for (auto count : arrayCounts)
		if (count > 0)
			f.appendf("[{}]", count);
		else
			f << "[]";
}

void ShaderCodePrinter::printVertexInputs(StringBuilder& f)
{
	if (m_stageIndex != ShaderStage::Vertex)
		return;

	uint8_t attribIndex = 0;
	for (const auto* stream : m_program->vertexStreams)
	{
		for (const auto* element : stream->elements)
		{
			f.appendf("in layout(location = {}) ", attribIndex);

			auto name = StringBuf(TempString("_vi_{}_{}", stream->name, element->name));
			m_paramNames[element] = name;

			printType(f, element->type, name);
			f.append(";\n");

			attribIndex += 1;
		}

		f << "\n";
	}

	if (!m_program->vertexStreams.empty())
		f << "\n";
}

void ShaderCodePrinter::printStructureDeclarations(StringBuilder& f)
{
	for (const auto* s : m_stage->structures)
	{
		f.appendf("struct {} {\n", s->name);

		for (const auto* m : s->members)
		{
			f.append("  ");
			printType(f, m->type, m->name.view());
			f.append(";\n");
		}

		f.append("};\n\n");
	}
}

static void PrintScalar(StringBuilder& f, shader::ScalarType type, const uint32_t*& data)
{
	switch (type)
	{
		case shader::ScalarType::Boolean:
			f << (*data ? "true" : "false");
			break;

		case shader::ScalarType::Float:
			f.appendf("{}", Prec(*(const float*)data, 10));
			break;

		case shader::ScalarType::Uint:
			f.appendf("{}", *data);
			break;

		case shader::ScalarType::Int:
			f.appendf("{}", *(const int*)data);
			break;
	}

	data += 1;
}

void ShaderCodePrinter::printConstant(StringBuilder& f, const shader::StubTypeDecl* type, const uint32_t*& data)
{
	if (const auto* arr = type->asArrayTypeDecl())
	{
		printType(f, arr); // vec3[10]
		f << " (\n";

		const bool scalarArray = (nullptr != arr->innerType->asScalarTypeDecl());
		for (uint32_t i = 0; i < arr->count; ++i)
		{
			if (i > 0 && !scalarArray)
				f << ",\n";

			printConstant(f, arr->innerType, data);
		}

		f << ")";
	}
	else if (const auto* vec = type->asVectorTypeDecl())
	{
		printType(f, vec); // vec3
		f << "(";

		for (uint32_t i = 0; i < vec->componentCount; ++i)
		{
			if (i > 0)
				f << ", ";
			PrintScalar(f, vec->type, data);
		}

		f << ")";
	}
	else if (const auto* matrix = type->asMatrixTypeDecl())
	{
		printType(f, matrix); // vec3
		f << "(";

		const auto totalComponents = matrix->componentCount * matrix->rowCount;
		for (uint32_t i = 0; i < totalComponents; ++i)
		{
			if (i > 0)
				f << ", ";
			PrintScalar(f, matrix->type, data);
		}

		f << ")";
	}
	else if (const auto* scalar = type->asScalarTypeDecl())
	{
		PrintScalar(f, scalar->type, data);
	}
	else
	{
		ASSERT(!"Unsupported data");
	}
}

void ShaderCodePrinter::printGlobalConstants(StringBuilder& f)
{
	for (const auto* c : m_stage->globalConstants)
	{
		const auto name = StringBuf(TempString("_CONST_{}", c->index));

		f.append("const ");
		printType(f, c->typeDecl, name);
		f.append(" = ");

		const auto* dataPtr = (const uint32_t*)c->data;
		printConstant(f, c->typeDecl, dataPtr);

		f.append(";\n");

		m_paramNames[c] = name;
	}

	if (!m_stage->globalConstants.empty())
		f.append("\n");
}

static void PrintTextureDeclaration(StringBuilder& f, ImageViewType type, bool ms, bool depth)
{
	f << "sampler";

	switch (type)
	{
	case ImageViewType::View1D:
	case ImageViewType::View1DArray:
		f << "1D"; break;

	case ImageViewType::View2D:
	case ImageViewType::View2DArray:
		f << "2D"; break;

	case ImageViewType::View3D:
		f << "3D"; break;

	case ImageViewType::ViewCube:
	case ImageViewType::ViewCubeArray:
		f << "Cube"; break;
	}

	if (ms)
		f << "MS";

	switch (type)
	{
	case ImageViewType::View1DArray:
	case ImageViewType::View2DArray:
	case ImageViewType::ViewCubeArray:
		f << "Array";
	}

	if (depth)
		f << "Shadow";
}

static void PrintImageFormat(StringBuilder& f, ImageFormat format)
{
	auto& info = GetImageFormatInfo(format);
	f << info.shaderName;
}

static void PrintImageDeclaration(StringBuilder& f, ImageViewType type, ImageFormat format, bool ms, bool depth)
{
	auto formatClass = GetImageFormatInfo(format).formatClass;
	if (formatClass == ImageFormatClass::UINT)
		f << "u";
	else if (formatClass == ImageFormatClass::INT)
		f << "i";

	f << "image";

	switch (type)
	{
	case ImageViewType::View1D:
	case ImageViewType::View1DArray:
		f << "1D"; break;

	case ImageViewType::View2D:
	case ImageViewType::View2DArray:
		f << "2D"; break;

	case ImageViewType::View3D:
		f << "3D"; break;

	case ImageViewType::ViewCube:
	case ImageViewType::ViewCubeArray:
		f << "Cube"; break;
	}

	if (ms)
		f << "MS";

	switch (type)
	{
	case ImageViewType::View1DArray:
	case ImageViewType::View2DArray:
	case ImageViewType::ViewCubeArray:
		f << "Array";
	}

	if (depth)
		f << "Shadow";

	f << " ";
}

void ShaderCodePrinter::printResources(StringBuilder& f)
{
	for (const auto* m : m_stage->descriptorMembers)
	{
		if (m->asDescriptorMemberSampler())
			continue;

		short binding = -1;
		m_sharedBindings.m_descriptorResourceMapping.find(m, binding);
		ASSERT(binding != -1);

		if (const auto* obj = m->asDescriptorMemberConstantBuffer())
		{
			f.appendf("layout(binding = {}, std140) ", binding);
			f.appendf("uniform _{}_{} {\n", m->descriptor->name, m->name);

			for (const auto* m : obj->elements)
			{
				auto name = StringBuf(TempString("_cb{}_x{}_x{}", binding, obj->descriptor->name, m->name)); // _cb0_ObjectParams_LocalToWorld
				m_paramNames[m] = name;

				f.appendf("  layout(offset = {}) ", m->linearOffset);
				printType(f, m->type, name);
				f.appendf("; // align={}, size={}", m->linearAlignment, m->linearSize);

				if (m->type->asArrayTypeDecl())
					f.appendf(" arrayCount={}, arrayStride={}", m->linearArrayCount, m->linearArrayStride);

				f << "\n";
			}

			f.append("};\n\n");
		}
		else if (const auto* obj = m->asDescriptorMemberSampledImage())
		{
			f.appendf("layout(binding = {}) uniform ", binding);
			PrintTextureDeclaration(f, obj->viewType, obj->multisampled, obj->depth);

			auto name = StringBuf(TempString("_tex{}_x{}_x{}", binding, m->descriptor->name, m->name)); // _tex0_Material_DiffuseMap
			m_paramNames[m] = name;

			f.appendf(" {};\n", name);
		}
		else if (const auto* obj = m->asDescriptorMemberImage())
		{
			f.appendf("layout(binding = {}, ", binding);
			PrintImageFormat(f, obj->format);
			f.appendf(") {}uniform ", obj->writable ? " " : "readonly ");
			PrintImageDeclaration(f, obj->viewType, obj->format, false, false);

			auto name = StringBuf(TempString("_img{}_x{}_x{}", binding, m->descriptor->name, m->name)); // _img5_Lights_VoxelGrid
			m_paramNames[m] = name;

			f.appendf(" {};\n", name);
		}
		else if (const auto* obj = m->asDescriptorMemberFormatBuffer())
		{
			const auto writeonly = HasAttribute(obj->attributes, "writeonly"_id);
			const auto readonly = !obj->writable || HasAttribute(obj->attributes, "readonly"_id);
			const char* modeText = readonly ? "readonly " : (writeonly ? "writeonly " : "");

			f.appendf("layout(binding = {}, ", binding);
			PrintImageFormat(f, obj->format);

			auto formatClass = GetImageFormatInfo(obj->format).formatClass;
			if (formatClass == ImageFormatClass::INT)
				f.appendf(") uniform {} iimageBuffer ", modeText);
			else if (formatClass == ImageFormatClass::UINT)
				f.appendf(") uniform {} uimageBuffer ", modeText);
			else
				f.appendf(") uniform {} imageBuffer ", modeText);

			auto name = StringBuf(TempString("_buf{}_x{}_x{}", binding, m->descriptor->name, m->name)); // _buf2_Test_TexelColors
			m_paramNames[m] = name;

			f.appendf("{};\n\n", name);
		}
		else if (const auto* obj = m->asDescriptorMemberStructuredBuffer())
		{
			const auto writeonly = HasAttribute(obj->attributes, "writeonly"_id);
			const auto readonly = !obj->writable || HasAttribute(obj->attributes, "readonly"_id);
			const char* modeText = readonly ? "readonly " : (writeonly ? "writeonly " : "");

			auto name = StringBuf(TempString("_sbuf{}_x{}_x{}", binding, m->descriptor->name, m->name)); // _sbuf2_Lights_LightInfo
			m_paramNames[m] = name;

			f.appendf("layout(binding = {}, std430) {} buffer {}_BUFFER\n{\n", binding, modeText, name);
			f.appendf("  {} data[1];\n", obj->layout->name);
			f.appendf("} {};\n\n", name);
		}
		else if (const auto* obj = m->asDescriptorMemberSampledImageTable())
		{
			ASSERT(!"Glorious day it is!");
		}
		else if (const auto* obj = m->asDescriptorMemberSampler())
		{
			// not emitted directly
		}
		else
		{
			ASSERT(!"Invalid descriptor member");
		}
	}
}

void ShaderCodePrinter::printBuiltIns(StringBuilder& f)
{
	shader::ShaderBuiltInMask buildInMask;

	for (const auto* b : m_stage->builtins)
		buildInMask |= b->builinType;

	if (m_stageIndex == ShaderStage::Vertex)
	{
		if (buildInMask.test(shader::ShaderBuiltIn::VertexID))
			f.append("in int gl_VertexID;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::InstanceID))
			f.append("in int gl_InstanceID;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::DrawID))
			f.append("in int gl_DrawID;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::BaseVertex))
			f.append("in int gl_BaseVertex;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::BaseInstance))
			f.append("in int gl_BaseInstance;\n");

		if (buildInMask.test(shader::ShaderBuiltIn::Position) || buildInMask.test(shader::ShaderBuiltIn::PointSize) || buildInMask.test(shader::ShaderBuiltIn::ClipDistance))
		{
			f.append("out gl_PerVertex {\n");

			if (buildInMask.test(shader::ShaderBuiltIn::Position))
				f.append("  vec4 gl_Position;\n");
			if (buildInMask.test(shader::ShaderBuiltIn::PointSize))
				f.append("  float gl_PointSize;\n");
			if (buildInMask.test(shader::ShaderBuiltIn::ClipDistance))
				f.append("  float gl_ClipDistance[];\n");

			f.append("};\n");
		}
	}
	else if (m_stageIndex == ShaderStage::Pixel)
	{
		if (buildInMask.test(shader::ShaderBuiltIn::FragCoord))
			f.append("in vec4 gl_FragCoord;\n");
		/*if (buildInMask.test(shader::ShaderBuiltIn::FrontFacing))
			f.append("in bool gl_FrontFacing;\n");*/
		if (buildInMask.test(shader::ShaderBuiltIn::PointCoord))
			f.append("in vec2 gl_PointCoord;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::SampleID))
			f.append("in int gl_SampleID;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::SamplePosition))
			f.append("in vec2 gl_SamplePosition;\n");
		//if (buildInMask.test(shader::ShaderBuiltIn::SampleMaskIn))
			//f.append("in int gl_SampleMaskIn[];\n");
		if (buildInMask.test(shader::ShaderBuiltIn::ClipDistance))
			f.append("in float gl_ClipDistance[];\n");
		if (buildInMask.test(shader::ShaderBuiltIn::PrimitiveID))
			f.append("in int gl_PrimitiveID;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::Layer))
			f.append("in int gl_Layer;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::ViewportIndex))
			f.append("in int gl_ViewportIndex;\n");

		if (buildInMask.test(shader::ShaderBuiltIn::Depth))
			f.append("out float gl_FragDepth;\n"); // TODO: depth conditions!
		//if (buildInMask.test(shader::ShaderBuiltIn::SampleMask))
//						f.append("out int gl_SampleMask[];\n");
		/*if (buildInMask.test(shader::ShaderBuiltIn::Target0))
			f.append("out layout(location=0) vec4 _ColorTarget0;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::Target1))
			f.append("out layout(location=1) vec4 _ColorTarget1;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::Target2))
			f.append("out layout(location=2) vec4 _ColorTarget2;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::Target3))
			f.append("out layout(location=3) vec4 _ColorTarget3;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::Target4))
			f.append("out layout(location=4) vec4 _ColorTarget4;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::Target5))
			f.append("out layout(location=5) vec4 _ColorTarget5;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::Target6))
			f.append("out layout(location=6) vec4 _ColorTarget6;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::Target7))
			f.append("out layout(location=7) vec4 _ColorTarget7;\n");*/


	}
	else if (m_stageIndex == ShaderStage::Geometry)
	{
		if (buildInMask.test(shader::ShaderBuiltIn::PositionIn) || buildInMask.test(shader::ShaderBuiltIn::PointSizeIn) || buildInMask.test(shader::ShaderBuiltIn::ClipDistanceIn))
		{
			f.append("in gl_PerVertex {\n");

			if (buildInMask.test(shader::ShaderBuiltIn::PositionIn))
				f.append("  vec4 gl_Position;\n");
			if (buildInMask.test(shader::ShaderBuiltIn::PointSizeIn))
				f.append("  float gl_PointSize;\n");
			if (buildInMask.test(shader::ShaderBuiltIn::ClipDistanceIn))
				f.append("  float gl_ClipDistance[];\n");

			f.append("} gl_in[];\n");
		}

		if (buildInMask.test(shader::ShaderBuiltIn::PrimitiveIDIn))
			f.append("in int gl_PrimitiveIDIn;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::InvocationID))
			f.append("in int gl_InvocationID;\n");					

		if (buildInMask.test(shader::ShaderBuiltIn::Position) || buildInMask.test(shader::ShaderBuiltIn::PointSize) || buildInMask.test(shader::ShaderBuiltIn::ClipDistance))
		{
			f.append("out gl_PerVertex {\n");

			if (buildInMask.test(shader::ShaderBuiltIn::Position))
				f.append("  vec4 gl_Position;\n");
			if (buildInMask.test(shader::ShaderBuiltIn::PointSize))
				f.append("  float gl_PointSize;\n");
			if (buildInMask.test(shader::ShaderBuiltIn::ClipDistance))
				f.append("  float gl_ClipDistance[];\n");

			f.append("};\n");
		}

		if (buildInMask.test(shader::ShaderBuiltIn::PrimitiveID))
			f.append("out int gl_PrimitiveID;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::Layer))
			f.append("out int gl_Layer;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::ViewportIndex))
			f.append("out int gl_ViewportIndex;\n");
	}
	else if (m_stageIndex == ShaderStage::Compute)
	{
		/*if (buildInMask.test(shader::ShaderBuiltIn::NumWorkGroups))
			f.append("in uvec3 gl_NumWorkGroups;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::WorkGroupID))
			f.append("in uvec3 gl_WorkGroupID;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::LocalInvocationID))
			f.append("in uvec3 gl_LocalInvocationID;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::GlobalInvocationID))
			f.append("in uvec3 gl_GlobalInvocationID;\n");
		if (buildInMask.test(shader::ShaderBuiltIn::LocalInvocationIndex))
			f.append("in uint gl_LocalInvocationIndex;\n");*/
	}

	if (!buildInMask.empty())
		f << "\n";
}

void ShaderCodePrinter::printStageInputs(StringBuilder& f)
{
	for (const auto* s : m_stage->inputs)
	{
		// flat
		if (m_stageIndex == ShaderStage::Pixel && HasAttribute(s->attributes, "flat"_id))
			f << "flat ";

		/*if (var.m_name == "gl_FragCoord")
			f << "layout(origin_upper_left) ";*/

		short binding = -1;
		m_sharedBindings.m_stageInputMappings.find(s, binding);
		f.appendf("in layout(location = {}) ", binding);
		printType(f, s->type, s->name.view());
		f << ";\n";

		m_paramNames[s] = StringBuf(s->name.view());
	}

	if (!m_stage->inputs.empty())
		f << "\n";
}

void ShaderCodePrinter::printStageOutputs(StringBuilder& f)
{
	for (const auto* s : m_stage->outputs)
	{
		short binding = -1;
		m_sharedBindings.m_stageOutputMapping.find(s, binding);

		f.appendf("out layout(location = {}) ", binding);
		printType(f, s->type, s->name.view());
		f << ";\n";

		m_paramNames[s] = StringBuf(s->name.view());
	}

	if (!m_stage->outputs.empty())
		f << "\n";
}

void ShaderCodePrinter::printSharedMemory(StringBuilder& f)
{
	for (const auto* s : m_stage->sharedMemory)
	{
		f << "shared ";
		printType(f, s->type, s->name.view());
		f << ";\n";

		m_paramNames[s] = StringBuf(s->name.view());
	}

	if (!m_stage->sharedMemory.empty())
		f << "\n";
}

void ShaderCodePrinter::printFunctionsForwardDeclarations(StringBuilder& f)
{
	for (const auto* func : m_stage->functionsRefs)
	{
		printHeader(f, func);
		f << ";\n";
	}

	if (!m_stage->functionsRefs.empty())
		f << "\n";
}

void ShaderCodePrinter::printFunctions(StringBuilder& f)
{
	for (const auto* func : m_stage->functions)
	{
		printFunction(f, func);
		f << "\n";
	}
}

void ShaderCodePrinter::printHeader(StringBuilder& f, const shader::StubFunction* func)
{
	printType(f, func->returnType);
	f.appendf(" {}(", func->name);

	bool hasParams = false;
	for (const auto* param : func->parameters)
	{
		if (hasParams)
			f << ", ";

		if (param->reference)
			f << "inout ";

		printType(f, param->type, param->name.view());
		hasParams = true;
	}

	f << ")";
}

void ShaderCodePrinter::printFunction(StringBuilder& f, const shader::StubFunction* func)
{
	printHeader(f, func);
	f << " {\n";
	printStatement(f, func->code, 4);

	if (func == m_stage->entryFunction && m_stage->stage == ShaderStage::Vertex)
	{
		f << "  gl_Position.z = 2.0 * gl_Position.z - gl_Position.w;\n";

		if (HasAttribute(m_stage->entryFunction->attributes, "glflip"_id))
			f << "  gl_Position.y = -gl_Position.y;\n";
	}

	f << "}\n";
}

void ShaderCodePrinter::printVariableDeclaration(StringBuilder& f, const shader::StubOpcodeVariableDeclaration* var)
{
	printType(f, var->var->type, var->var->name.view());

	if (var->init)
	{
		f.append(" = ");
		printExpression(f, var->init);
	}
}
			
void ShaderCodePrinter::printStatement(StringBuilder& f, const shader::StubOpcode* baseOp, int depth)
{
	if (const auto* op = baseOp->asOpcodeScope())
	{
		for (auto* child : op->statements)
			printStatement(f, child, depth);
	}
	else if (const auto* op = baseOp->asOpcodeReturn())
	{
		f.appendPadding(' ', depth).append("return ");

		if (op->value)
			printExpression(f, op->value);

		f.append(";\n");
	}
	else if (const auto* op = baseOp->asOpcodeLoop())
	{
		bool whileLoop = (op->increment == nullptr);

		if (whileLoop && op->init)
			printStatement(f, op->init, depth);

		f.appendPadding(' ', depth);

		if (m_featureMask.test(ShaderFeatureBit::ControlFlowHints))
		{
			if (op->dependencyLength || op->unrollHint)
			{
				f << "[[";
				if (op->unrollHint < 0)
					f << "loop";
				else if (op->unrollHint > 0)
					f << "unroll";

				if (op->dependencyLength && op->unrollHint)
					f << ", ";

				if (op->dependencyLength < 0)
					f << "dependency_infinite";
				else if (op->dependencyLength > 0)
					f.appendf("dependency_length({})", op->dependencyLength);

				f << "]] ";
			}
		}
					
		if (whileLoop)
		{
			f << "while (";
			printExpression(f, op->condition);
			f << ") {\n";
			printStatement(f, op->body, depth + 4);
		}
		else
		{
			f << "for (";
			printExpression(f, op->init);
			f << ";";
			printExpression(f, op->condition);
			f << ";";
			printExpression(f, op->increment);
			f << ") {\n";
			printStatement(f, op->body, depth + 4);
		}

		f.appendPadding(' ', depth).append("}\n");
	}
	else if (const auto* op = baseOp->asOpcodeIfElse())
	{
		const auto count = op->conditions.size();
		for (int i = 0; i < count; ++i)
		{
			f.appendPadding(' ', depth);

			if (m_featureMask.test(ShaderFeatureBit::ControlFlowHints) && i == 0)
			{
				if (op->branchHint < 0)
					f << "[[flatten]] ";
				else if (op->branchHint > 0)
					f << "[[branch]] ";
			}

			if (i == 0)
				f << "if (";
			else
				f << "} else if (";

			printExpression(f, op->conditions[i]);

			f.append(") {\n");

			printStatement(f, op->statements[i], depth + 4);
		}

		if (op->elseStatement)
		{
			f.appendPadding(' ', depth).append("} else {\n");
			printStatement(f, op->elseStatement, depth + 4);
		}

		f.appendPadding(' ', depth).append("}\n");
	}
	else if (const auto* op = baseOp->asOpcodeBreak())
	{
		f.appendPadding(' ', depth).append("break;\n");
	}
	else if (const auto* op = baseOp->asOpcodeContinue())
	{
		f.appendPadding(' ', depth).append("continue;\n");
	}
	else if (const auto* op = baseOp->asOpcodeExit())
	{
		f.appendPadding(' ', depth).append("discard;\n");
	}
	else
	{
		f.appendPadding(' ', depth);
		printExpression(f, baseOp);
		f.append(";\n");
	}
}

void ShaderCodePrinter::printExpression(StringBuilder& f, const shader::StubOpcode* baseOp)
{
	if (const auto* op = baseOp->asOpcodeLoad())
		printExpression(f, op->valueReferece);
	else if (const auto* op = baseOp->asOpcodeStore())
		printExpressionStore(f, op);
	else if (const auto* op = baseOp->asOpcodeConstant())
		printExpressionConstant(f, op);
	else if (const auto* op = baseOp->asOpcodeSwizzle())
		printExpressionReadSwizzle(f, op);
	else if (const auto* op = baseOp->asOpcodeDataRef())
		printExpressionDataRef(f, op);
	else if (const auto* op = baseOp->asOpcodeNativeCall())
		printExpressionNativeCall(f, op);
	else if (const auto* op = baseOp->asOpcodeCall())
		printExpressionCall(f, op);
	else if (const auto* op = baseOp->asOpcodeAccessArray())
		printExpressionAccessArray(f, op);
	else if (const auto* op = baseOp->asOpcodeAccessMember())
		printExpressionAccessMember(f, op);
	else if (const auto* op = baseOp->asOpcodeResourceRef())
		printExpressionResourceRef(f, op);
	else if (const auto* op = baseOp->asOpcodeResourceLoad())
		printExpressionResourceLoad(f, op);
	else if (const auto* op = baseOp->asOpcodeResourceStore())
		printExpressionResourceStore(f, op);
	else if (const auto* op = baseOp->asOpcodeResourceElement())
		printExpressionResourceElement(f, op);
	else if (const auto* op = baseOp->asOpcodeCreateVector())
		printExpressionCreateVector(f, op);
	else if (const auto* op = baseOp->asOpcodeCreateArray())
		printExpressionCreateArray(f, op);
	else if (const auto* op = baseOp->asOpcodeCreateMatrix())
		printExpressionCreateMatrix(f, op);
	else if (const auto* op = baseOp->asOpcodeCast())
		printExpressionCast(f, op);				
	else if (const auto* op = baseOp->asOpcodeVariableDeclaration())
		printVariableDeclaration(f, op);
	else
		ASSERT(!"Invalid opcode");
}

void ShaderCodePrinter::printExpressionStore(StringBuilder& f, const shader::StubOpcodeStore* op)
{
	// TODO: resource store

	printExpression(f, op->lvalue);

	if (!op->mask.empty())
	{
		f << ".";

		for (const auto ch : op->mask.mask)
			if (ch)
				f.appendch(ch);
			else
				break;
	}
					

	f << " = ";

	printExpression(f, op->rvalue);
}

void ShaderCodePrinter::printExpressionNativeCall(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	StringView funcName = op->name.view();;

	// special case ?
	if (const auto* info = m_functions.find(op->name))
	{
		if (info->name)
		{
			if (info->op)
			{
				if (op->arguments.size() == 1)
				{
					f.appendf("({}({}))", info->name, ARG(0));
					return;
				}
				else if (op->arguments.size() == 2)
				{
					f.appendf("({} {} {})", ARG(0), info->name, ARG(1));
					return;
				}
				else
				{
					ASSERT(!"Invalid op");
				}
			}
			else
			{
				funcName = info->name;
			}
		}
		else if (info->func)
		{
			(this->*info->func)(f, op);
			return;
		}
	}

	// print generic
	{
		f.appendf("{}(", funcName);

		bool hasArgs = false;
		for (const auto* arg : op->arguments)
		{
			if (hasArgs) f << ", ";
			hasArgs = true;
			printExpression(f, arg);
		}

		f.append(")");
	}
}

void ShaderCodePrinter::printExpressionCall(StringBuilder& f, const shader::StubOpcodeCall* op)
{
	f.appendf("{}(", op->func->name);

	bool hasArgs = false;
	for (const auto* arg : op->arguments)
	{
		if (hasArgs) f << ", ";
		hasArgs = true;
		printExpression(f, arg);
	}

	f.append(")");
}

static void PrintBuiltIn(StringBuilder& f, shader::ShaderBuiltIn b)
{
	switch (b)
	{
	case shader::ShaderBuiltIn::Position: f << "gl_Position"; return;
	case shader::ShaderBuiltIn::PositionIn: f << "gl_Position"; return;
	case shader::ShaderBuiltIn::PointSize: f << "gl_PointSize"; return;
	case shader::ShaderBuiltIn::PointSizeIn: f << "gl_PointSize"; return;
	case shader::ShaderBuiltIn::ClipDistance: f << "gl_ClipDistance"; return;
	case shader::ShaderBuiltIn::ClipDistanceIn: f << "gl_ClipDistance"; return;
	case shader::ShaderBuiltIn::VertexID: f << "gl_VertexID"; return;
	case shader::ShaderBuiltIn::InstanceID: f << "gl_InstanceID"; return;
	case shader::ShaderBuiltIn::DrawID: f << "gl_DrawID"; return;
	case shader::ShaderBuiltIn::BaseVertex: f << "gl_BaseVertex"; return;
	case shader::ShaderBuiltIn::BaseInstance: f << "gl_BaseInstance"; return;
	case shader::ShaderBuiltIn::PatchVerticesIn: f << "gl_PatchVertices"; return;
	case shader::ShaderBuiltIn::PrimitiveID: f << "gl_PrimitiveID"; return;
	case shader::ShaderBuiltIn::PrimitiveIDIn: f << "gl_PrimitiveIDIn"; return;
	case shader::ShaderBuiltIn::InvocationID: f << "gl_InvocationID"; return;
	case shader::ShaderBuiltIn::TessLevelOuter: f << ""; return;
	case shader::ShaderBuiltIn::TessLevelInner: f << ""; return;
	case shader::ShaderBuiltIn::TessCoord: f << "gl_TessCoord"; return;
	case shader::ShaderBuiltIn::FragCoord: f << "gl_FragCoord"; return;
	case shader::ShaderBuiltIn::FrontFacing: f << "gl_FrontFacing"; return;
	case shader::ShaderBuiltIn::PointCoord: f << "gl_PointCoord"; return;
	case shader::ShaderBuiltIn::SampleID: f << "gl_SampleID"; return;
	case shader::ShaderBuiltIn::SamplePosition: f << "gl_SamplePosition"; return;
	case shader::ShaderBuiltIn::SampleMaskIn: f << "gl_SampleMaskIn[0]"; return;
	case shader::ShaderBuiltIn::SampleMask: f << "gl_SampleMask[0]"; return;
	case shader::ShaderBuiltIn::Target0: f << "gl_FragData[0]"; return;
	case shader::ShaderBuiltIn::Target1: f << "gl_FragData[1]"; return;
	case shader::ShaderBuiltIn::Target2: f << "gl_FragData[2]"; return;
	case shader::ShaderBuiltIn::Target3: f << "gl_FragData[3]"; return;
	case shader::ShaderBuiltIn::Target4: f << "gl_FragData[4]"; return;
	case shader::ShaderBuiltIn::Target5: f << "gl_FragData[5]"; return;
	case shader::ShaderBuiltIn::Target6: f << "gl_FragData[6]"; return;
	case shader::ShaderBuiltIn::Target7: f << "gl_FragData[7]"; return;
	case shader::ShaderBuiltIn::Depth: f << "gl_FragDepth"; return;
	case shader::ShaderBuiltIn::Layer: f << "gl_Layer"; return;
	case shader::ShaderBuiltIn::ViewportIndex: f << "gl_ViewportIndex"; return;
	case shader::ShaderBuiltIn::NumWorkGroups: f << "gl_NumWorkGroups"; return;
	case shader::ShaderBuiltIn::GlobalInvocationID: f << "gl_GlobalInvocationID"; return;
	case shader::ShaderBuiltIn::LocalInvocationID: f << "gl_LocalInvocationID"; return;
	case shader::ShaderBuiltIn::WorkGroupID: f << "gl_WorkGroupID"; return;
	case shader::ShaderBuiltIn::LocalInvocationIndex: f << "gl_LocalInvocationIndex"; return;
	}
}

void ShaderCodePrinter::printExpressionDataRef(StringBuilder& f, const shader::StubOpcodeDataRef* op)
{
	if (const auto* name = m_paramNames.find(op->stub))
		f << name->view();
	else if (const auto* param = op->stub->asScopeLocalVariable())
		f << param->name;
	else if (const auto* param = op->stub->asFunctionParameter())
		f << param->name;
	else if (const auto* param = op->stub->asBuiltInVariable())
		PrintBuiltIn(f, param->builinType);
	else
		ASSERT(!"Unknown param");
}

void ShaderCodePrinter::printExpressionReadSwizzle(StringBuilder& f, const shader::StubOpcodeSwizzle* op)
{
	printExpression(f, op->value);
	if (!op->swizzle.empty())
	{
		f << ".";

		for (const auto ch : op->swizzle.mask)
			if (ch)
				f.appendch(ch);
			else
				break;
	}

}

void ShaderCodePrinter::printExpressionAccessArray(StringBuilder& f, const shader::StubOpcodeAccessArray* op)
{
	if (op->arrayOp->asOpcodeDataRef() && op->arrayOp->asOpcodeDataRef()->stub->asBuiltInVariable())
	{
		// TODO: get the instance name
		f << "gl_in[";
					
		if (op->indexOp)
			printExpression(f, op->indexOp);
		else
			f << op->staticIndex;

		f << "].";
		printExpression(f, op->arrayOp);
		return;
	}
			
	printExpression(f, op->arrayOp);
	if (op->indexOp)
	{
		f << "[";
		printExpression(f, op->indexOp);
		f << "]";
	}
	else
	{
		f.appendf("[{}]", op->staticIndex);
	}
}

void ShaderCodePrinter::printExpressionAccessMember(StringBuilder& f, const shader::StubOpcodeAccessMember* op)
{
	printExpression(f, op->value);
	f.appendf(".{}", op->member->name);
}

void ShaderCodePrinter::printExpressionCreateVector(StringBuilder& f, const shader::StubOpcodeCreateVector* op)
{
	printType(f, op->typeDecl);
	f << "(";

	bool hasParams = false;
	for (const auto* val : op->elements)
	{
		if (hasParams) f << ", ";
		hasParams = true;
		printExpression(f, val);
	}

	f << ")";
}

void ShaderCodePrinter::printExpressionCreateMatrix(StringBuilder& f, const shader::StubOpcodeCreateMatrix* op)
{
	printType(f, op->typeDecl);
	f << "(";

	bool hasParams = false;
	for (const auto* val : op->elements)
	{
		if (hasParams) f << ", ";
		hasParams = true;
		printExpression(f, val);
	}

	f << ")";
}

void ShaderCodePrinter::printExpressionCreateArray(StringBuilder& f, const shader::StubOpcodeCreateArray* op)
{
	printType(f, op->arrayTypeDecl);
	f << "{";

	bool hasParams = false;
	for (const auto* val : op->elements)
	{
		if (hasParams) f << ", ";
		hasParams = true;
		printExpression(f, val);
	}

	f << "}";
}

void ShaderCodePrinter::printExpressionCast(StringBuilder& f, const shader::StubOpcodeCast* op)
{
	if (op->generalType == shader::ScalarType::Boolean)
	{
		if (op->targetType->asScalarTypeDecl())
		{
			f << "(";
			printExpression(f, op->value);
			f << " != 0)";
			return;
		}
	}

	printType(f, op->targetType);
	f << "(";
	printExpression(f, op->value);
	f << ")";
}

void ShaderCodePrinter::printExpressionConstant(StringBuilder& f, const shader::StubOpcodeConstant* op)
{
	const auto* ptr = (const uint32_t*)op->data;
	printConstant(f, op->typeDecl, ptr);
}

void ShaderCodePrinter::printExpressionResourceLoad(StringBuilder& f, const shader::StubOpcodeResourceLoad* op)
{
	// use load function
	f << "imageLoad(  ";
	printExpressionResourceRef(f, op->resourceRef),
	f << ", ";
	printExpression(f, op->address);
	f << ")";

	// do we emit less than 4 components ?
	if (op->numValueComponents == 3)
		f << ".xyz";
	else if (op->numValueComponents == 2)
		f << ".xy";
	else if (op->numValueComponents == 1)
		f << ".x";
}

void ShaderCodePrinter::printExpressionResourceStore(StringBuilder& f, const shader::StubOpcodeResourceStore* op)
{
	f << "imageStore(  ";
	printExpressionResourceRef(f, op->resourceRef),
	f << ",";
	printExpression(f, op->address);
	f << ", ";

	if (op->numValueComponents == 4)
	{
		printExpression(f, op->value);
	}
	else if (op->numValueComponents == 3)
	{
		printExpression(f, op->value);
		f << ".xyzz";
	}
	else if (op->numValueComponents == 2)
	{
		printExpression(f, op->value);
		f << ".xyyy";
	}
	else if (op->numValueComponents == 1)
	{
		f << "(";
		printExpression(f, op->value);
		f << ").xxxx";
	}

	f << ")";
}

void ShaderCodePrinter::printExpressionResourceElement(StringBuilder& f, const shader::StubOpcodeResourceElement* op)
{
	ASSERT(op->resourceRef->type == DeviceObjectViewType::BufferStructured || op->resourceRef->type == DeviceObjectViewType::BufferStructuredWritable);

	printExpressionResourceRef(f, op->resourceRef);
	f << ".data[";
	printExpression(f, op->address);
	f << "]";
}

void ShaderCodePrinter::printExpressionResourceRef(StringBuilder& f, const shader::StubOpcodeResourceRef* op)
{
	const auto* name = m_paramNames.find(op->descriptorEntry);
	f << name->view();
}

//--

void ShaderCodePrinter::printFuncSaturate(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	f.appendf("clamp({}, 0.0, 1.0)", ARG(0));
}

void ShaderCodePrinter::printFuncCompareStd(StringBuilder& f, const shader::StubOpcodeNativeCall* op, const char* scalarOp, const char* vectorFunc)
{
	const auto argScalar = (op->argumentTypes[0]->asScalarTypeDecl() != nullptr);
	if (argScalar)
		f.appendf("({} {} {})", ARG(0), scalarOp, ARG(1));
	else
		f.appendf("{}({}, {})", vectorFunc, ARG(0), ARG(1));
}

static bool IsIntegerScalar(const shader::StubTypeDecl* type)
{
	if (const auto* op = type->asScalarTypeDecl())
		return (op->type == shader::ScalarType::Int) || (op->type == shader::ScalarType::Uint);
	return false;
}

void ShaderCodePrinter::printFuncMod(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	if (IsIntegerScalar(op->argumentTypes[0]))
		f.appendf("({} % {})", ARG(0), ARG(1));
	else
		f.appendf("mod({}, {})", ARG(0), ARG(1));
}

void ShaderCodePrinter::printFuncSelect(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	f.appendf("({} ? {} : {})", ARG(0), ARG(1), ARG(2));
}

void ShaderCodePrinter::printFuncNotEqual(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncCompareStd(f, op, "!=", "notEqual");
}

void ShaderCodePrinter::printFuncEqual(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncCompareStd(f, op, "==", "equal");
}

void ShaderCodePrinter::printFuncGreaterEqual(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncCompareStd(f, op, ">=", "greaterThanEqual");
}

void ShaderCodePrinter::printFuncGreaterThen(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncCompareStd(f, op, ">", "greaterThan");
}

void ShaderCodePrinter::printFuncLessEqual(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncCompareStd(f, op, "<=", "lessThanEqual");
}

void ShaderCodePrinter::printFuncLessThen(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncCompareStd(f, op, "<", "lessThan");
}

//--

void ShaderCodePrinter::printFuncAtomicStd(StringBuilder& f, const shader::StubOpcodeNativeCall* op, StringView stem)
{
	// printable stem
	StringView functionStem = stem;
	if (functionStem == "tomicIncrement")
		functionStem = "tomicAdd";
	else if (functionStem == "tomicDecrement")
		functionStem = "tomicAdd";
	else if (functionStem == "tomicSubtract")
		functionStem = "tomicAdd";

	// allow resources to inject special versions
	bool usedResourceFunction = false;
	const auto* memoryNode = op->arguments[0];
	if (const auto* resourceLoadNode = memoryNode->asOpcodeResourceLoad())
	{
		f.appendf("imageA{}(  ", functionStem);
		printExpressionResourceRef(f, resourceLoadNode->resourceRef);
		f.appendf(", ");
		printExpression(f, resourceLoadNode->address);
	}
	else
	{
		f.appendf("a{}(", functionStem);
		printExpression(f, memoryNode);
	}

	// add rest of arguments
	if (stem == "tomicIncrement")
	{
		f.append(", 1)");
	}
	else if (stem == "tomicDecrement")
	{
		f.append(", -1)");
	}
	else if (stem == "tomicSubtract")
	{
		f.appendf(", -({}))", ARG(1));
	}
	else
	{
		for (uint32_t i = 1; i < op->arguments.size(); ++i)
		{
			f << ", ";
			printExpression(f, op->arguments[i]);
		}
		f << ")";
	}
}

void ShaderCodePrinter::printFuncAtomicIncrement(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncAtomicStd(f, op, "tomicIncrement");
}

void ShaderCodePrinter::printFuncAtomicDecrement(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncAtomicStd(f, op, "tomicDecrement");
}

void ShaderCodePrinter::printFuncAtomicAdd(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncAtomicStd(f, op, "tomicAdd");
}

void ShaderCodePrinter::printFuncAtomicSubtract(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncAtomicStd(f, op, "tomicSubtract");
}

void ShaderCodePrinter::printFuncAtomicMin(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncAtomicStd(f, op, "tomicMin");
}

void ShaderCodePrinter::printFuncAtomicMax(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncAtomicStd(f, op, "tomicMax");
}

void ShaderCodePrinter::printFuncAtomicOr(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncAtomicStd(f, op, "tomicOr");
}

void ShaderCodePrinter::printFuncAtomicAnd(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncAtomicStd(f, op, "tomicAnd");
}

void ShaderCodePrinter::printFuncAtomicXor(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncAtomicStd(f, op, "tomicXor");
}

void ShaderCodePrinter::printFuncAtomicExchange(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncAtomicStd(f, op, "tomicExchange");
}

void ShaderCodePrinter::printFuncAtomicCompareSwap(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	printFuncAtomicStd(f, op, "tomicCompSwap");
}

//--

void ShaderCodePrinter::printFuncTextureGatherOffset(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	f.appendf("textureGatherOffset({}, {}, {}, 0)", ARG(0), ARG(1), ARG(2));
}

void ShaderCodePrinter::printFuncTextureGather(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	f.appendf("textureGather({}, {}, 0)", ARG(0), ARG(1));
}

static uint8_t CalcComponentCount(const shader::StubTypeDecl* type)
{
	if (const auto* op = type->asScalarTypeDecl())
		return 1;
	else if (const auto* op = type->asVectorTypeDecl())
		return op->componentCount;

	ASSERT(!"Invalid type");
	return 0;
}

void ShaderCodePrinter::printFuncTextureDepthCompare(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	const auto uvCompCount = CalcComponentCount(op->argumentTypes[1]);
	DEBUG_CHECK(uvCompCount >= 1 && uvCompCount <= 3);
	f.appendf("textureLod({}, vec{}({}, {}), 0.0)", ARG(0), uvCompCount + 1, ARG(1), ARG(2));
}

void ShaderCodePrinter::printFuncTextureDepthCompareOffset(StringBuilder& f, const shader::StubOpcodeNativeCall* op)
{
	const auto uvCompCount = CalcComponentCount(op->argumentTypes[1]);
	DEBUG_CHECK(uvCompCount >= 1 && uvCompCount <= 3);
	f.appendf("textureLod({}, vec{}({}, {}), 0.0, {})", ARG(0), uvCompCount + 1, ARG(1), ARG(2), ARG(3));
}

//--
	
END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
