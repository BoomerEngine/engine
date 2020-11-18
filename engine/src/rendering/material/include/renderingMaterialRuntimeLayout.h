/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "rendering/device/include/renderingParametersLayoutID.h"
#include "rendering/device/include/renderingBufferView.h"
#include "rendering/device/include/renderingImageView.h"
#include "rendering/device/include/renderingManagedBuffer.h"
#include "rendering/device/include/renderingManagedBufferWithAllocator.h"

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
        uint32_t descriptorDataOffset = 0;
    };

    /// entry in the constant buffer
    struct MaterialDataLayoutDescriptor
    {
        base::Array<MaterialDataLayoutConstantBufferEntry> constantBufferEntries;
        base::Array< MaterialDataLayoutDescriptorResourceEntry> resourceEntries;
        uint32_t constantDataSize = 0;
        uint32_t descriptorSize = 0;
        ParametersLayoutID layoutId;

        INLINE bool empty() const { return constantDataSize == 0 && descriptorSize == 0; }
    };

    //--

    // layout ID for material template (a descriptor prototype)
    // NOTE: material parameters are allocated with specific layout only if it changes (ie. template is changed) we have to allocate new parameter block....
    // NOTE: we are technically NOT interested in parameter names here but in practice it's best to keep, only the types and order as it's the only thing that influences the packing
    class RENDERING_MATERIAL_API MaterialDataLayout : public base::IReferencable
    {
    public:
        MaterialDataLayout(MaterialDataLayoutID id, uint64_t key, base::Array<MaterialDataLayoutEntry>&& entries);

        // unique ID of the layout, NOTE: same entry layout will generate same layout ID
        INLINE MaterialDataLayoutID id() const { return m_id; }

        // unique descriptor name that matches this layout
        INLINE const base::StringID descriptorName() const { return m_descriptorName; }

        // get the layout entries
        INLINE const base::Array<MaterialDataLayoutEntry>& entries() const { return m_entries; }

        // get descriptor for direct binding
        INLINE const MaterialDataLayoutDescriptor& descriptorLayout() const { return m_descriptor; }

        //--

        // print layout
        void print(const base::IFormatStream& f) const;

    private:
        base::Array<MaterialDataLayoutEntry> m_entries;
        base::StringID m_descriptorName;
        MaterialDataLayoutID m_id;

        MaterialDataLayoutDescriptor m_descriptor;

        void buildDescriptorLayout();
    };

    //---

} // rendering