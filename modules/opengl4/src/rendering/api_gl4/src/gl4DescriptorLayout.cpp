/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4DescriptorLayout.h"
#include "gl4ObjectCache.h"
#include "gl4Thread.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{
			//--

			static uint32_t CountDescriptorElements(const base::Array<ShaderDescriptorMetadata>& descriptors)
			{
				uint32_t ret = 0;
				for (const auto& info : descriptors)
					ret += info.elements.size();
				return ret;
			}

			//--

			DescriptorBindingLayout::DescriptorBindingLayout(Thread* owner, const base::Array<ShaderDescriptorMetadata>& descriptors, const base::Array<ShaderStaticSamplerMetadata>& staticSamplers)
				: IBaseDescriptorBindingLayout(descriptors)
				, m_owner(owner)
			{
				// binding slot assignment
				// NOTE: MUST MATCH THE ONE IN GLSL CODE GENERATOR!!!!
				uint8_t numUniformBuffers = 0;
				uint8_t numStorageBuffers = 0;
				uint8_t numImages = 0;
				uint8_t numTextures = 0;
				uint8_t numSamplers = 0;

				// copy sampler initialization data, discarded once final objects are created
				m_staticSamplersStates.reserve(staticSamplers.size());
				for (const auto& info : staticSamplers)
					m_staticSamplersStates.emplaceBack(info.state);

				// create new state
				m_elements.reserve(CountDescriptorElements(descriptors));
				for (auto i : descriptors.indexRange())
				{
					const auto& descriptor = descriptors[i];
					const auto descriptorBindingIndex = owner->objectCache()->resolveDescriptorBindPointIndex(descriptor.name, descriptor.id);

					for (auto j : descriptor.elements.indexRange())
					{
						const auto& element = descriptor.elements[j];

						if (element.type == DeviceObjectViewType::Sampler)
							continue; // samplers are not found directly

						auto& bindingElement = m_elements.emplaceBack();
						bindingElement.bindPointName = descriptor.name.view();
						bindingElement.bindPointIndex = descriptorBindingIndex;
						bindingElement.bindPointLayout = descriptor.id;
						bindingElement.elementName = element.name.view();
						bindingElement.elementIndex = j;
						bindingElement.objectType = element.type;
						bindingElement.objectFormat = element.format;

						switch (element.type)
						{
							case DeviceObjectViewType::ConstantBuffer:
								bindingElement.objectSlot = numUniformBuffers++;
								break;

							case DeviceObjectViewType::Buffer:
							case DeviceObjectViewType::BufferWritable:
								bindingElement.objectSlot = numImages++;
								break;

							case DeviceObjectViewType::BufferStructured:
							case DeviceObjectViewType::BufferStructuredWritable:
								bindingElement.objectSlot = numStorageBuffers++;
								break;

							case DeviceObjectViewType::Image:
								bindingElement.objectSlot = numTextures++;
								break;

							case DeviceObjectViewType::ImageWritable:
								bindingElement.objectSlot = numImages++;
								break;

							case DeviceObjectViewType::ImageTable:
								bindingElement.objectSlot = numUniformBuffers++;
								break;

							default:
								DEBUG_CHECK(!"Rendering: Invalid resource type");
						}

						if (element.type == DeviceObjectViewType::Image || element.type == DeviceObjectViewType::ImageTable)
						{
							if (element.number < 0)
							{
								const auto localSamplerIndex = -(element.number + 1);
								ASSERT(localSamplerIndex >= 0 && localSamplerIndex <= descriptor.elements.lastValidIndex());
								ASSERT(descriptor.elements[localSamplerIndex].type == DeviceObjectViewType::Sampler);
								bindingElement.samplerSlotIndex = localSamplerIndex; // local sampler slot in the descriptor
								bindingElement.glStaticSampler = 0;
							}
							else if (element.number > 0)
							{
								const auto staticSamplerIndex = element.number - 1;
								ASSERT(staticSamplerIndex >= 0 && staticSamplerIndex <= descriptor.elements.lastValidIndex());
								bindingElement.samplerSlotIndex = -1; // use static sampler
								bindingElement.glStaticSampler = staticSamplerIndex;
							}
						}
					}
				}
			}

			DescriptorBindingLayout::~DescriptorBindingLayout()
			{}

			//--

			void DescriptorBindingLayout::prepare()
			{
				if (m_prepared)
					return;

				if (!m_staticSamplersStates.empty())
				{
					// create samplers
					const auto staticSamplers = std::move(m_staticSamplersStates);
					m_staticSamplers.reserve(staticSamplers.size());
					for (const auto& sampler : staticSamplers)
						m_staticSamplers.pushBack(m_owner->objectCache()->createSampler(sampler));

					// push the samplers into descriptor bindings
					for (auto& element : m_elements)
						if (element.samplerSlotIndex == -1)
							element.glStaticSampler = m_staticSamplers[element.glStaticSampler];
				}

				m_prepared = true;
			}

			//--

		} // gl4
    } // gl4
} // rendering
