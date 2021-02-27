/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "runtimeLayout.h"
#include "materialTemplate.h"

BEGIN_BOOMER_NAMESPACE()

//---

/// binding points data (descriptors)
struct MaterialDataDescriptor : public NoCopy
{
	RTTI_DECLARE_POOL(POOL_MATERIAL_DATA);

public:
	~MaterialDataDescriptor();

	uint8_t* constantData = nullptr; // TODO: double buffer if needed
	uint32_t constantDataSize = 0;

	StringID descriptorName;
	gpu::DescriptorID descriptorID;
	gpu::DescriptorEntry* descriptorData = nullptr; // resource views (we reference)
	uint32_t descriptorCount = 0;

	static MaterialDataDescriptor* Create(const MaterialDataLayoutDescriptor& layout, const IMaterial& source);
};

//---

// bindless data (textures are saved as IDs in constant buffer)
struct MaterialDataBindless : public NoCopy
{
	RTTI_DECLARE_POOL(POOL_MATERIAL_DATA);

public:
	~MaterialDataBindless();

	uint8_t* constantData = nullptr; // TODO: double buffer if needed
	uint32_t constantDataSize = 0;

	static MaterialDataBindless* Create(const MaterialDataLayoutBindless& layout, const IMaterial& source);
};

//---

// proxy for material's rendering data
class ENGINE_MATERIAL_API MaterialDataProxy : public IReferencable
{
	RTTI_DECLARE_POOL(POOL_MATERIAL_DATA);

public:
    MaterialDataProxy(const MaterialTemplateProxy* materialTemplate);
    ~MaterialDataProxy();

    //--

    // get internal ID - note that IDs maybe reused
    INLINE MaterialDataProxyID id() const { return m_id; }

    // template proxy we got created for
	INLINE const MaterialTemplateProxy* templateProxy() const { return m_template; }

	// get material template's metadata
	INLINE const MaterialTemplateMetadata& metadata() const { return m_metadata; }

    // data layout for the material data proxy
    INLINE const MaterialDataLayout* layout() const { return m_layout; };

    //--

    // update data in data proxy
    void update(const IMaterial& dataSource);

    //--

	// bind material descriptor
	void bind(gpu::CommandWriter& cmd) const;

	// write bindless data to provided memory
	void writeBindlessData(void* ptr) const;

	//--

private:
    MaterialDataProxyID m_id;

	const MaterialTemplateMetadata m_metadata; // copied from template

    const MaterialDataLayout* m_layout = nullptr;
    MaterialTemplateProxyPtr m_template;

	//--

	std::atomic<MaterialDataBindless*> m_bindlessData = nullptr;
	std::atomic<MaterialDataDescriptor*> m_descriptorData = nullptr;

	//--

	SpinLock m_prevDataLock;
	Array<MaterialDataBindless*> m_prevBindlessData;
	Array<MaterialDataDescriptor*> m_prevDescriptorData;

	void clanupOldData();

	//--
};

//---

END_BOOMER_NAMESPACE()
