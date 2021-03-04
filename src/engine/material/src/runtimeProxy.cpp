/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#include "build.h"
#include "runtimeProxy.h"
#include "runtimeTechnique.h"
#include "runtimeTemplate.h"
#include "materialTemplate.h"

#include "gpu/device/include/commandWriter.h"
#include "engine/texture/include/texture.h"
#include "gpu/device/include/descriptor.h"
#include "gpu/device/include/globalObjects.h"
#include "gpu/device/include/image.h"

BEGIN_BOOMER_NAMESPACE()

///---

MaterialDataDescriptor::~MaterialDataDescriptor()
{
	mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Free(constantData);
	constantData = nullptr;

	mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Free(descriptorData);
	descriptorData = nullptr;
}

MaterialDataDescriptor* MaterialDataDescriptor::Create(const MaterialDataLayoutDescriptor& layout, const IMaterial& source)
{
	auto* ret = new MaterialDataDescriptor();

	ret->descriptorName = layout.descriptorName;

	ret->constantDataSize = layout.constantDataSize;
	ret->constantData = mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::AllocN(layout.constantDataSize, 16);
    memzero(ret->constantData, ret->constantDataSize);

	ret->descriptorCount = layout.descriptorSize;
	ret->descriptorData = mem::GlobalPool<POOL_MATERIAL_DATA, gpu::DescriptorEntry>::AllocN(layout.descriptorSize, 4);
	memzero(ret->descriptorData, ret->descriptorCount * sizeof(gpu::DescriptorEntry));

	if (layout.constantDataSize > 0)
	{
		ret->descriptorData[0].constants(ret->constantData, ret->constantDataSize);

		for (const auto& entry : layout.constantBufferEntries)
		{
			auto* targetDataPtr = ret->constantData + entry.dataOffset;
			switch (entry.type)
			{
				case MaterialDataLayoutParameterType::StaticBool:
					break;

				case MaterialDataLayoutParameterType::Float:
					source.readParameter(entry.name, targetDataPtr, reflection::GetTypeObject<float>());
					break;

				case MaterialDataLayoutParameterType::Vector2:
					source.readParameter(entry.name, targetDataPtr, reflection::GetTypeObject<Vector2>());
                    break;

				case MaterialDataLayoutParameterType::Vector3:
					source.readParameter(entry.name, targetDataPtr, reflection::GetTypeObject<Vector3>());
                    break;

				case MaterialDataLayoutParameterType::Vector4:
                    source.readParameter(entry.name, targetDataPtr, reflection::GetTypeObject<Vector4>());
                    break;

				case MaterialDataLayoutParameterType::Color:
				{
					Color color;
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
			TextureRef textureRef;
			source.readParameterTyped(entry.name, textureRef);

			auto texture = textureRef.load();
			auto textureView = texture ? texture->view() : nullptr;

			if (!textureView)
				textureView = gpu::Globals().TextureWhite;

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
	mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Free(constantData);
	constantData = nullptr;
}

MaterialDataBindless* MaterialDataBindless::Create(const MaterialDataLayoutBindless& layout, const IMaterial& source)
{
	auto* ret = new MaterialDataBindless();

	ret->constantDataSize = layout.constantDataSize;
	ret->constantData = mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::Alloc(layout.constantDataSize, 16);

	return ret;
}

///---

MaterialDataProxy::MaterialDataProxy(const MaterialTemplateProxy* materialTemplate)
    : m_layout(materialTemplate->layout())
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

void MaterialDataProxy::bind(gpu::CommandWriter& cmd, uint32_t& outStaticSwitchMask) const
{
	auto lock = CreateLock(m_lock);

	if (m_descriptorData && m_descriptorData->descriptorName)
	{
		DEBUG_CHECK_RETURN_EX(m_descriptorData->descriptorCount != 0, "No descirptors");
		cmd.opBindDescriptorEntries(m_descriptorData->descriptorName, m_descriptorData->descriptorData, m_descriptorData->descriptorCount);
	}

	outStaticSwitchMask = m_staticSwitchMask;
}

void MaterialDataProxy::writeBindlessData(void* ptr) const
{

}

void MaterialDataProxy::update(const IMaterial& dataSource)
{
	auto lock = CreateLock(m_lock);

	m_template->evalRenderStates(dataSource, m_renderStates);

	m_staticSwitchMask = m_template->layout()->compileStaticSwitchMask(dataSource);
	m_descriptorData = MaterialDataDescriptor::Create(m_template->layout()->discreteDataLayout(), dataSource);
	m_bindlessData = MaterialDataBindless::Create(m_template->layout()->bindlessDataLayout(), dataSource);
}

///---

END_BOOMER_NAMESPACE()
