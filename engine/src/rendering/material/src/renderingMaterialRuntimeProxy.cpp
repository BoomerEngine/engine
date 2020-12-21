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
#include "../../device/include/renderingDescriptor.h"

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

		ret->constantDataSize = layout.constantDataSize;
		ret->constantData = base::mem::GlobalPool<POOL_MATERIAL_DATA, uint8_t>::AllocN(layout.constantDataSize, 16);
		ret->descriptorCount = layout.descriptorSize;
		ret->descriptorData = base::mem::GlobalPool<POOL_MATERIAL_DATA, DescriptorEntry>::AllocN(layout.descriptorSize, 4);

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
        , m_sortGroup(materialTemplate->sortGroup())
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

	void MaterialDataProxy::bindDescriptor(command::CommandWriter& cmd) const
	{

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