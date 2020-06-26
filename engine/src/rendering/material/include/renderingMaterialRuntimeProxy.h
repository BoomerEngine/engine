/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: runtime #]
***/

#pragma once

#include "rendering/driver/include/renderingImageView.h"
#include "rendering/driver/include/renderingParametersView.h"
#include "renderingMaterialRuntimeLayout.h"

namespace rendering
{
    //---

    // proxy for material's rendering data
    class RENDERING_MATERIAL_API MaterialDataProxy : public base::IReferencable
    {
    public:
        MaterialDataProxy(const MaterialTemplate* materialTemplate, bool keepRef, const IMaterial& dataSource);
        ~MaterialDataProxy();

        //--

        // get internal ID - note that IDs maybe reused
        INLINE MaterialDataProxyID id() const { return m_id; }

        // sort group for this material proxy, NOTE: never updated, a new proxy must be created as if we changed the template (usually the template DOES change)
        INLINE MaterialSortGroup sortGroup() const { return m_sortGroup; }

        // data layout for the material data proxy
        INLINE const MaterialDataLayout* layout() const { return m_layout; };

        // get the material template used as a base
        INLINE const MaterialTemplateWeakPtr& materialTemplate() const { return m_materialTemplate; }

        //--

        // update data in data proxy
        void update(const IMaterial& dataSource);

        // upload descriptor to recorded command buffer
        ParametersView upload(command::CommandWriter& cmd) const;

        //--

    private:
        MaterialDataProxyID m_id;
        MaterialSortGroup m_sortGroup = MaterialSortGroup::Opaque;
        const MaterialDataLayout* m_layout = nullptr;

        MaterialTemplateWeakPtr m_materialTemplate;
        MaterialTemplatePtr m_materialTemplateOwnedRef;

        base::SpinLock m_constantDataLock;
        uint8_t* m_constantData = nullptr; // TODO: double buffer if needed
        uint32_t m_constantDataSize = 0;
        uint8_t* m_descriptorData = nullptr; // resource views
        uint32_t m_descriptorDataSize = 0;

        ParametersLayoutID m_descriptorLayoutID;

        base::Array<base::res::ResourcePtr> m_usedResources;
    };

    //---

} // rendering