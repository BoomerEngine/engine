/***a
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: format #]
***/

#include "build.h"
#include "wavefrontFormatMTL.h"

#include "base/io/include/utils.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/containers/include/stringParser.h"
#include "base/containers/include/hashSet.h"
#include "base/containers/include/stringBuilder.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"

namespace wavefront
{

    //---

    RTTI_BEGIN_TYPE_STRUCT(MaterialMap);
        RTTI_PROPERTY(m_path);
        RTTI_PROPERTY(m_blendU);
        RTTI_PROPERTY(m_blendV);
        RTTI_PROPERTY(m_clamp);
        RTTI_PROPERTY(m_bumpMultiplier);
        RTTI_PROPERTY(m_lodBias);
        RTTI_PROPERTY(m_colorCorrection);
        RTTI_PROPERTY(m_scalarChannel);
        RTTI_PROPERTY(m_valueOffset);
        RTTI_PROPERTY(m_valueGain);
        RTTI_PROPERTY(m_uvOffset);
        RTTI_PROPERTY(m_uvScale);
    RTTI_END_TYPE();
        
    //---

    RTTI_BEGIN_TYPE_STRUCT(Material);
        RTTI_PROPERTY(m_name);
        RTTI_PROPERTY(m_colorAmbient);
        RTTI_PROPERTY(m_colorDiffuse);
        RTTI_PROPERTY(m_colorSpecular);
        RTTI_PROPERTY(m_colorTransmission);
        RTTI_PROPERTY(m_colorEmissive);
        RTTI_PROPERTY(m_mapAmbient);
        RTTI_PROPERTY(m_mapDiffuse);
        RTTI_PROPERTY(m_mapSpecular);
        RTTI_PROPERTY(m_mapDissolve);
        RTTI_PROPERTY(m_mapEmissive);
        RTTI_PROPERTY(m_mapBump);
        RTTI_PROPERTY(m_mapNormal);
        RTTI_PROPERTY(m_illumMode);
        RTTI_PROPERTY(m_dissolveCenter);
        RTTI_PROPERTY(m_dissolveHalo);
        RTTI_PROPERTY(m_specularExp);
        RTTI_PROPERTY(m_sharpness);
        RTTI_PROPERTY(m_opticalDensity);
    RTTI_END_TYPE();
    
    //---

    RTTI_BEGIN_TYPE_CLASS(FormatMTL);
        RTTI_PROPERTY(m_materials);
    RTTI_END_TYPE();

    FormatMTL::FormatMTL()
    {
    }

    FormatMTL::FormatMTL(const base::Array<Material>& materials)
        : m_materials(materials)
    {
        buildMaterialMap();
    }

    const Material* FormatMTL::findMaterial(base::StringView<char> name) const
    {
        uint32_t index = 0;
        if (m_materialMap.find(name, index))
            return &m_materials[index];

        return nullptr;
    }

    void FormatMTL::buildMaterialMap()
    {
        m_materialMap.reserve(m_materials.size());
        for (uint32_t i=0; i<m_materials.size(); ++i)
        {
            auto& mat = m_materials[i];
            if (!mat.m_name.empty())
                m_materialMap.set(mat.m_name, i);
        }
    }
    
    void FormatMTL::onPostLoad()
    {
        TBaseClass::onPostLoad();
        buildMaterialMap();
    }

    ///--

    /// loader of the MTL file
    class FormatMTLResourceLoader : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(FormatMTLResourceLoader, base::res::IResourceCooker);

    public:
        virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override
        {
            // load the file to buffer
            auto filePath = cooker.queryResourcePath().path();
            auto fileData = cooker.loadToBuffer(filePath);
            if (!fileData)
            {
                TRACE_ERROR("Failed to load content of '{}'", filePath);
                return nullptr;
            }

            // parse data
            return LoadMaterials(cooker.queryResourceContextName(), fileData.data(), fileData.size());
        }
    };

    RTTI_BEGIN_TYPE_CLASS(FormatMTLResourceLoader);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<wavefront::FormatMTL>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("mtl");
    RTTI_END_TYPE();

    ///--

} // wavefront
