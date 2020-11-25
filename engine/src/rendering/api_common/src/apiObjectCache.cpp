/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiObjectCache.h"

#include "rendering/device/include/renderingShaderLibrary.h"
#include "rendering/device/include/renderingDescriptor.h"

namespace rendering
{
    namespace api
    {
		//---

		IBaseVertexBindingLayout::IBaseVertexBindingLayout(base::Array<BindingInfo>&& data)
			: m_vertexBindPoints(std::move(data))
		{}

		IBaseVertexBindingLayout::~IBaseVertexBindingLayout()
		{}

		void IBaseVertexBindingLayout::print(base::IFormatStream& f) const
		{
			f.appendf("{} vertex streams:\n", m_vertexBindPoints.size());

			for (const auto& v : m_vertexBindPoints)
			{
				f.appendf("  Stream [{}]: {}, stride {}{}, {} element(s)\n", v.bindPointIndex, v.name, v.stride, v.instanced ? " INSTANCED" : "", v.attributes.size());

				for (auto i : v.attributes.indexRange())
				{
					const auto& a = v.attributes[i];
					f.appendf("    Attribute [{}]: {} at offset {}\n", i, a.format, a.offset);
				}
			}
		}

        //---

		DescriptorBindingElement::DescriptorBindingElement()
		{}

		void DescriptorBindingElement::print(base::IFormatStream& f) const
		{
			f.appendf("{}({}) at {}({}): {}", paramName, descriptorElementIndex, bindPointName, bindPointIndex, objectType);

			if (objectFormat != ImageFormat::UNKNOWN)
				f.appendf(", format: {}", objectFormat);

			if (writable)
				f.appendf(", writable");

			f.appendf(" at slot {}", apiObjectSlot);
		}

		//---

		IBaseDescriptorBindingLayout::IBaseDescriptorBindingLayout(const base::Array<DescriptorBindingElement>& elements)
			: m_elements(elements)
		{}

		IBaseDescriptorBindingLayout::~IBaseDescriptorBindingLayout()
		{}

		void IBaseDescriptorBindingLayout::print(base::IFormatStream& f) const
		{
			f.appendf("{} binding elements (API: {})\n", m_elements.size());

			for (auto i : m_elements.indexRange())
			{
				const auto& b = m_elements[i];
				f.appendf("  Binding [{}]: {}\n", i, b);
			}
		}

		//---

        IBaseObjectCache::IBaseObjectCache(IBaseThread* owner)
            : m_owner(owner)
        {
            m_vertexLayoutMap.reserve(256);
            m_vertexBindPointMap.reserve(256);
        }

        IBaseObjectCache::~IBaseObjectCache()
        {
			ASSERT_EX(m_vertexBindPointMap.empty(), "Cache not cleared properly");
			ASSERT_EX(m_descriptorBindPointMap.empty(), "Cache not cleared properly");
			ASSERT_EX(m_descriptorBindPointLayouts.empty(), "Cache not cleared properly");
			ASSERT_EX(m_vertexLayoutMap.empty(), "Cache not cleared properly");
			ASSERT_EX(m_descriptorBindingMap.empty(), "Cache not cleared properly");
        }

		void IBaseObjectCache::clear()
		{
			m_vertexLayoutMap.clearPtr();
			m_descriptorBindingMap.clearPtr();

			m_descriptorBindPointLayouts.clear();
			m_descriptorBindPointMap.clear();
			m_vertexBindPointMap.clear();
		}

        uint16_t IBaseObjectCache::resolveVertexBindPointIndex(base::StringID name)
        {
            uint16_t ret = 0;
            if (m_vertexBindPointMap.find(name, ret))
                return ret;

            ret = m_vertexBindPointMap.size();
            m_vertexBindPointMap[name] = ret;            
            return ret;
        }

        uint16_t IBaseObjectCache::resolveDescriptorBindPointIndex(base::StringID name, DescriptorID layout)
        {
            ASSERT_EX(layout, "Invalid layout used");

            UniqueParamBindPointKey key;
            key.name = name;
            key.layout = layout;

            uint16_t ret = 0;
            if (m_descriptorBindPointMap.find(key, ret))
                return ret;

            ret = m_descriptorBindPointMap.size();
            m_descriptorBindPointMap[key] = ret;
            m_descriptorBindPointLayouts.pushBack(layout);			
            return ret;
        }

        uint16_t IBaseObjectCache::resolveDescriptorBindPointIndex(const rendering::ShaderLibraryData& shaderLib, PipelineIndex descriptorLayoutIndex)
        {
            const auto& paramTable = shaderLib.parameterLayouts()[descriptorLayoutIndex];
            const auto paramTableName = shaderLib.names()[paramTable.name];

            base::InplaceArray<DeviceObjectViewType, 32> viewTypes;
            viewTypes.reserve(paramTable.numElements);

            for (uint32_t i = 0; i < paramTable.numElements; ++i)
            {
                const auto& paramTableElement = shaderLib.parameterLayoutsElements()[shaderLib.indirectIndices()[i + paramTable.firstElementIndex]];
                const auto uav = (paramTableElement.access != ResourceAccess::ReadOnly);
                switch (paramTableElement.type)
                {
                    case ResourceType::Constants:
                        viewTypes.pushBack(DeviceObjectViewType::ConstantBuffer);
                        break;

                    case ResourceType::Buffer: 
                        if (paramTableElement.layout != INVALID_PIPELINE_INDEX)
                            viewTypes.pushBack(uav ? DeviceObjectViewType::BufferStructuredWritable : DeviceObjectViewType::BufferStructured);
                        else
                            viewTypes.pushBack(uav ? DeviceObjectViewType::BufferWritable : DeviceObjectViewType::Buffer);
                        break;

                    case ResourceType::Texture: 
                        viewTypes.pushBack(uav ? DeviceObjectViewType::ImageWritable : DeviceObjectViewType::Image);
                        break;

                    default: DEBUG_CHECK(!"Invalid resource type");
                }
            }

            auto layoutId = DescriptorID::FromTypes(viewTypes.typedData(), viewTypes.size());
            return resolveDescriptorBindPointIndex(paramTableName, layoutId);
        }

        IBaseVertexBindingLayout* IBaseObjectCache::resolveVertexBindingLayout(const rendering::ShaderLibraryData& shaderLib, PipelineIndex vertexInputStateIndex)
        {
            DEBUG_CHECK(vertexInputStateIndex != INVALID_PIPELINE_INDEX);
            const auto& srcBinding = shaderLib.vertexInputStates()[vertexInputStateIndex];

            // use cached one
			{
				IBaseVertexBindingLayout* ret = nullptr;
				if (m_vertexLayoutMap.find(srcBinding.structureKey, ret))
					return ret;
			}

            // create the return wrapper
			base::Array<IBaseVertexBindingLayout::BindingInfo> vertexBindPoints;
			vertexBindPoints.reserve(srcBinding.numStreamLayouts);

            // create the attribute mapping from all active streams
            for (uint32_t i = 0; i < srcBinding.numStreamLayouts; ++i)
            {
                auto vertexLayoutIndex = shaderLib.indirectIndices()[srcBinding.firstStreamLayout + i];
                DEBUG_CHECK_EX(vertexLayoutIndex != INVALID_PIPELINE_INDEX, "Invalid layout index");

                // get the data layout description for the vertex layout
                auto& vertexLayout = shaderLib.vertexInputLayouts()[vertexLayoutIndex];
                auto& dataStructure = shaderLib.dataLayoutStructures()[vertexLayout.structureIndex];

                // get name of this binding point
                auto& bindingPointInfo = vertexBindPoints.emplaceBack();
                bindingPointInfo.name = shaderLib.names()[vertexLayout.name];
                bindingPointInfo.stride = vertexLayout.customStride ? vertexLayout.customStride : dataStructure.size;
                bindingPointInfo.bindPointIndex = resolveVertexBindPointIndex(bindingPointInfo.name);
                bindingPointInfo.instanced = vertexLayout.instanced;

                // is this instanced stream ?
                auto isInstanceData = vertexLayout.instanced;

                // setup the vertex attributes as in the layout
                for (uint32_t j = 0; j < dataStructure.numElements; ++j)
                {
                    auto structureElementIndex = shaderLib.indirectIndices()[dataStructure.firstElementIndex + j];
                    auto& dataElement = shaderLib.dataLayoutElements()[structureElementIndex];

					auto& bindingAttribute = bindingPointInfo.attributes.emplaceBack();
					bindingAttribute.format = dataElement.format;
					bindingAttribute.offset = dataElement.offset;
                }
            }

			// create entry
			if (auto* ret = createOptimalVertexBindingLayout(std::move(vertexBindPoints)))
			{
				m_vertexLayoutMap[srcBinding.structureKey] = ret;
				return ret;
			}

			// rare case when something can't be created
			DEBUG_CHECK(!"Unable to create vertex layout binding state");
            return nullptr;
        }

        IBaseDescriptorBindingLayout* IBaseObjectCache::resolveDescriptorBindingLayout(const rendering::ShaderLibraryData& shaderLib, PipelineIndex parameterBindingStateIndex)
        {
            DEBUG_CHECK(parameterBindingStateIndex != INVALID_PIPELINE_INDEX);
            const auto& bindingSetup = shaderLib.parameterBindingStates()[parameterBindingStateIndex];
            
            // already cached ?
			{
				IBaseDescriptorBindingLayout* ret = nullptr;
				if (m_descriptorBindingMap.find(bindingSetup.structureKey, ret))
					return ret;
			}

            // create new state
			base::Array<DescriptorBindingElement> bindingElements;
            for (uint32_t i = 0; i < bindingSetup.numParameterLayoutIndices; ++i)
            {
                const auto& parameterLayoutIndex = shaderLib.indirectIndices()[i + bindingSetup.firstParameterLayoutIndex];
                const auto parameterLayoutBindingIndex = resolveDescriptorBindPointIndex(shaderLib, parameterLayoutIndex);

                const auto& parameterLayoutData = shaderLib.parameterLayouts()[parameterLayoutIndex];
                const auto parameterLayoutName = shaderLib.names()[parameterLayoutData.name];

                for (uint32_t j = 0; j < parameterLayoutData.numElements; ++j)
                {
                    const auto& parameterElementIndex = shaderLib.indirectIndices()[j + parameterLayoutData.firstElementIndex];
                    const auto& parameterElementData = shaderLib.parameterLayoutsElements()[parameterElementIndex];
                    const auto parameterElementName = shaderLib.names()[parameterElementData.name];

                    auto& bindingElement = bindingElements.emplaceBack();
                    bindingElement.bindPointName = parameterLayoutName;
                    bindingElement.bindPointLayout = m_descriptorBindPointLayouts[parameterLayoutBindingIndex];
                    bindingElement.paramName = parameterElementName;
                    bindingElement.bindPointIndex = parameterLayoutBindingIndex;
                    bindingElement.descriptorElementIndex = j;
                    bindingElement.writable = (parameterElementData.access != ResourceAccess::ReadOnly);
                    bindingElement.objectFormat = parameterElementData.format;

                    switch (parameterElementData.type)
                    {
                        case rendering::ResourceType::Constants:
                        {
                            bindingElement.objectType = DeviceObjectViewType::ConstantBuffer;
                            break;
                        }

                        case rendering::ResourceType::Texture:
                        {
                            if (parameterElementData.access == rendering::ResourceAccess::ReadOnly)
                                bindingElement.objectType = DeviceObjectViewType::Image;
                            else
                                bindingElement.objectType = DeviceObjectViewType::ImageWritable;
                            break;
                        }

                        case rendering::ResourceType::Buffer:
                        {
                            if (parameterElementData.layout != INVALID_PIPELINE_INDEX)
                            {
                                if (parameterElementData.access == rendering::ResourceAccess::ReadOnly)
                                    bindingElement.objectType = DeviceObjectViewType::BufferStructured;
                                else
                                    bindingElement.objectType = DeviceObjectViewType::BufferStructuredWritable;
                            }
                            else
                            {
                                if (parameterElementData.access == rendering::ResourceAccess::ReadOnly)
                                    bindingElement.objectType = DeviceObjectViewType::Buffer;
                                else
                                    bindingElement.objectType = DeviceObjectViewType::BufferWritable;
                            }

                            break;
                        }
                        
                        default:
                            DEBUG_CHECK(!"Rendering: Invalid resource type");
                    }
                }   
            }

			// create entry
			if (auto* ret = createOptimalDescriptorBindingLayout(std::move(bindingElements)))
			{
				m_descriptorBindingMap[bindingSetup.structureKey] = ret;
				return ret;
			}

			// rare case when something can't be created
			DEBUG_CHECK(!"Unable to create descrwiptor layout binding state");
			return nullptr;
        }
		
        //---

    } // gl4
} // rendering