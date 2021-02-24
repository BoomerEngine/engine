/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiObjectCache.h"

#include "rendering/device/include/renderingDescriptor.h"

BEGIN_BOOMER_NAMESPACE(rendering::api)

//---

IBaseVertexBindingLayout::IBaseVertexBindingLayout(IBaseObjectCache* cache, const base::Array<ShaderVertexStreamMetadata>& streams)
	: m_elements(streams)
{
	for (auto& element : m_elements)
		element.index = cache->resolveVertexBindPointIndex(element.name);
}

IBaseVertexBindingLayout::~IBaseVertexBindingLayout()
{}

void IBaseVertexBindingLayout::print(base::IFormatStream& f) const
{
	f.appendf("{} vertex streams:\n", m_elements.size());
	for (const auto& v : m_elements)
		f.appendf("{}\n", v);
}

//---

IBaseDescriptorBindingLayout::IBaseDescriptorBindingLayout(const base::Array<ShaderDescriptorMetadata>& descriptors)
	: m_descriptors(descriptors)
{}

IBaseDescriptorBindingLayout::~IBaseDescriptorBindingLayout()
{}

void IBaseDescriptorBindingLayout::print(base::IFormatStream& f) const
{
	f.appendf("{} binding elements (API: {})\n", m_descriptors.size());

	for (auto i : m_descriptors.indexRange())
		f.appendf("  Binding [{}]: {}\n", i, m_descriptors[i]);
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

IBaseVertexBindingLayout* IBaseObjectCache::resolveVertexBindingLayout(const ShaderMetadata* data)
{
	//DEBUG_CHECK_RETURN_EX_V(!data->vertexStreams.empty(), "No vertex bindings in shader, compute shader?", nullptr);
	DEBUG_CHECK_RETURN_EX_V(data->vertexLayoutKey != 0 || data->vertexStreams.empty(), "Invalid vertex layout key", nullptr);

    // use cached one
	IBaseVertexBindingLayout* ret = nullptr;
	if (m_vertexLayoutMap.find(data->vertexLayoutKey, ret))
		return ret;
			
	// create entry
	if (auto* ret = createOptimalVertexBindingLayout(data->vertexStreams))
	{
		m_vertexLayoutMap[data->vertexLayoutKey] = ret;
		return ret;
	}

	// rare case when something can't be created
	DEBUG_CHECK(!"Unable to create vertex layout binding state");
    return nullptr;
}

IBaseDescriptorBindingLayout* IBaseObjectCache::resolveDescriptorBindingLayout(const ShaderMetadata* metadata)
{
	//DEBUG_CHECK_RETURN_EX_V(!data->vertexStreams.empty(), "No vertex bindings in shader, compute shader?", nullptr);
	DEBUG_CHECK_RETURN_EX_V(metadata->descriptorLayoutKey, "Invalid vertex layout key", nullptr);

	// use cached one
	IBaseDescriptorBindingLayout* ret = nullptr;
	if (m_descriptorBindingMap.find(metadata->descriptorLayoutKey, ret))
		return ret;

	// create entry
	if (auto* ret = createOptimalDescriptorBindingLayout(metadata->descriptors, metadata->staticSamplers))
	{
		m_descriptorBindingMap[metadata->descriptorLayoutKey] = ret;
		return ret;
	}

	// rare case when something can't be created
	DEBUG_CHECK(!"Unable to create descrwiptor layout binding state");
	return nullptr;
}
		
//---

END_BOOMER_NAMESPACE(rendering::api)