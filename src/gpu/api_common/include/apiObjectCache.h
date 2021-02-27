/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/device/include/descriptorID.h"
#include "gpu/device/include/shaderMetadata.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//---

// vertex layout/binding states, owned by cache since they repeat a lot
class GPU_API_COMMON_API IBaseVertexBindingLayout : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_API_PIPELINES)

public:
	IBaseVertexBindingLayout(IBaseObjectCache* cache, const Array<ShaderVertexStreamMetadata>& streams);
	virtual ~IBaseVertexBindingLayout();

	INLINE const Array<ShaderVertexStreamMetadata>& vertexStreams() const { return m_elements; }

	void print(IFormatStream& f) const;

protected:
	Array<ShaderVertexStreamMetadata> m_elements;
};

//--

// collection of descriptors that should be bound together
// represents Root Signature / Pipeline Layout 
class GPU_API_COMMON_API IBaseDescriptorBindingLayout : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_API_PIPELINES)

public:
	IBaseDescriptorBindingLayout(const Array<ShaderDescriptorMetadata>& descriptors);
	virtual ~IBaseDescriptorBindingLayout();

	INLINE const Array<ShaderDescriptorMetadata>& descriptors() const { return m_descriptors; }

	void print(IFormatStream& f) const;

private:
	Array<ShaderDescriptorMetadata> m_descriptors;
};

//--

struct UniqueParamBindPointKey
{
    StringID name;
    DescriptorID layout;

	INLINE UniqueParamBindPointKey() {};

    INLINE static uint32_t CalcHash(const UniqueParamBindPointKey& key) { return CRC32() << key.name << key.layout.value(); }

    INLINE bool operator==(const UniqueParamBindPointKey& other) const { return (name == other.name) && (layout == other.layout); }
    INLINE bool operator!=(const UniqueParamBindPointKey& other) const { return !operator==(other); }
};

//---

class GPU_API_COMMON_API IBaseObjectCache : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_API_PIPELINES)

public:
	IBaseObjectCache(IBaseThread* owner);
    virtual ~IBaseObjectCache();

	//--

	// cache owner
	INLINE IBaseThread* owner() const { return m_owner; }

	//--

	// clear all objects from the cache
	virtual void clear();

	//--

    /// find a bindpoint index by name
    uint16_t resolveVertexBindPointIndex(StringID name);

    /// find a parameter index by name and type
    uint16_t resolveDescriptorBindPointIndex(StringID name, DescriptorID layout);

    /// find/create VBO layout
    IBaseVertexBindingLayout* resolveVertexBindingLayout(const ShaderMetadata* metadata);

	/// resolve the mapping of inputs parameters to actual slots in OpenGL
	IBaseDescriptorBindingLayout* resolveDescriptorBindingLayout(const ShaderMetadata* metadata);

	//--

private:
    IBaseThread* m_owner;

    HashMap<StringID, uint16_t> m_vertexBindPointMap;
    HashMap<UniqueParamBindPointKey, uint16_t> m_descriptorBindPointMap;
    Array<DescriptorID> m_descriptorBindPointLayouts;

    HashMap<uint64_t, IBaseVertexBindingLayout*> m_vertexLayoutMap;
    HashMap<uint64_t, IBaseDescriptorBindingLayout*> m_descriptorBindingMap;

	virtual IBaseVertexBindingLayout* createOptimalVertexBindingLayout(const Array<ShaderVertexStreamMetadata>& streams) = 0;
	virtual IBaseDescriptorBindingLayout* createOptimalDescriptorBindingLayout(const Array<ShaderDescriptorMetadata>& descriptors, const Array<ShaderStaticSamplerMetadata>& staticSamplers) = 0;
};

//---

END_BOOMER_NAMESPACE_EX(gpu::api)
