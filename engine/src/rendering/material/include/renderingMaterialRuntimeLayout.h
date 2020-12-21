/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "rendering/device/include/renderingManagedBuffer.h"
#include "rendering/device/include/renderingManagedBufferWithAllocator.h"
#include "rendering/device/include/renderingDescriptorID.h"

#include "base/app/include/localService.h"
#include "base/containers/include/blockPool.h"
#include "base/containers/include/staticStructurePool.h"

namespace rendering
{

    //---

    /// parameter type, unified and simplified to an enum
    enum class MaterialDataLayoutParameterType : uint8_t
    {
        Float,
        Vector2,
        Vector3,
        Vector4,
        Color,
        Texture2D,
    };

    /// entry in the material data layout
    struct MaterialDataLayoutEntry
    {
        MaterialDataLayoutParameterType type;
        base::StringID name;
    };


    /// entry in the constant buffer
    struct MaterialDataLayoutConstantBufferEntry
    {
        MaterialDataLayoutParameterType type;
        base::StringID name;
        uint32_t dataSize = 0;
        uint32_t dataOffset = 0;
    };

    /// entry in the constant buffer
    struct MaterialDataLayoutDescriptorResourceEntry
    {
        MaterialDataLayoutParameterType type;
        base::StringID name;
        ImageViewType viewType;
        uint32_t descriptorEntryIndex = 0;
    };

    /// layout information for standard binding
    struct MaterialDataLayoutDescriptor
    {
        base::Array<MaterialDataLayoutConstantBufferEntry> constantBufferEntries;
        base::Array<MaterialDataLayoutDescriptorResourceEntry> resourceEntries;
        uint32_t constantDataSize = 0;
        uint32_t descriptorSize = 0;

		base::StringID descriptorName;
		DescriptorID descriptorID;

        INLINE bool empty() const { return constantDataSize == 0 && descriptorSize == 0; }
    };

	/// layout information for bindless binding
	struct MaterialDataLayoutBindless
	{
		base::Array<MaterialDataLayoutConstantBufferEntry> constantBufferEntries;
		uint32_t constantDataSize = 0;

		INLINE bool empty() const { return constantDataSize == 0; }
	};

    //--

    // layout ID for material template (a descriptor prototype)
    // NOTE: material parameters are allocated with specific layout only if it changes (ie. template is changed) we have to allocate new parameter block....
    // NOTE: we are technically NOT interested in parameter names here but in practice it's best to keep, only the types and order as it's the only thing that influences the packing
    class RENDERING_MATERIAL_API MaterialDataLayout : public base::IReferencable
    {
    public:
        MaterialDataLayout(MaterialDataLayoutID id, base::Array<MaterialDataLayoutEntry>&& entries);

		//--

        // unique ID of the layout, NOTE: same order of entries will generate same layout ID
        INLINE MaterialDataLayoutID id() const { return m_id; }

		// get the layout entries
		INLINE const base::Array<MaterialDataLayoutEntry>& entries() const { return m_entries; }

        // get descriptor for discrete resource binding
        INLINE const MaterialDataLayoutDescriptor& discreteDataLayout() const { return m_discreteDataLayout; }

		// get descriptor for bindless resource binding
		INLINE const MaterialDataLayoutBindless& bindlessDataLayout() const { return m_bindlessDataLayout; }

		//--

    private:
        base::Array<MaterialDataLayoutEntry> m_entries;
        MaterialDataLayoutID m_id;

		MaterialDataLayoutDescriptor m_discreteDataLayout;
		MaterialDataLayoutBindless m_bindlessDataLayout;

        static void BuildDescriptorLayout(const base::Array<MaterialDataLayoutEntry>& entries, MaterialDataLayoutDescriptor& outLayout);
		static void BuildBindlessLayout(const base::Array<MaterialDataLayoutEntry>& entries, MaterialDataLayoutBindless& outLayout);
    };

    //---

} // rendering