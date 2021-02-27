/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader\stubs #]
***/

#include "build.h"
#include "shaderStubs.h"

#include "core/object/include/stubLoader.h"
#include "core/object/include/stub.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::shader)

//--

StubDebugPrinter::StubDebugPrinter(IFormatStream& f)
	: m_printer(f)
{
	m_stubs.reserve(1024);
}

void StubDebugPrinter::enableOutput()
{
	m_outputEnabled = true;
}

int StubDebugPrinter::index(const Stub* stub)
{
	if (!stub)
		return 0;

	int index = 0;
	if (m_stubs.find(stub, index))
		return index;

	index = m_stubs.size() + 1;
	m_stubs[stub] = index;
	return index;
}

void StubDebugPrinter::printChild(const char* name, const Stub* s)
{
	int id = index(s);

	if (m_outputEnabled)
	{
		if (m_pendingLineEnd)
		{
			m_printer.append("\n");
			m_pendingLineEnd = false;
		}

		m_printer.appendPadding(' ', m_depth * 4);
		m_printer.appendf("{}: ", name);

		if (s)
		{
			m_printer.appendf("#({}_{}) ", s->debugName(), id);
			m_depth += 1;
			m_pendingLineEnd = true;
			s->dump(*this); // may recurse
			m_depth -= 1;
		}
		else
		{
			m_printer.appendf("null\n");
		}
	}
	else
	{
		if (s)
			s->dump(*this);
	}
}

void StubDebugPrinter::printChildRefArray(const char* name, const Stub* const* ptr, uint32_t count)
{
	if (m_outputEnabled)
	{
		if (m_pendingLineEnd)
		{
			m_printer.append("\n");
			m_pendingLineEnd = false;
		}

		if (count)
		{
			m_printer.appendPadding(' ', m_depth * 4);
			m_printer.appendf("{}: {} references(s)\n", name, count);

			m_depth += 1;

			for (uint32_t i = 0; i < count; ++i)
			{
				if (ptr[i])
				{
					m_printer.appendPadding(' ', m_depth * 4);
					m_printer.appendf("{}[{}]: ", name, i);
					m_printer.appendf("#({}_{})\n", ptr[i]->debugName(), index(ptr[i]));
				}
				else
				{
					//m_printer.appendf("null\n");
				}
			}

			m_depth -= 1;
		}
		else
		{
			//m_printer.appendPadding(' ', m_depth * 4);
			//m_printer.appendf("{}: empty", name);
		}
	}
	else
	{
		// not indexed			
	}
}

void StubDebugPrinter::printChildArray(const char* name, const Stub* const* ptr, uint32_t count)
{
	if (m_outputEnabled)
	{
		if (m_pendingLineEnd)
		{
			m_printer.append("\n");
			m_pendingLineEnd = false;
		}

		if (count)
		{
			m_printer.appendPadding(' ', m_depth * 4);
			m_printer.appendf("{}: {} element(s)\n", name, count);

			m_depth += 1;

			for (uint32_t i = 0; i < count; ++i)
			{
				if (m_pendingLineEnd)
				{
					m_printer.append("\n");
					m_pendingLineEnd = false;
				}

				if (ptr[i])
				{
					m_printer.appendPadding(' ', m_depth * 4);
					m_printer.appendf("{}[{}]: ", name, i);
					m_printer.appendf("#({}_{}) ", ptr[i]->debugName(), index(ptr[i]));

					m_depth += 1;
					m_pendingLineEnd = true;
					ptr[i]->dump(*this); // may recurse
					m_depth -= 1;
				}
				else
				{
					//m_printer.appendf("null\n");
				}
			}

			m_depth -= 1;
		}
		else
		{
			//m_printer.appendPadding(' ', m_depth * 4);
			//m_printer.appendf("{}: empty", name);
		}
	}
	else
	{
		for (uint32_t i = 0; i < count; ++i)
		{
			if (ptr[i])
			{
				index(ptr[i]);
				ptr[i]->dump(*this);
			}
		}
	}
}

template< typename T >
INLINE void printChildArray(const char* name, const StubPseudoArray<T>& arr)
{
	printChildArray(name, (const Stub* const*)arr.elems, arr.size());
}

StubDebugPrinter& StubDebugPrinter::printRef(const char* name, const Stub* s)
{
	if (m_outputEnabled)
	{
		m_printer.appendf(" {}=", name);

		if (s)
		{
			int index = 0;
			if (m_stubs.find(s, index))
				m_printer.appendf("#({}_{})", s->debugName(), index);
			else
				m_printer.appendf("#({}_???)", s->debugName());
		}
		else
		{
			m_printer << "null";
		}
	}

	return *this;
}

IFormatStream& StubDebugPrinter::append(const char* str, uint32_t len /*= INDEX_MAX*/)
{
	if (m_outputEnabled)
		m_printer.append(str, len);
	return *this;
}

//--

ComponentSwizzle::ComponentSwizzle()
{
	memzero(mask, sizeof(mask));
}

ComponentSwizzle::ComponentSwizzle(StringView str)
{
	memzero(mask, sizeof(mask));

	const auto len = str.length();
	DEBUG_CHECK_RETURN_EX(len >= 1 && len <= MAX_COMPONENTS, TempString("Invalid swizzle '{}'", str));

	for (uint32_t i = 0; i < len; ++i)
	{
		const char ch = str.data()[i];
		DEBUG_CHECK_RETURN_EX(ch == 'x' || ch == 'y' || ch == 'z' || ch == 'w' || ch == '1' || ch == '0', TempString("Invalid swizzle '{}'", str));
		mask[i] = ch;

		if (ch == 0)
			break;
	}			
}

static bool ComponentIndexFromMask(char ch, uint8_t& outIndex)
{
	switch (ch)
	{
	case 'r':
	case 'x':
		outIndex = 0;
		return true;

	case 'g':
	case 'y':
		outIndex = 1;
		return true;

	case 'b':
	case 'z':
		outIndex = 2;
		return true;

	case 'a':
	case 'w':
		outIndex = 3;
		return true;
	}

	return false;
}


uint8_t ComponentSwizzle::writeMask() const
{
	uint8_t mask = 0;
	uint8_t prevValue = 0;

	for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
	{
		uint8_t componentIndex = 0;
		if (!ComponentIndexFromMask(this->mask[i], componentIndex))
			return 0;

		uint8_t bitValue = 1 << componentIndex;
		if (mask & bitValue)
			return 0;
		if (bitValue < prevValue)
			return 0;
		mask |= bitValue;
		prevValue = bitValue;
	}

	return mask;
}
		
uint8_t ComponentSwizzle::outputComponents(uint8_t* outList /*= nullptr*/) const
{
	uint8_t ret = 0;

	for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
	{
		if (mask[i] == 0)
			break;

		uint8_t componentIndex = 0;
		if (!ComponentIndexFromMask(this->mask[i], componentIndex))
			return 0;

		if (outList)
			outList[ret] = componentIndex;

		ret += 1;
	}

	return ret;
}

uint8_t ComponentSwizzle::inputComponents() const
{
	uint8_t ret = 0;
	for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
	{
		uint8_t componentIndex = 0;
		if (ComponentIndexFromMask(this->mask[i], componentIndex))
			ret = std::max<uint8_t>(ret, componentIndex + 1);
	}

	return ret;
}

void ComponentSwizzle::print(IFormatStream& f) const
{
	for (uint32_t i = 0; i < MAX_COMPONENTS; ++i)
	{
		f.appendch(mask[i] ? mask[i] : '-');
	}
}

void ComponentSwizzle::write(IStubWriter& f) const
{
	const auto* ptr = (const uint32_t*)&mask;
	f.writeUint32(*ptr);
}

void ComponentSwizzle::read(IStubReader& f)
{
	auto* ptr = (uint32_t*)&mask;
	*ptr = f.readUint32();
}
		
//--

void StubLocation::write(IStubWriter& f) const
{
	f.writeRef(file);
	f.writeCompressedInt(line);
}

void StubLocation::read(IStubReader& f)
{
	f.readRef(file);
	line = f.readCompressedInt();
}

void StubLocation::print(IFormatStream& f) const
{}

//--

void Stub::postLoad()
{}

//--

class RenderStubFactory : public StubFactory
{
public:
	RenderStubFactory()
	{
		#define DECLARE_GPU_SHADER_STUB(x) registerTypeNoDestructor<Stub##x>();
		#include "shaderStubsCodes.inl"
	}
};

const StubFactory& Stub::Factory()
{
	static RenderStubFactory TheFactory;
	return TheFactory;
}

//--

void StubFile::write(IStubWriter& f) const
{
	f.writeString(depotPath);
	f.writeUint64(contentCrc);
	f.writeUint64(contentTimestamp);
}

void StubFile::read(IStubReader& f)
{
	depotPath = f.readString();
	contentCrc = f.readUint64();
	contentTimestamp = f.readUint64();
}

void StubFile::dump(StubDebugPrinter& f) const
{
	f.appendf("path='{}', crc={}, timestamp={}", depotPath, contentCrc, contentTimestamp);
}

//--

void StubAttribute::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeString(value);
}

void StubAttribute::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	value = f.readString();
}

void StubAttribute::dump(StubDebugPrinter& f) const
{
	f.appendf("{}={}", name, value);
}

//--

StubStage::StubStage()
{}

void StubStage::write(IStubWriter& f) const
{
	f.writeEnum(stage);
	f.writeUint32(featureMask.rawValue());
	f.writeArray(types);
	f.writeArray(structures);
	f.writeArray(inputs);
	f.writeArray(outputs);
	f.writeArray(sharedMemory);
	f.writeArray(descriptorMembers);
	f.writeArray(builtins);
	f.writeArray(vertexStreams);
	f.writeArray(globalConstants);
	f.writeArray(samplers);
	f.writeArray(functionsRefs);
	f.writeArray(functions);
	f.writeRef(entryFunction);
}

void StubStage::read(IStubReader& f)
{
	f.readEnum(stage);
	featureMask = ShaderFeatureMask(f.readUint32());
	f.readArray(types);
	f.readArray(structures);
	f.readArray(inputs);
	f.readArray(outputs);
	f.readArray(sharedMemory);
	f.readArray(descriptorMembers);
	f.readArray(builtins);
	f.readArray(vertexStreams);
	f.readArray(globalConstants);
	f.readArray(samplers);
	f.readArray(functionsRefs);
	f.readArray(functions);
	f.readRef(entryFunction);
}

void StubStage::dump(StubDebugPrinter& f) const
{
	f.appendf("stage={}", stage);
	f.printRef("entry", entryFunction);

	f.printChildRefArray("TypeDeclRef", types);
	f.printChildRefArray("StructureRef", structures);
	f.printChildRefArray("DescriptorMemberRef", descriptorMembers);
	f.printChildRefArray("StaticSamplerRef", samplers);
	f.printChildRefArray("VertexStream", vertexStreams);
	f.printChildArray("StageInput", inputs);
	f.printChildArray("StageOutput", outputs);
	f.printChildArray("SharedMemory", sharedMemory);
	f.printChildArray("GlobalConstants", globalConstants);
	f.printChildArray("BuiltIn", builtins);
	f.printChildRefArray("FunctionRefs", functionsRefs);
	f.printChildArray("Function", functions);
}

//--

StubProgram::StubProgram()
{
}

void StubProgram::write(IStubWriter& f) const
{
	f.writeString(depotPath);
	f.writeString(options);
	f.writeUint32(featureMask.rawValue());
	f.writeArray(files);
	f.writeArray(types);
	f.writeArray(structures);
	f.writeArray(descriptors);
	f.writeArray(samplers);
	f.writeArray(vertexStreams);
	f.writeArray(stages);
	f.writeRef(renderStates);
}

void StubProgram::read(IStubReader& f)
{
	depotPath = f.readString();
	options = f.readString();
	featureMask = ShaderFeatureMask(f.readUint32());
	f.readArray(files);
	f.readArray(types);
	f.readArray(structures);
	f.readArray(descriptors);
	f.readArray(samplers);
	f.readArray(vertexStreams);
	f.readArray(stages);
	f.readRef(renderStates);
}

void StubProgram::dump(StubDebugPrinter& f) const
{
	f.appendf("file='{}' options='{}'", depotPath, options);
	f.printChildArray("File", files);
	f.printChildArray("TypeDecl", types);
	f.printChildArray("Structure", structures);
	f.printChildArray("Descriptor", descriptors);
	f.printChildArray("Sampler", samplers);
	f.printChildArray("VertexStream", vertexStreams);
	f.printChildArray("Stage", stages);
	f.printChild("RenderStates", renderStates);
}

//--

RTTI_BEGIN_TYPE_ENUM(ScalarType);
	RTTI_ENUM_OPTION(Void);
	RTTI_ENUM_OPTION(Half);
	RTTI_ENUM_OPTION(Float);
	RTTI_ENUM_OPTION(Double);
	RTTI_ENUM_OPTION(Int);
	RTTI_ENUM_OPTION(Uint);
	RTTI_ENUM_OPTION(Int64);
	RTTI_ENUM_OPTION(Uint64);
	RTTI_ENUM_OPTION(Boolean);
RTTI_END_TYPE();

void StubScalarTypeDecl::print(IFormatStream& f) const
{
	switch (type)
	{
		case ScalarType::Void: f << "void"; break;
		case ScalarType::Boolean: f << "bool"; break;
		case ScalarType::Half: f << "half"; break;
		case ScalarType::Double: f << "double"; break;
		case ScalarType::Float: f << "float"; break;
		case ScalarType::Int: f << "int"; break;
		case ScalarType::Uint: f << "uint"; break;
		case ScalarType::Int64: f << "int64"; break;
		case ScalarType::Uint64: f << "uint64"; break;
		default:
			ASSERT(!"Invalid scalar type");
	}
}

void StubScalarTypeDecl::write(IStubWriter& f) const
{
	f.writeEnum(type);
}

void StubScalarTypeDecl::read(IStubReader& f)
{
	f.readEnum(type);
}

void StubScalarTypeDecl::dump(StubDebugPrinter& f) const
{
	f.appendf("type=");
	print(f);
}

//--

void StubVectorTypeDecl::print(IFormatStream& f) const
{
	ASSERT_EX(componentCount >= 2 && componentCount <= 4, "Invalid component count");

	const char* prefix = "";
	switch (type)
	{
	case ScalarType::Boolean: prefix = "b"; break;
	case ScalarType::Int: prefix = "i"; break;
	case ScalarType::Uint: prefix = "u"; break;
	case ScalarType::Double: prefix = "d"; break;
	case ScalarType::Half: prefix = "h"; break;
	case ScalarType::Float: prefix = ""; break;
	default:
		ASSERT(!"Invalid scalar type for vector");
	}

	f.appendf("{}vec{}", prefix, componentCount); // vec4, uvec2, bvec3, etc			
}

void StubVectorTypeDecl::write(IStubWriter& f) const
{
	f.writeEnum(type);
	f.writeUint8(componentCount);
}

void StubVectorTypeDecl::read(IStubReader& f)
{
	f.readEnum(type);
	componentCount = f.readUint8();
}

void StubVectorTypeDecl::dump(StubDebugPrinter& f) const
{
	f.appendf("type=");
	print(f);
}

//--

void StubMatrixTypeDecl::print(IFormatStream& f) const
{
	bool hasRows = componentCount >= 2 && componentCount <= 4;
	bool hasColumns = rowCount >= 2 && rowCount <= 4;
	ASSERT_EX(hasRows || hasColumns, "Invalid component count");

	const char* prefix = "";
	switch (type)
	{
	case ScalarType::Double: prefix = "d"; break;
	case ScalarType::Float: prefix = ""; break;
	default:
		ASSERT(!"Invalid scalar type for matrix");
	}

	if (componentCount == rowCount)
		f.appendf("{}mat{}", prefix, componentCount); // vec4, uvec2, bvec3, etc			
	else
		f.appendf("{}mat{}x{}", prefix, componentCount, rowCount); // vec4, uvec2, bvec3, etc			
}

void StubMatrixTypeDecl::write(IStubWriter& f) const
{
	f.writeEnum(type);
	f.writeUint8(componentCount);
	f.writeUint8(rowCount);
}

void StubMatrixTypeDecl::read(IStubReader& f)
{
	f.readEnum(type);
	componentCount = f.readUint8();
	rowCount = f.readUint8();
}

void StubMatrixTypeDecl::dump(StubDebugPrinter& f) const
{
	f.appendf("type=");
	print(f);
}

//--

void StubArrayTypeDecl::dump(StubDebugPrinter& f) const
{
	f.append("type=");
	print(f);

	f.printRef("innerType", innerType);
}

void StubArrayTypeDecl::print(IFormatStream& f) const
{
	ASSERT_EX(innerType, "Invalid array type");
	innerType->print(f);

	if (count > 0)
		f.appendf("[{}]", count);
	else
		f.append("[]");
}

void StubArrayTypeDecl::write(IStubWriter& f) const
{
	f.writeUint32(count);
	f.writeRef(innerType);
}

void StubArrayTypeDecl::read(IStubReader& f)
{
	count = f.readUint32();
	f.readRef(innerType);
}

//--

void StubStructTypeDecl::print(IFormatStream& f) const
{
	f << structType->name;
}

void StubStructTypeDecl::write(IStubWriter& f) const
{
	f.writeRef(structType);
}

void StubStructTypeDecl::read(IStubReader& f)
{
	f.readRef(structType);
}

void StubStructTypeDecl::dump(StubDebugPrinter& f) const
{
	f.append("type=");
	print(f);

	f.printRef("struct", structType);
}

//--

void StubStructMember::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeRef(owner);
	f.writeCompressedInt(index);
	f.writeCompressedInt(linearAlignment);
	f.writeCompressedInt(linearOffset);
	f.writeCompressedInt(linearSize);
	f.writeCompressedInt(linearArrayCount);
	f.writeCompressedInt(linearArrayStride);
	f.writeRef(type);
	f.writeArray(attributes);
}

void StubStructMember::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	f.readRef(owner);
	index = f.readCompressedInt();
	linearAlignment = f.readCompressedInt();
	linearOffset = f.readCompressedInt();
	linearSize = f.readCompressedInt();
	linearArrayCount = f.readCompressedInt();
	linearArrayStride = f.readCompressedInt();
	f.readRef(type);
	f.readArray(attributes);
}

void StubStructMember::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} index={}, offset={}, align={}, size={}, arrayCount={}, arrayStride={}",
		name, index, linearOffset, linearAlignment, linearSize, linearArrayCount, linearArrayStride);

	f.append(" type=");
	type->print(f);
	f.printRef("typeRef", type);

	f.printChildArray("Attribute", attributes);
}

//--

void StubSamplerState::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeCompressedInt(index);
	f.writeArray(attributes);

	f.writeEnum(state.magFilter);
	f.writeEnum(state.minFilter);
	f.writeEnum(state.mipmapMode);
	f.writeEnum(state.addresModeU);
	f.writeEnum(state.addresModeV);
	f.writeEnum(state.addresModeW);
	f.writeBool(state.compareEnabled);
	f.writeEnum(state.compareOp);
	f.writeEnum(state.borderColor);
	f.writeUint8(state.maxAnisotropy);
	f.writeFloat(state.mipLodBias);
	f.writeFloat(state.minLod);
	f.writeFloat(state.maxLod);
}

void StubSamplerState::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	index = f.readCompressedInt();
	f.readArray(attributes);

	f.readEnum(state.magFilter);
	f.readEnum(state.minFilter);
	f.readEnum(state.mipmapMode);
	f.readEnum(state.addresModeU);
	f.readEnum(state.addresModeV);
	f.readEnum(state.addresModeW);
	state.compareEnabled = f.readBool();
	f.readEnum(state.compareOp);
	f.readEnum(state.borderColor);
	state.maxAnisotropy = f.readUint8();
	state.mipLodBias = f.readFloat();
	state.minLod = f.readFloat();
	state.maxLod = f.readFloat();
}

void StubSamplerState::dump(StubDebugPrinter& f) const
{
	f.appendf("magFilter={} ", state.magFilter);
	f.appendf("minFilter={} ", state.minFilter);
	f.appendf("mipmapMode={} ", state.mipmapMode);
	f.appendf("addresModeU={} ", state.addresModeU);
	f.appendf("addresModeV={} ", state.addresModeV);
	f.appendf("addresModeW={} ", state.addresModeW);
	f.appendf("compareEnabled={} ", state.compareEnabled);
	f.appendf("compareOp={} ", state.compareOp);
	f.appendf("borderColor={} ", state.borderColor);
	f.appendf("maxAnisotropy={} ", state.maxAnisotropy);
	f.appendf("mipLodBias={} ", state.mipLodBias);
	f.appendf("minLod={} ", state.minLod);
	f.appendf("maxLod={} ", state.maxLod);
}

//--

void StubRenderStates::write(IStubWriter& f) const
{
	f.writeData(&states, sizeof(states));
}

void StubRenderStates::read(IStubReader& f)
{
	const auto* sourceData = f.readData(sizeof(states));
	memcpy(&states, sourceData, sizeof(states));
}

void StubRenderStates::dump(StubDebugPrinter& f) const
{
	f.append("\n");
	states.print(f);
}

//--

void StubStruct::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeCompressedInt(size);
	f.writeCompressedInt(alignment);
	f.writeArray(members);
	f.writeArray(attributes);
}

void StubStruct::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	size = f.readCompressedInt();
	alignment = f.readCompressedInt();
	f.readArray(members);
	f.readArray(attributes);
}

void StubStruct::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} size={} alignment={}", name, size, alignment);
	f.printChildArray("Attribute", attributes);
	f.printChildArray("StructureMember", members);
}

//--

void StubDescriptorMember::write(IStubWriter& f) const
{
	location.write(f);
	f.writeRef(descriptor);
	f.writeCompressedInt(index);
	f.writeName(name);
	f.writeArray(attributes);
}

void StubDescriptorMember::read(IStubReader& f)
{
	location.read(f);
	f.readRef(descriptor);
	index = f.readCompressedInt();
	name = f.readName();
	f.readArray(attributes);
}

//--

void StubDescriptorMemberConstantBufferElement::write(IStubWriter& f) const
{
	location.write(f);
	f.writeRef(constantBuffer);
	f.writeName(name);
	f.writeCompressedInt(linearAlignment);
	f.writeCompressedInt(linearOffset);
	f.writeCompressedInt(linearSize);
	f.writeCompressedInt(linearArrayCount);
	f.writeCompressedInt(linearArrayStride);
	f.writeRef(type);
	f.writeArray(attributes);
}

void StubDescriptorMemberConstantBufferElement::read(IStubReader& f)
{
	location.read(f);
	f.readRef(constantBuffer);
	name = f.readName();
	linearAlignment = f.readCompressedInt();
	linearOffset = f.readCompressedInt();
	linearSize = f.readCompressedInt();
	linearArrayCount = f.readCompressedInt();
	linearArrayStride = f.readCompressedInt();
	f.readRef(type);
	f.readArray(attributes);
}

void StubDescriptorMemberConstantBufferElement::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} offset={}, align={}, size={}, arrayCount={}, arrayStride={}",
		name, linearOffset, linearAlignment, linearSize, linearArrayCount, linearArrayStride);

	f.append(" type=");
	type->print(f);
	f.printRef("typeRef", type);
	f.printRef("parent", constantBuffer);

	f.printChildArray("Attribute", attributes);
}

//--

void StubDescriptorMemberConstantBuffer::write(IStubWriter& f) const
{
	StubDescriptorMember::write(f);
	f.writeCompressedInt(size);
	f.writeArray(elements);
}

void StubDescriptorMemberConstantBuffer::read(IStubReader& f)
{
	StubDescriptorMember::read(f);
	size = f.readCompressedInt();
	f.readArray(elements);
}

void StubDescriptorMemberConstantBuffer::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} index={} size={}", name, index, size);
	f.printChildArray("Attribute", attributes);
	f.printChildArray("Constant", elements);
}

//--

void StubDescriptorMemberFormatBuffer::write(IStubWriter& f) const
{
	StubDescriptorMember::write(f);
	f.writeEnum(format);
	f.writeBool(writable);
}

void StubDescriptorMemberFormatBuffer::read(IStubReader& f)
{
	StubDescriptorMember::read(f);
	f.readEnum(format);
	writable = f.readBool();
}

void StubDescriptorMemberFormatBuffer::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} index={} format={} writable={}", name, index, format, writable);
}

//--

void StubDescriptorMemberStructuredBuffer::write(IStubWriter& f) const
{
	StubDescriptorMember::write(f);
	f.writeRef(layout);
	f.writeCompressedInt(stride);
	f.writeBool(writable);
}

void StubDescriptorMemberStructuredBuffer::read(IStubReader& f)
{
	StubDescriptorMember::read(f);
	f.readRef(layout);
	stride = f.readCompressedInt();
	writable = f.readBool();
}

void StubDescriptorMemberStructuredBuffer::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} index={} stride={} writable={}", name, index, stride, writable);

	f.appendf(" layout={}", layout->name);
	f.printRef("layoutRef", layout);
}

//--

void StubDescriptorMemberSampledImage::write(IStubWriter& f) const
{
	StubDescriptorMember::write(f);
	f.writeEnum(viewType);
	f.writeEnum(scalarType);
	f.writeBool(depth);
	f.writeBool(multisampled);
	f.writeRef(staticState);
	f.writeRef(dynamicSamplerDescriptorEntry);
}

void StubDescriptorMemberSampledImage::read(IStubReader& f)
{
	StubDescriptorMember::read(f);
	f.readEnum(viewType);
	f.readEnum(scalarType);
	depth = f.readBool();
	multisampled = f.readBool();
	f.readRef(staticState);
	f.readRef(dynamicSamplerDescriptorEntry);
}

void StubDescriptorMemberSampledImage::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} index={} view={} depth={} ms={}", name, index, viewType, depth, multisampled);

	if (staticState)
		f.printRef("staticSamplerRef", staticState);

	if (dynamicSamplerDescriptorEntry)
		f.printRef("dynamicSamplerRef", dynamicSamplerDescriptorEntry);
}

//--

void StubDescriptorMemberSampledImageTable::write(IStubWriter& f) const
{
	StubDescriptorMember::write(f);
	f.writeEnum(viewType);
	f.writeBool(depth);
	f.writeRef(staticState);
	f.writeRef(dynamicSamplerDescriptorEntry);
}

void StubDescriptorMemberSampledImageTable::read(IStubReader& f)
{
	StubDescriptorMember::read(f);
	f.readEnum(viewType);
	depth = f.readBool();
	f.readRef(staticState);
	f.readRef(dynamicSamplerDescriptorEntry);
}
		
void StubDescriptorMemberSampledImageTable::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} index={} view={} depth={}", name, index, viewType, depth);

	if (staticState)
		f.printRef("staticSamplerRef", staticState);

	if (dynamicSamplerDescriptorEntry)
		f.printRef("dynamicSamplerRef", dynamicSamplerDescriptorEntry);
}

//--

void StubDescriptorMemberImage::write(IStubWriter& f) const
{
	StubDescriptorMember::write(f);
	f.writeEnum(viewType);
	f.writeEnum(format);
	f.writeBool(writable);
}

void StubDescriptorMemberImage::read(IStubReader& f)
{
	StubDescriptorMember::read(f);
	f.readEnum(viewType);
	f.readEnum(format);
	writable = f.readBool();
}

void StubDescriptorMemberImage::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} index={} view={} format={}", name, index, viewType, format);
}

//--

void StubDescriptorMemberSampler::write(IStubWriter& f) const
{
	StubDescriptorMember::write(f);
}

void StubDescriptorMemberSampler::read(IStubReader& f)
{
	StubDescriptorMember::read(f);
}

void StubDescriptorMemberSampler::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} index={}", name, index);
}

//--

void StubDescriptor::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeArray(attributes);
	f.writeArray(members);
}

void StubDescriptor::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	f.readArray(attributes);
	f.readArray(members);
}

void StubDescriptor::dump(StubDebugPrinter& f) const
{
	f.appendf("name={}", name);
	f.printChildArray("Attribute", attributes);
	f.printChildArray("DescriptorMember", members);
}

//--

void StubStageOutput::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeName(bindingName);
	f.writeRef(type);
	f.writeRef(nextStageInput);
	f.writeArray(attributes);
}

void StubStageOutput::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	bindingName = f.readName();
	f.readRef(type);
	f.readRef(nextStageInput);
	f.readArray(attributes);
}

void StubStageOutput::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} binding={}", name, bindingName);

	f.append(" type=");
	type->print(f);
	f.printRef("typeRef", type);
	f.printRef("nextStageInput", nextStageInput);

	f.printChildArray("Attribute", attributes);
}

//--

void StubStageInput::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeName(bindingName);
	f.writeRef(type);
	f.writeRef(prevStageOutput);
	f.writeArray(attributes);
}

void StubStageInput::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	bindingName = f.readName();
	f.readRef(type);
	f.readRef(prevStageOutput);
	f.readArray(attributes);
}

void StubStageInput::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} binding={}", name, bindingName);

	f.append(" type=");
	type->print(f);
	f.printRef("typeRef", type);
	f.printRef("prevStageOutput", prevStageOutput);

	f.printChildArray("Attribute", attributes);
}

//--

void StubGlobalConstant::write(IStubWriter& f) const
{
	f.writeCompressedInt(index);
	f.writeRef(typeDecl);
	f.writeEnum(dataType);
	f.writeCompressedInt(dataSize);
	f.writeData(data, dataSize);
}

void StubGlobalConstant::read(IStubReader& f)
{
	index = f.readCompressedInt();
	f.readRef(typeDecl);
	f.readEnum(dataType);
	dataSize = f.readCompressedInt();
	data = f.readData(dataSize);
}

template< typename T >
static void PrintArray(IFormatStream& f, const void* ptr, uint32_t dataSize)
{
	const auto count = dataSize / sizeof(T);
	const auto* readPtr = (const T*)ptr;
	f.appendf(" count={}", count);
	for (uint32_t i = 0; i < count; ++i)
		f.appendf(" [{}]={}", i, readPtr[i]);
}

void StubGlobalConstant::dump(StubDebugPrinter& f) const
{
	f.appendf("index={} size={} scalar={}", index, dataSize, dataType);

	switch (dataType)
	{
	case ScalarType::Float:
	case ScalarType::Half:
		PrintArray<float>(f, data, dataSize);
		break;

	case ScalarType::Boolean:
	case ScalarType::Uint:
		PrintArray<uint32_t>(f, data, dataSize);
		break;

	case ScalarType::Int:
		PrintArray<int>(f, data, dataSize);
		break;

	case ScalarType::Int64:
		PrintArray<int64_t>(f, data, dataSize);
		break;

	case ScalarType::Uint64:
		PrintArray<uint64_t>(f, data, dataSize);
		break;
	}
}

//--

void StubSharedMemory::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeRef(type);
	f.writeArray(attributes);
}

void StubSharedMemory::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	f.readRef(type);
	f.readArray(attributes);
}

void StubSharedMemory::dump(StubDebugPrinter& f) const
{
	f.appendf("name={}", name);

	f.append(" type=");
	type->print(f);
	f.printRef("typeRef", type);

	f.printChildArray("Attribute", attributes);
}

//--

RTTI_BEGIN_TYPE_ENUM(ShaderBuiltIn)
	RTTI_ENUM_OPTION(Position);
	RTTI_ENUM_OPTION(PositionIn);
	RTTI_ENUM_OPTION(PointSize);
	RTTI_ENUM_OPTION(PointSizeIn);
	RTTI_ENUM_OPTION(ClipDistance);
	RTTI_ENUM_OPTION(VertexID);
	RTTI_ENUM_OPTION(InstanceID);
	RTTI_ENUM_OPTION(DrawID);
	RTTI_ENUM_OPTION(BaseVertex);
	RTTI_ENUM_OPTION(BaseInstance);
	RTTI_ENUM_OPTION(PatchVerticesIn);
	RTTI_ENUM_OPTION(PrimitiveID);
	RTTI_ENUM_OPTION(InvocationID);
	RTTI_ENUM_OPTION(TessLevelOuter);
	RTTI_ENUM_OPTION(TessLevelInner);
	RTTI_ENUM_OPTION(TessCoord);
	RTTI_ENUM_OPTION(FragCoord);
	RTTI_ENUM_OPTION(FrontFacing);
	RTTI_ENUM_OPTION(PointCoord);
	RTTI_ENUM_OPTION(SampleID);
	RTTI_ENUM_OPTION(SamplePosition);
	RTTI_ENUM_OPTION(SampleMaskIn);
	RTTI_ENUM_OPTION(Target0);
	RTTI_ENUM_OPTION(Target1);
	RTTI_ENUM_OPTION(Target2);
	RTTI_ENUM_OPTION(Target3);
	RTTI_ENUM_OPTION(Target4);
	RTTI_ENUM_OPTION(Target5);
	RTTI_ENUM_OPTION(Target6);
	RTTI_ENUM_OPTION(Target7);
	RTTI_ENUM_OPTION(NumWorkGroups);
	RTTI_ENUM_OPTION(GlobalInvocationID);
	RTTI_ENUM_OPTION(LocalInvocationID);
	RTTI_ENUM_OPTION(WorkGroupID);
	RTTI_ENUM_OPTION(LocalInvocationIndex);
RTTI_END_TYPE();

void StubBuiltInVariable::write(IStubWriter& f) const
{
	f.writeEnum(builinType);
	f.writeRef(dataType);
}

void StubBuiltInVariable::read(IStubReader& f)
{
	f.readEnum(builinType);
	f.readRef(dataType);
}

void StubBuiltInVariable::dump(StubDebugPrinter& f) const
{
	f.appendf("builtin={}", builinType);

	f.append(" type=");
	dataType->print(f);
	f.printRef("typeRef", dataType);
}

//--

void StubVertexInputElement::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeRef(stream);
	f.writeUint8(elementIndex);
	f.writeCompressedInt(elementOffset);
	f.writeCompressedInt(elementSize);
	f.writeEnum(elementFormat);
	f.writeRef(type);
}

void StubVertexInputElement::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	f.readRef(stream);
	elementIndex = f.readUint8();
	elementOffset = f.readCompressedInt();
	elementSize = f.readCompressedInt();
	f.readEnum(elementFormat);
	f.readRef(type);
}

void StubVertexInputElement::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} index={} offset={} size={} format={}", name, elementIndex, elementOffset, elementSize, elementFormat);

	f.append(" type=");
	type->print(f);
	f.printRef("typeRef", type);
	f.printRef("stream", stream);
}

//--

void StubVertexInputStream::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeBool(instanced);
	f.writeUint8(streamIndex);
	f.writeCompressedInt(streamSize);
	f.writeCompressedInt(streamStride);
	f.writeArray(elements);
}

void StubVertexInputStream::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	instanced = f.readBool();
	streamIndex = f.readUint8();
	streamSize = f.readCompressedInt();
	streamStride = f.readCompressedInt();
	f.readArray(elements);
}

void StubVertexInputStream::dump(StubDebugPrinter& f) const
{
	f.appendf("name={} index={} size={} stride={}, instanced={}", name, streamIndex, streamSize, streamStride, instanced);
	f.printChildArray("Element", elements);
}

//--

void StubFunction::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeRef(returnType);
	f.writeArray(attributes);
	f.writeArray(parameters);
	f.writeRef(code);
}

void StubFunction::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	f.readRef(returnType);
	f.readArray(attributes);
	f.readArray(parameters);
	f.readRef(code);
}

void StubFunction::dump(StubDebugPrinter& f) const
{
	f.appendf("name={}", name);

	f.append(" returnType=");
	returnType->print(f);
	f.printRef("returnTypeRef", returnType);

	f.printChildArray("Attribute", attributes);
	f.printChildArray("Parameter", parameters);

	f.printChild("Code", code);
}

//--

void StubScopeLocalVariable::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeRef(type);
}

void StubScopeLocalVariable::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	f.readRef(type);
}

void StubScopeLocalVariable::dump(StubDebugPrinter& f) const
{
	f.appendf("name={}", name, initialized);

	f.append(" type=");
	type->print(f);
	f.printRef("typeRef", type);
}

//--

void StubFunctionParameter::write(IStubWriter& f) const
{
	location.write(f);
	f.writeName(name);
	f.writeBool(reference);
	f.writeRef(type);
}

void StubFunctionParameter::read(IStubReader& f)
{
	location.read(f);
	name = f.readName();
	reference = f.readBool();
	f.readRef(type);
}

void StubFunctionParameter::dump(StubDebugPrinter& f) const
{
	f.appendf("name={}", name);

	f.append(" type=");
	type->print(f);
	f.printRef("typeRef", type);
}

//--

void StubOpcode::write(IStubWriter& f) const
{
	location.write(f);
}

void StubOpcode::read(IStubReader& f)
{
	location.read(f);
}

//--

void StubOpcodeVariableDeclaration::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(var);
	f.writeRef(init);
}

void StubOpcodeVariableDeclaration::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(var);
	f.readRef(init);
}

void StubOpcodeVariableDeclaration::dump(StubDebugPrinter& f) const
{
	f.printRef("Variable", var);

	if (init)
		f.printChild("Init", init);
}

//--

void StubOpcodeScope::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeArray(locals);
	f.writeArray(statements);
}

void StubOpcodeScope::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readArray(locals);
	f.readArray(statements);
}

void StubOpcodeScope::dump(StubDebugPrinter& f) const
{
	f.printChildArray("LocalVariable", locals);
	f.printChildArray("Statement", statements);
}

//--

void StubOpcodeLoad::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(valueReferece);
}

void StubOpcodeLoad::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(valueReferece);
}

void StubOpcodeLoad::dump(StubDebugPrinter& f) const
{
	f.printChild("Ptr", valueReferece);
}

//--

void StubOpcodeStore::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	mask.write(f);
	f.writeRef(type);
	f.writeRef(lvalue);
	f.writeRef(rvalue);
}

void StubOpcodeStore::read(IStubReader& f)
{
	StubOpcode::read(f);
	mask.read(f);
	f.readRef(type);
	f.readRef(lvalue);
	f.readRef(rvalue);
}

void StubOpcodeStore::dump(StubDebugPrinter& f) const
{
	f.appendf("mask={} type=", mask);
	type->print(f);
	f.printRef("typeRef", type);

	f.printChild("LValue", lvalue);
	f.printChild("RValue", rvalue);
}

//--

void StubOpcodeCast::write(IStubWriter& f) const
{
	f.writeEnum(generalType);
	f.writeRef(targetType);
	f.writeRef(value);
}

void StubOpcodeCast::read(IStubReader& f)
{
	f.readEnum(generalType);
	f.readRef(targetType);
	f.readRef(value);
}

void StubOpcodeCast::dump(StubDebugPrinter& f) const
{
	f.appendf("scalar={} type=", generalType);
	targetType->print(f);
	f.printRef("typeRef", targetType);

	f.printChild("Value", value);
}

//--

void StubOpcodeCreateArray::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(arrayTypeDecl);
	f.writeArray(elements);
}

void StubOpcodeCreateArray::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(arrayTypeDecl);
	f.readArray(elements);
}

void StubOpcodeCreateArray::dump(StubDebugPrinter& f) const
{
	f.appendf("type=");
	arrayTypeDecl->print(f);
	f.printRef("typeRef", arrayTypeDecl);

	f.printChildArray("ArrayElement", elements);
}

//--

void StubOpcodeCreateVector::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(typeDecl);
	f.writeArray(elements);
}

void StubOpcodeCreateVector::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(typeDecl);
	f.readArray(elements);
}

void StubOpcodeCreateVector::dump(StubDebugPrinter& f) const
{
	f.appendf("type=");
	typeDecl->print(f);
	f.printRef("typeRef", typeDecl);

	f.printChildArray("VectorElement", elements);
}

//--

void StubOpcodeCreateMatrix::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(typeDecl);
	f.writeArray(elements);
}

void StubOpcodeCreateMatrix::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(typeDecl);
	f.readArray(elements);
}

void StubOpcodeCreateMatrix::dump(StubDebugPrinter& f) const
{
	f.appendf("type=");
	typeDecl->print(f);
	f.printRef("typeRef", typeDecl);

	f.printChildArray("VectorElement", elements);
}

//--

void StubOpcodeResourceRef::write(IStubWriter& f) const
{
	f.writeEnum(type);
	f.writeRef(descriptorEntry);
	f.writeRef(index);
	f.writeUint8(numAddressComponents);
}

void StubOpcodeResourceRef::read(IStubReader& f)
{
	f.readEnum(type);
	f.readRef(descriptorEntry);
	f.readRef(index);
	numAddressComponents = f.readUint8();

}

void StubOpcodeResourceRef::dump(StubDebugPrinter& f) const
{
	f.printRef("descriptorEntry", descriptorEntry);
	if (index)
		f.printChild("Index", index);
}

///--

void StubOpcodeResourceLoad::write(IStubWriter& f) const
{
	f.writeRef(resourceRef);
	f.writeRef(address);
	f.writeUint8(numAddressComponents);
	f.writeUint8(numValueComponents);
}

void StubOpcodeResourceLoad::read(IStubReader& f)
{
	f.readRef(resourceRef);
	f.readRef(address);
	numAddressComponents = f.readUint8();
	numValueComponents = f.readUint8();
}

void StubOpcodeResourceLoad::dump(StubDebugPrinter& f) const
{
	f.printRef("Resource", resourceRef);
	f.printChild("Address", address);
}

///--

void StubOpcodeResourceStore::write(IStubWriter& f) const
{
	f.writeRef(resourceRef);
	f.writeRef(address);
	f.writeRef(value);
	f.writeUint8(numAddressComponents);
	f.writeUint8(numValueComponents);
}

void StubOpcodeResourceStore::read(IStubReader& f)
{
	f.readRef(resourceRef);
	f.readRef(address);
	f.readRef(value);
	numAddressComponents = f.readUint8();
	numValueComponents = f.readUint8();
}

void StubOpcodeResourceStore::dump(StubDebugPrinter& f) const
{
	f.printRef("Resource", resourceRef);
	f.printChild("Address", address);
	f.printChild("Value", address);
}

///--

void StubOpcodeResourceElement::write(IStubWriter& f) const
{
	f.writeRef(resourceRef);
	f.writeRef(address);
	f.writeUint8(numAddressComponents);
	f.writeUint8(numValueComponents);
}

void StubOpcodeResourceElement::read(IStubReader& f)
{
	f.readRef(resourceRef);
	f.readRef(address);
	numAddressComponents = f.readUint8();
	numValueComponents = f.readUint8();
}

void StubOpcodeResourceElement::dump(StubDebugPrinter& f) const
{
	f.printRef("Resource", resourceRef);
	f.printChild("Address", address);
}

//--

void StubOpcodeConstant::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(typeDecl);
	f.writeEnum(dataType);
	f.writeCompressedInt(dataSize);
	f.writeData(data, dataSize);
}

void StubOpcodeConstant::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(typeDecl);
	f.readEnum(dataType);
	dataSize = f.readCompressedInt();
	data = f.readData(dataSize);
}

void StubOpcodeConstant::dump(StubDebugPrinter& f) const
{
	f.appendf("size={} scalar={}", dataSize, dataType);

	switch (dataType)
	{
	case ScalarType::Float: 
	case ScalarType::Half:
		PrintArray<float>(f, data, dataSize);
		break;

	case ScalarType::Boolean:
	case ScalarType::Uint:
		PrintArray<uint32_t>(f, data, dataSize);
		break;

	case ScalarType::Int:
		PrintArray<int>(f, data, dataSize);
		break;

	case ScalarType::Int64:
		PrintArray<int64_t>(f, data, dataSize);
		break;

	case ScalarType::Uint64:
		PrintArray<uint64_t>(f, data, dataSize);
		break;				
	}
}

//--

void StubOpcodeAccessArray::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(arrayOp);
	f.writeRef(indexOp);
	f.writeRef(arrayType);
	f.writeInt32(staticIndex);
}

void StubOpcodeAccessArray::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(arrayOp);
	f.readRef(indexOp);
	f.readRef(arrayType);
	staticIndex = f.readInt32();
}

void StubOpcodeAccessArray::dump(StubDebugPrinter& f) const
{
	f.appendf("type=");
	arrayType->print(f);
	f.printRef("typeRef", arrayType);

	if (!indexOp)
		f.appendf("index={}", staticIndex);

	f.printChild("Array", arrayOp);

	if (indexOp)
		f.printChild("Index", indexOp);
}

//--

void StubOpcodeDataRef::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(stub);
}

void StubOpcodeDataRef::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(stub);
}

void StubOpcodeDataRef::dump(StubDebugPrinter& f) const
{
	if (stub)
	{
		f.appendf("refType={}", stub->debugName());
		f.printRef("ref", stub);
	}
	else
	{
		f.append("INVALID");
	}
}

//--

void StubOpcodeAccessMember::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(value);
	f.writeName(name);
	f.writeRef(member);
}

void StubOpcodeAccessMember::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(value);
	name = f.readName();
	f.readRef(member);
}

void StubOpcodeAccessMember::dump(StubDebugPrinter& f) const
{
	f.appendf("name={}", name);

	if (member)
	{
		if (member->owner)
		{
			f.appendf(" struct={}", member->owner->name);
			f.printRef("structRef", member->owner);
		}

		f.printRef("memberRef", member);
	}
}

//--

void StubOpcodeSwizzle::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(value);
	f.writeUint8(inputComponents);
	f.writeUint8(outputComponents);
	swizzle.write(f);
}

void StubOpcodeSwizzle::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(value);
	inputComponents = f.readUint8();
	outputComponents = f.readUint8();
	swizzle.read(f);
}

void StubOpcodeSwizzle::dump(StubDebugPrinter& f) const
{
	f.appendf("inputCount={} outputCount={} swizzle={}", inputComponents, outputComponents, swizzle);
	f.printChild("Value", value);
}

//--

void StubOpcodeNativeCall::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeName(name);
	f.writeRef(returnType);
	f.writeArray(arguments);
	f.writeArray(argumentTypes);
}

void StubOpcodeNativeCall::read(IStubReader& f)
{
	StubOpcode::read(f);
	name = f.readName();
	f.readRef(returnType);
	f.readArray(arguments);
	f.readArray(argumentTypes);
}

void StubOpcodeNativeCall::dump(StubDebugPrinter& f) const
{
	f.appendf("name={}", name);

	if (returnType)
	{
		f.appendf(" returnType=");
		returnType->print(f);
		f.printRef("returnTypeRef", returnType);
	}

	f.printChildArray("NativeCallArgument", arguments);
	f.printChildRefArray("NativeCallArgumentTypes", argumentTypes);
}

//--

void StubOpcodeCall::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(func);
	f.writeArray(arguments);
	f.writeArray(argumentTypes);
}

void StubOpcodeCall::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(func);
	f.readArray(arguments);
	f.readArray(argumentTypes);
}

void StubOpcodeCall::dump(StubDebugPrinter& f) const
{
	if (func)
	{
		f.appendf("name={}", func->name);

		if (func->returnType)
		{
			f.appendf(" returnType=");
			func->returnType->print(f);
			f.printRef("returnTypeRef", func->returnType);
		}

		f.printRef("funcRef", func);
	}

	f.printChildArray("CallArgument", arguments);
	f.printChildRefArray("CallArgumentTypes", argumentTypes);
}

//--

void StubOpcodeIfElse::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeInt8(branchHint);
	f.writeArray(conditions);
	f.writeArray(statements);
	f.writeRef(elseStatement);
}

void StubOpcodeIfElse::read(IStubReader& f)
{
	StubOpcode::read(f);
	branchHint = f.readInt8();
	f.readArray(conditions);
	f.readArray(statements);
	f.readRef(elseStatement);
}

void StubOpcodeIfElse::dump(StubDebugPrinter& f) const
{
	f.appendf("branchHint={}", branchHint);
	f.printChildArray("Condition", conditions);
	f.printChildArray("Statement", statements);
	if (elseStatement)
		f.printChild("Else", elseStatement);
}

//--

void StubOpcodeLoop::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeInt8(unrollHint);
	f.writeCompressedInt(dependencyLength);
	f.writeRef(init);
	f.writeRef(condition);
	f.writeRef(increment);
	f.writeRef(body);
}

void StubOpcodeLoop::read(IStubReader& f)
{
	StubOpcode::read(f);
	unrollHint = f.readInt8();
	dependencyLength = f.readCompressedInt();
	f.readRef(init);
	f.readRef(condition);
	f.readRef(increment);
	f.readRef(body);
}

void StubOpcodeLoop::dump(StubDebugPrinter& f) const
{
	f.appendf("unrollHint={}, dependencyLength=", unrollHint, dependencyLength);
	f.printChild("LoopInit", init);
	f.printChild("LoopCondition", condition);
	f.printChild("LoopIncrement", increment);
	f.printChild("LoopBody", body);
}

//--

void StubOpcodeReturn::write(IStubWriter& f) const
{
	StubOpcode::write(f);
	f.writeRef(value);
}

void StubOpcodeReturn::read(IStubReader& f)
{
	StubOpcode::read(f);
	f.readRef(value);
}

void StubOpcodeReturn::dump(StubDebugPrinter& f) const
{
	if (value)
		f.printChild("ReturnValue", value);
}

//--

void StubOpcodeBreak::write(IStubWriter& f) const
{
	StubOpcode::write(f);
}

void StubOpcodeBreak::read(IStubReader& f)
{
	StubOpcode::read(f);
}

//--

void StubOpcodeContinue::write(IStubWriter& f) const
{
	StubOpcode::write(f);
}

void StubOpcodeContinue::read(IStubReader& f)
{
	StubOpcode::read(f);
}

//--

void StubOpcodeExit::write(IStubWriter& f) const
{
	StubOpcode::write(f);
}

void StubOpcodeExit::read(IStubReader& f)
{
	StubOpcode::read(f);
}

//--
		
static char SafePathChar(char ch)
{
	if (ch >= 'A' && ch <= 'Z') return ch;
	if (ch >= 'a' && ch <= 'z') return ch;
	if (ch >= '0' && ch <= '9') return ch;

	switch (ch)
	{
	case '_':
	case '-':
	case '+':
	case '[':
	case ']':
	case '(':
	case ')':
		return ch;
	}

	return '_';
}

void AssembleDumpFileName(StringBuilder& f, const StringView contextName, const StringView contextOptions, const StringView type)
{
	auto fileName = contextName.fileStem();
	if (fileName.empty())
		fileName = "UnnamedShader";

	auto pathHash = contextName.calcCRC64();
	f.appendf("{}_{}", fileName, Hex(pathHash));

	if (contextOptions)
	{
		f.append("_(");
		for (auto ch : contextOptions)
			f.appendch(SafePathChar(ch));
		f.append(")");
	}

	if (type)
		f.appendf(".{}", type);

	f.append(".txt");
}

StringBuf AssembleDumpFileName(const StringView contextName, const StringView contextOptions, const StringView type)
{
	StringBuilder txt;
	AssembleDumpFileName(txt, contextName, contextOptions, type);
	return txt.toString();
}

StringBuf AssembleDumpFilePath(const StringView contextName, const StringView contextOptions, const StringView type)
{
	const auto& tempDir = io::SystemPath(io::PathCategory::LocalTempDir);

	StringBuilder txt;
	txt << tempDir;
	txt << "shader_dump/";
	AssembleDumpFileName(txt, contextName, contextOptions, type);

	return txt.toString();
}

//--

END_BOOMER_NAMESPACE_EX(gpu::shader)
