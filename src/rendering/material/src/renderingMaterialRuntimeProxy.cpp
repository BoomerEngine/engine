/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "renderingMaterialRuntimeProxy.h"
#include "renderingMaterialRuntimeTechnique.h"
#include "renderingMaterialRuntimeTemplate.h"
#include "renderingMaterialTemplate.h"

#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/texture/include/renderingTexture.h" 
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingDeviceGlobalObjects.h"
#include "rendering/device/include/renderingImage.h"

namespace rendering
{
	///---

	MaterialDataDescriptor::~MaterialDataDescriptor()
	{
		base::mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Free(constantData);
		constantData = nullptr;

		base::mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Free(descriptorData);
		descriptorData = nullptr;
	}

	MaterialDataDescriptor* MaterialDataDescriptor::Create(const MaterialDataLayoutDescriptor& layout, const IMaterial& source)
	{
		auto* ret = new MaterialDataDescriptor();

		ret->descriptorName = layout.descriptorName;

		ret->constantDataSize = layout.constantDataSize;
		ret->constantData = base::mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::AllocN(layout.constantDataSize, 16);
        memzero(ret->constantData, ret->constantDataSize);

		ret->descriptorCount = layout.descriptorSize;
		ret->descriptorData = base::mem::GlobalPool<POOL_MATERIAL_DATA, DescriptorEntry>::AllocN(layout.descriptorSize, 4);
		memzero(ret->descriptorData, ret->descriptorCount * sizeof(DescriptorEntry));

		if (layout.constantDataSize > 0)
		{
			ret->descriptorData[0].constants(ret->constantData, ret->constantDataSize);

			for (const auto& entry : layout.constantBufferEntries)
			{
				auto* targetDataPtr = ret->constantData + entry.dataOffset;
				switch (entry.type)
				{
					case MaterialDataLayoutParameterType::Float:
						source.readParameter(entry.name, targetDataPtr, base::reflection::GetTypeObject<float>());
						break;

					case MaterialDataLayoutParameterType::Vector2:
						source.readParameter(entry.name, targetDataPtr, base::reflection::GetTypeObject<base::Vector2>());
                        break;

					case MaterialDataLayoutParameterType::Vector3:
						source.readParameter(entry.name, targetDataPtr, base::reflection::GetTypeObject<base::Vector3>());
                        break;

					case MaterialDataLayoutParameterType::Vector4:
                        source.readParameter(entry.name, targetDataPtr, base::reflection::GetTypeObject<base::Vector4>());
                        break;

					case MaterialDataLayoutParameterType::Color:
					{
						base::Color color;
						source.readParameterTyped(entry.name, color);

						auto* writePtr = (float*)targetDataPtr;
						auto linearColor = color.toVectorLinear();
						writePtr[0] = linearColor.x;
						writePtr[1] = linearColor.y;
						writePtr[2] = linearColor.z;
						writePtr[3] = linearColor.w;
						break;
					}
				}				
			}
		}
			
		for (const auto& entry : layout.resourceEntries)
		{
			if (entry.viewType == ImageViewType::View2D)
			{
				rendering::TextureRef textureRef;
				source.readParameterTyped(entry.name, textureRef);

				auto texture = textureRef.load();
				auto textureView = texture ? texture->view() : nullptr;

				if (!textureView)
					textureView = Globals().TextureWhite;

				ret->descriptorData[entry.descriptorEntryIndex] = textureView.get();

			}
			else
			{
				ASSERT(!"Unsupported resouce type");
			}
		}

		return ret;
	}

	//--

	MaterialDataBindless::~MaterialDataBindless()
	{
		base::mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Free(constantData);
		constantData = nullptr;
	}

	MaterialDataBindless* MaterialDataBindless::Create(const MaterialDataLayoutBindless& layout, const IMaterial& source)
	{
		auto* ret = new MaterialDataBindless();

		ret->constantDataSize = layout.constantDataSize;
		ret->constantData = base::mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Alloc(layout.constantDataSize, 16);

		return ret;
	}

    ///---

    MaterialDataProxy::MaterialDataProxy(const MaterialTemplateProxy* materialTemplate)
        : m_layout(materialTemplate->layout())
        , m_metadata(materialTemplate->metadata())
        , m_template(AddRef(materialTemplate))
    {
    }

    MaterialDataProxy::~MaterialDataProxy()
    {
		delete m_bindlessData;
		m_bindlessData = nullptr;

		delete m_descriptorData;
		m_descriptorData = nullptr;
    }

	void MaterialDataProxy::clanupOldData()
	{
		m_prevDataLock.acquire();
		auto oldBindless = std::move(m_prevBindlessData);
		auto oldDescriptors = std::move(m_prevDescriptorData);
		m_prevDataLock.release();

		oldBindless.clearPtr();
		oldDescriptors.clearPtr();
	}

	void MaterialDataProxy::bind(command::CommandWriter& cmd) const
	{
		auto* data = m_descriptorData.load();
		if (data->descriptorName)
		{
			DEBUG_CHECK_RETURN_EX(data->descriptorCount != 0, "No descirptors");
			cmd.opBindDescriptorEntries(data->descriptorName, data->descriptorData, data->descriptorCount);
		}
	}

	void MaterialDataProxy::writeBindlessData(void* ptr) const
	{

	}

    void MaterialDataProxy::update(const IMaterial& dataSource)
    {
		{
			auto bindlessData = MaterialDataBindless::Create(m_template->layout()->bindlessDataLayout(), dataSource);
			if (auto prevBindlessData = m_bindlessData.exchange(bindlessData))
				m_prevBindlessData.pushBack(prevBindlessData);
		}

		{
			auto descriptorData = MaterialDataDescriptor::Create(m_template->layout()->discreteDataLayout(), dataSource);
			if (auto prevDescriptorData = m_descriptorData.exchange(descriptorData))
				m_prevDescriptorData.pushBack(prevDescriptorData);
		}
    }

    ///---

} // rendering