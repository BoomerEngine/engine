/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shader #]
*/

#include "build.h"
#include "renderingShaderMetadata.h"
#include "renderingShaderStubs.h"

namespace rendering
{
	//--

	static bool HasAttribute(const base::StubPseudoArray<shader::StubAttribute>& attributes, base::StringID name)
	{
		for (const auto* attr : attributes)
			if (attr->name == name)
				return true;

		return false;
	}

	template< typename T >
	static bool FindAttributeVal(const base::StubPseudoArray<shader::StubAttribute>& attributes, base::StringID name, T& outVal)
	{
		for (const auto* attr : attributes)
			if (attr->name == name)
				return attr->value.match(outVal) == base::MatchResult::OK;

		return false;
	}

	ShaderMetadataPtr ShaderMetadata::BuildFromStubs(const shader::StubProgram* program, uint64_t key)
	{
		auto ret = base::RefNew<ShaderMetadata>();

		uint32_t index = 0;
		for (const auto* desc : program->descriptors)
		{
			auto& info = ret->descriptors.emplaceBack();
			info.index = index++;
			info.name = desc->name;

			for (const auto* descMember : desc->members)
			{
				auto& memberInfo = info.elements.emplaceBack();
				memberInfo.name = descMember->name;
				memberInfo.index = descMember->index;

				if (const auto* obj = descMember->asDescriptorMemberConstantBuffer())
				{
					memberInfo.type = DeviceObjectViewType::ConstantBuffer;
					memberInfo.number = obj->size;
				}
				else if (const auto* obj = descMember->asDescriptorMemberFormatBuffer())
				{
					memberInfo.type = obj->writable ? DeviceObjectViewType::BufferWritable : DeviceObjectViewType::Buffer;
					memberInfo.format = obj->format;
				}
				else if (const auto* obj = descMember->asDescriptorMemberStructuredBuffer())
				{
					memberInfo.type = obj->writable ? DeviceObjectViewType::BufferStructuredWritable : DeviceObjectViewType::BufferStructured;
					memberInfo.number = obj->stride;
				}
				else if (const auto* obj = descMember->asDescriptorMemberSampler())
				{
					memberInfo.type = DeviceObjectViewType::Sampler;
				}
				else if (const auto* obj = descMember->asDescriptorMemberSampledImage())
				{
					memberInfo.type = DeviceObjectViewType::Image;

					if (obj->staticState)
						memberInfo.number = 1 + obj->staticState->index;
					else if (obj->dynamicSamplerDescriptorEntry)
						memberInfo.number = -(1 + obj->dynamicSamplerDescriptorEntry->index);
					else
						memberInfo.number = 0; // no sampler
				}
				else if (const auto* obj = descMember->asDescriptorMemberSampledImageTable())
				{
					memberInfo.type = DeviceObjectViewType::ImageTable;

					if (obj->staticState)
						memberInfo.number = 1 + obj->staticState->index;
					else if (obj->dynamicSamplerDescriptorEntry)
						memberInfo.number = -(1 + obj->dynamicSamplerDescriptorEntry->index);
					else
						memberInfo.number = 0; // no sampler
				}
				else if (const auto* obj = descMember->asDescriptorMemberImage())
				{
					memberInfo.type = DeviceObjectViewType::ImageWritable;
					memberInfo.format = obj->format;
				}
			}
		}

		for (const auto* desc : program->vertexStreams)
		{
			auto& info = ret->vertexStreams.emplaceBack();
			info.name = desc->name;
			info.instanced = desc->instanced;
			info.index = desc->streamIndex;
			info.size = desc->streamSize;
			info.stride = desc->streamStride;

			for (const auto* descMember : desc->elements)
			{
				auto& memberInfo = info.elements.emplaceBack();
				memberInfo.name = descMember->name;
				memberInfo.offset = descMember->elementOffset;
				memberInfo.format = descMember->elementFormat;
				memberInfo.size = descMember->elementSize;
			}
		}

		for (const auto* desc : program->samplers)
		{
			auto& info = ret->staticSamplers.emplaceBack();
			info.name = desc->name;
			info.state = desc->state;
		}

		for (uint32_t i = 0; i < program->stages.size(); ++i)
		{
			if (auto* stage = program->stages[i])
			{
				auto shaderStage = (ShaderStage)i;
				ret->stageMask |= shaderStage;

				if (stage->entryFunction)
				{
					if (shaderStage == ShaderStage::Pixel)
					{
						if (HasAttribute(stage->entryFunction->attributes, "early_fragment_tests"_id))
							ret->usesPixelShaderEarlyTest = true;

						// TODO: usesPixelShaderDiscard 
						// TODO: usesPixelShaderWritesDepth
					}
					else if (shaderStage == ShaderStage::Compute)
					{
						FindAttributeVal(stage->entryFunction->attributes, "local_size_x"_id, ret->computeGroupSizeX);
						FindAttributeVal(stage->entryFunction->attributes, "local_size_y"_id, ret->computeGroupSizeY);
						FindAttributeVal(stage->entryFunction->attributes, "local_size_z"_id, ret->computeGroupSizeZ);
					}
				}

				// mark descriptors used in this stage
				for (const auto* descMember : stage->descriptorMembers)
				{
					const auto* desc = descMember->descriptor;
					ASSERT(desc);

					for (auto& descInfo : ret->descriptors)
					{
						if (descInfo.name == desc->name)
						{
							descInfo.stageMask |= shaderStage;

							for (auto& descMemberInfo : descInfo.elements)
								if (descMemberInfo.name == descMember->name)
									descMemberInfo.stageMask |= shaderStage;

							break;
						}
					}

					base::StringID samplerName, staticSamplerName;
					if (auto* image = descMember->asDescriptorMemberSampledImage())
					{
						if (image->dynamicSamplerDescriptorEntry)
							samplerName = image->dynamicSamplerDescriptorEntry->name;
						else if (image->staticState)
							staticSamplerName = image->staticState->name;
					}
					else if (auto* imageTable = descMember->asDescriptorMemberSampledImageTable())
					{
						if (imageTable->dynamicSamplerDescriptorEntry)
							samplerName = imageTable->dynamicSamplerDescriptorEntry->name;
						else if (imageTable->staticState)
							staticSamplerName = imageTable->staticState->name;
					}

					if (samplerName)
					{
						for (auto& descInfo : ret->descriptors)
						{
							if (descInfo.name == samplerName)
							{
								descInfo.stageMask |= shaderStage;
								break;
							}
						}
					}
					else if (staticSamplerName)
					{
						for (auto& descInfo : ret->staticSamplers)
						{
							if (descInfo.name == staticSamplerName)
							{
								descInfo.stageMask |= shaderStage;
								break;
							}
						}
					}
				}				
			}
		}
		
		ret->key = key;

		if (program->renderStates)
			ret->renderStates = program->renderStates->states;

		ret->cacheRuntimeData();
		return ret;
	}

	//--

} // rendering
