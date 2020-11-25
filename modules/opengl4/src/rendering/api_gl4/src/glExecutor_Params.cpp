/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\frame\execution #]
***/

#include "build.h"
#include "glExecutor.h"
#include "glShaders.h"
#include "glUtils.h"
#include "glTempBuffer.h"

#include "rendering/device/include/renderingCommands.h"
#include "rendering/device/include/renderingDescriptorInfo.h"
#include "rendering/device/include/renderingDescriptor.h"

namespace rendering
{
    namespace gl4
    {
        namespace exec
        {

            //---

			void FrameExecutor::applyDescriptorEntry(const ResolvedParameterBindingState::BindingElement& elem, const DescriptorEntry& entry)
			{
				// validate that object is bound (most likely is filtered on the FE)
				if (elem.objectType != DeviceObjectViewType::ConstantBuffer)
				{
					ASSERT_EX(entry.id, base::TempString("Invalid object view in param table '{}', member '{}' at index'{}'. Expected {} got NULL",
						elem.bindPointName, elem.paramName, elem.descriptorElementIndex, elem.objectType));
				}

				// bind required resources
				switch (elem.objectType)
				{
					case DeviceObjectViewType::ConstantBuffer:
					{
						if (entry.offsetPtr == nullptr)
						{
							ASSERT_EX(entry.id, "Missing ConstantBufferView in passed descriptor entry bindings");

							const auto view = resolveUntypedBufferView(entry.id);
							ASSERT_EX(view, base::TempString("Unable to resolve constant buffer view in param table '{}', member '{}' at index'{}'.",
								elem.bindPointName, elem.paramName, elem.descriptorElementIndex));
							m_objectBindings.bindUniformBuffer(elem.objectSlot, view);
						}
						else
						{
							ASSERT_EX(!entry.id, "Both offset pointer and resource view ID are set")
							auto assignedRealOffset = *entry.offsetPtr + entry.offset;
							auto resolvedView = m_tempConstantBuffer->resolveUntypedView(assignedRealOffset, entry.size);
							m_objectBindings.bindUniformBuffer(elem.objectSlot, resolvedView);
						}
						break;
					}

					case DeviceObjectViewType::BufferWritable:
					case DeviceObjectViewType::Buffer:
					{
						const auto view = resolveTypedBufferView(entry.id);
						ASSERT_EX(view, base::TempString("Unable to resolve buffer view in param table '{}', member '{}' at index'{}'.",
							elem.bindPointName, elem.paramName, elem.descriptorElementIndex));
						m_objectBindings.bindImage(elem.objectSlot, view, elem.objectReadWriteMode);
						break;
					}

					case DeviceObjectViewType::BufferStructured:
					case DeviceObjectViewType::BufferStructuredWritable:
					{
						const auto view = resolveUntypedBufferView(entry.id);
						ASSERT_EX(view, base::TempString("Unable to resolve structured buffer view in param table '{}', member '{}' at index'{}'.",
							elem.bindPointName, elem.paramName, elem.descriptorElementIndex));
						m_objectBindings.bindStorageBuffer(elem.objectSlot, view);
						break;
					}

					case DeviceObjectViewType::Image:
					{
						const auto view = resolveImageView(entry.id);
						ASSERT_EX(view, base::TempString("Unable to resolve image view in param table '{}', member '{}' at index'{}'.",
							elem.bindPointName, elem.paramName, elem.descriptorElementIndex));

						m_objectBindings.bindTexture(elem.objectSlot, view);
						m_objectBindings.bindSampler(elem.objectSlot, view.glSampler); // TODO: static samplers, yeah :D
						break;
					}

					case DeviceObjectViewType::ImageWritable:
					{
						const auto view = resolveImageView(entry.id);
						ASSERT_EX(view, base::TempString("Unable to resolve image view in param table '{}', member '{}' at index'{}'.",
							elem.bindPointName, elem.paramName, elem.descriptorElementIndex));

						ResolvedFormatedView formatedView;
						formatedView.glBufferView = view.glImageView;
						formatedView.glViewFormat = TranslateImageFormat(elem.objectFormat);// view.format());

						m_objectBindings.bindImage(elem.objectSlot, formatedView, elem.objectReadWriteMode);
						break;
					}

					default:
					{
						ASSERT(!"Unsupported resource type");
					}
				}
			}

            void FrameExecutor::applyDescriptors(const Shaders& shaders, PipelineIndex parameterBindingStateIndex)
            {
                if (const auto* parameterBinding = shaders.parameterBindingState(parameterBindingStateIndex))
                {
                    for (const auto& elem : parameterBinding->bindingElements)
                    {
                        // do we have parameter ?
						ASSERT_EX(elem.bindPointIndex < m_descriptors.descriptors.size() && m_descriptors.descriptors[elem.bindPointIndex], 
							base::TempString("Missing parameters for '{}', bind point {}, expected layout: {}, {} bounds descriptrs",
							elem.bindPointName, elem.bindPointIndex, elem.bindPointLayout, printBoundDescriptors()));

                        // validate expected layout 
                        const auto& descriptor = m_descriptors.descriptors[elem.bindPointIndex];
						ASSERT_EX(descriptor.layoutPtr->id() == elem.bindPointLayout, base::TempString("Incompatible parameter layout for '{}': '{}' != '{}'",
							elem.bindPointName, descriptor.layoutPtr->id(), elem.bindPointLayout));

						// get the source data in descriptor memory
						const auto& descriptorEntry = descriptor.dataPtr[elem.descriptorElementIndex];
						applyDescriptorEntry(elem, descriptorEntry);
                    } 
                }
            }

            //---

        } // exec
    } // gl4
} // rendering
