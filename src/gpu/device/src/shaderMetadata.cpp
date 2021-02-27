/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
*/

#include "build.h"
#include "shaderMetadata.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//--

RTTI_BEGIN_TYPE_CLASS(ShaderVertexElementMetadata);
	RTTI_PROPERTY(name);
	RTTI_PROPERTY(format);
	RTTI_PROPERTY(offset);
	RTTI_PROPERTY(size);
RTTI_END_TYPE();

void ShaderVertexElementMetadata::print(IFormatStream& f) const
{
	f.appendf("[{}]: {} {} at offset {}, size {}", name, format, offset, size);
}

//--

RTTI_BEGIN_TYPE_CLASS(ShaderVertexStreamMetadata);
	RTTI_PROPERTY(name);
	RTTI_PROPERTY(size);
	RTTI_PROPERTY(index);
	RTTI_PROPERTY(stride);
	RTTI_PROPERTY(instanced);
	RTTI_PROPERTY(elements);
RTTI_END_TYPE();

void ShaderVertexStreamMetadata::print(IFormatStream& f) const
{
	f.appendf("Stream {}, size {}, stride {}, index {} {}\n", name, size, stride, index, instanced ? "INSTANCED" : "");

	for (auto i : elements.indexRange())
		f.appendf("    [{}]: {}\n", i, elements[i]);
}

static void PrintShaderStageMask(ShaderStageMask mask, IFormatStream& f)
{
	f.append(mask.test(ShaderStage::Vertex) ? "V" : "-");
	f.append(mask.test(ShaderStage::Geometry) ? "G" : "-");
	f.append(mask.test(ShaderStage::Domain) ? "D" : "-");
	f.append(mask.test(ShaderStage::Hull) ? "H" : "-");
	f.append(mask.test(ShaderStage::Pixel) ? "P" : "-");
	f.append(mask.test(ShaderStage::Compute) ? "C" : "-");
	f.append(mask.test(ShaderStage::Task) ? "T" : "-");
	f.append(mask.test(ShaderStage::Mesh) ? "M" : "-");
}

//--

RTTI_BEGIN_TYPE_CLASS(ShaderDescriptorEntryMetadata);
	RTTI_PROPERTY(name);
	RTTI_PROPERTY(index);
	RTTI_PROPERTY(type);
	RTTI_PROPERTY(viewType);
	RTTI_PROPERTY_FORCE_TYPE(stageMask, uint16_t);
	RTTI_PROPERTY(format);
	RTTI_PROPERTY(number);
RTTI_END_TYPE();

void ShaderDescriptorEntryMetadata::print(IFormatStream& f) const
{
	f.appendf("{} ({}) ", type, name);
	PrintShaderStageMask(stageMask, f);

	switch (type)
	{
	case DeviceObjectViewType::ConstantBuffer:
		f.appendf(" size={}", number);
		break;

	case DeviceObjectViewType::Buffer:
	case DeviceObjectViewType::BufferWritable:
	case DeviceObjectViewType::ImageWritable:
		f.appendf(" format={}", format);
		break;

	case DeviceObjectViewType::BufferStructured:
	case DeviceObjectViewType::BufferStructuredWritable:
		f.appendf(" stride={}", number);
		break;
	}

	if (type == DeviceObjectViewType::Image || type == DeviceObjectViewType::ImageWritable)
		f.appendf(" view={}", viewType);

	if (type == DeviceObjectViewType::Image)
	{
		if (number >= 1)
			f.appendf("  staticSampler={}", number-1);
		else if(number <= -1)
			f.appendf(" localSampler={}", -number - 1);
	}
}

//--

RTTI_BEGIN_TYPE_CLASS(ShaderDescriptorMetadata);
	RTTI_PROPERTY(name);
	RTTI_PROPERTY(index);
	RTTI_PROPERTY_FORCE_TYPE(stageMask, uint16_t);
	RTTI_PROPERTY(elements);
RTTI_END_TYPE();

void ShaderDescriptorMetadata::print(IFormatStream& f) const
{
	f.appendf("Descritor {} ", name);
	PrintShaderStageMask(stageMask, f);
	f << "\n";

	for (auto i : elements.indexRange())
		f.appendf("    [{}] {}\n", i, elements[i]);
}

//--

RTTI_BEGIN_TYPE_CLASS(ShaderStaticSamplerMetadata);
	RTTI_PROPERTY(name);
	RTTI_PROPERTY_FORCE_TYPE(stageMask, uint16_t);
	RTTI_PROPERTY(state);
RTTI_END_TYPE();

void ShaderStaticSamplerMetadata::print(IFormatStream& f) const
{
	f.appendf("Sampler '{}' ");
	PrintShaderStageMask(stageMask, f);
	f.append(":\n");
	state.print(f);
}

//--

RTTI_BEGIN_TYPE_BITFIELD(ShaderStageMask);
	RTTI_BITFIELD_OPTION(Vertex);
	RTTI_BITFIELD_OPTION(Geometry);
	RTTI_BITFIELD_OPTION(Domain);
	RTTI_BITFIELD_OPTION(Hull);
	RTTI_BITFIELD_OPTION(Pixel);
	RTTI_BITFIELD_OPTION(Compute);
	RTTI_BITFIELD_OPTION(Task);
	RTTI_BITFIELD_OPTION(Mesh);		
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_BITFIELD(ShaderFeatureMask);
	RTTI_BITFIELD_OPTION(ControlFlowHints)
	RTTI_BITFIELD_OPTION(UAVPixelShader);
	RTTI_BITFIELD_OPTION(UAVOutsidePixelShader);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(ShaderMetadata);
	RTTI_PROPERTY(stageMask);
	RTTI_PROPERTY(key);
	RTTI_PROPERTY(vertexLayoutKey);
	RTTI_PROPERTY(descriptorLayoutKey);
	RTTI_PROPERTY(computeGroupSizeX);
	RTTI_PROPERTY(computeGroupSizeY);
	RTTI_PROPERTY(computeGroupSizeZ);
	RTTI_PROPERTY(featureMask);
	RTTI_PROPERTY(descriptors);
	RTTI_PROPERTY(vertexStreams);
	RTTI_PROPERTY(staticSamplers);
	RTTI_PROPERTY(renderStates);
RTTI_END_TYPE();

ShaderMetadata::ShaderMetadata()
{}

void ShaderMetadata::print(IFormatStream& f) const
{
	f.appendf("Used stages: ");
	PrintShaderStageMask(stageMask, f);
	f << "\n";

	if (stageMask.test(ShaderStage::Compute))
		f.appendf("Compute group size: {}x{}x{}\n", computeGroupSizeX, computeGroupSizeY, computeGroupSizeZ);

	if (stageMask.test(ShaderStage::Pixel))
	{
//			f.appendf("Feature mask: {}\n", featureMask);

		f.appendf("Render states: {}\n", renderStates);
	}

	if (!descriptors.empty())
	{
		f.appendf("Uses {} descriptors:\n", descriptors.size());
		for (auto i : descriptors.indexRange())
			f.appendf("  [{}] {}\n", i, descriptors[i]);
	}

	if (!vertexStreams.empty())
	{
		f.appendf("Uses {} vertex streams:\n", vertexStreams.size());
		for (auto i : vertexStreams.indexRange())
			f.appendf("  [{}] {}\n", i, vertexStreams[i]);
	}

	if (!staticSamplers.empty())
	{
		f.appendf("Uses {} static samplers:\n", staticSamplers.size());
		for (auto i : staticSamplers.indexRange())
			f.appendf("  [{}] {}\n", i, staticSamplers[i]);
	}
}

void ShaderMetadata::cacheRuntimeData()
{
	{
		InplaceArray<DeviceObjectViewType, 32> viewTypes;
		for (auto& desc : descriptors)
		{
			viewTypes.reserve(desc.elements.size());
			viewTypes.reset();

			for (const auto& elem : desc.elements)
				viewTypes.pushBack(elem.type);

			desc.id = DescriptorID::FromTypes(viewTypes.typedData(), viewTypes.size());
			TRACE_INFO("Descriptor '{}' determine to have layout '{}'", desc.name, desc.id);
		}
	}

	if (vertexLayoutKey == 0 && !vertexStreams.empty())
	{
		CRC64 crc;
		for (const auto& element : vertexStreams)
		{
			crc << element.name;
			crc << element.instanced;
			crc << element.size;
			crc << element.stride;

			crc << element.elements.size();
			for (const auto& elem : element.elements)
			{
				//crc << (int)elem.name;
				crc << (int)elem.format;
				crc << elem.offset;
				crc << elem.size;
			}
		}

		vertexLayoutKey = crc;
	}

	if (descriptorLayoutKey == 0)// && !descriptors.empty())
	{
		CRC64 crc;
		for (const auto& element : descriptors)
		{
			crc << element.name;
			crc << element.stageMask.rawValue();

			crc << element.elements.size();
			for (const auto& elem : element.elements)
			{
				crc << (uint8_t) elem.type;

				switch (elem.type)
				{
					case DeviceObjectViewType::ConstantBuffer:
					case DeviceObjectViewType::BufferStructured:
					case DeviceObjectViewType::BufferStructuredWritable:
					case DeviceObjectViewType::Sampler:
						crc << elem.number;
						break;

					case DeviceObjectViewType::Buffer:
					case DeviceObjectViewType::BufferWritable:
						crc << (uint8_t)elem.format;
						break;

					case DeviceObjectViewType::ImageWritable:
					case DeviceObjectViewType::Image:
						crc << (uint8_t)elem.viewType;
						crc << (uint8_t)elem.format;
						break;

					case DeviceObjectViewType::SampledImage:
						crc << (uint8_t)elem.viewType;
						crc << (uint8_t)elem.number;
						break;
				}
			}

			for (const auto& sampler : staticSamplers)
			{
				crc << (char)sampler.state.magFilter;
				crc << (char)sampler.state.minFilter;
				crc << (char)sampler.state.mipmapMode;
				crc << (char)sampler.state.addresModeU;
				crc << (char)sampler.state.addresModeV;
				crc << (char)sampler.state.addresModeW;
				crc << sampler.state.compareEnabled;
				crc << (char)sampler.state.compareOp;
				crc << (char)sampler.state.borderColor;
				crc << sampler.state.maxAnisotropy;
				crc << sampler.state.mipLodBias;
				crc << sampler.state.minLod;
				crc << sampler.state.maxLod;
			}
		}

		descriptorLayoutKey = crc;
	}
}

void ShaderMetadata::onPostLoad()
{
	TBaseClass::onPostLoad();
	cacheRuntimeData();
}

//--

END_BOOMER_NAMESPACE_EX(gpu)
