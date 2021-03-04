/***a
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: format #]
***/

#include "build.h"
#include "fileMtl.h"

#include "core/io/include/io.h"
#include "core/io/include/fileHandle.h"
#include "core/containers/include/stringParser.h"
#include "core/containers/include/hashSet.h"
#include "core/containers/include/stringBuilder.h"
#include "core/resource/include/resource.h"
#include "core/resource_compiler/include/importSourceAsset.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

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

RTTI_BEGIN_TYPE_CLASS(SourceAssetMTL);
    RTTI_PROPERTY(m_materials);
RTTI_END_TYPE();

SourceAssetMTL::SourceAssetMTL()
{
}

SourceAssetMTL::SourceAssetMTL(const Array<Material>& materials)
    : m_materials(materials)
{
    buildMaterialMap();
}

const Material* SourceAssetMTL::findMaterial(StringView name) const
{
    uint32_t index = 0;
    if (m_materialMap.find(name, index))
        return &m_materials[index];

    return nullptr;
}

void SourceAssetMTL::buildMaterialMap()
{
    m_materialMap.reserve(m_materials.size());
    for (uint32_t i=0; i<m_materials.size(); ++i)
    {
        auto& mat = m_materials[i];
        if (!mat.m_name.empty())
            m_materialMap.set(mat.m_name, i);
    }
}
    
uint64_t SourceAssetMTL::calcMemoryUsage() const
{
    return m_materials.dataSize();
}

void SourceAssetMTL::onPostLoad()
{
    TBaseClass::onPostLoad();
    buildMaterialMap();
}

///--

/// source asset loader for OBJ data
class SourceAssetMTLAssetLoader : public ISourceAssetLoader
{
    RTTI_DECLARE_VIRTUAL_CLASS(SourceAssetMTLAssetLoader, ISourceAssetLoader);

public:
    virtual SourceAssetPtr loadFromMemory(StringView importPath, StringView contextPath, Buffer data) const override
    {
        return LoadMaterials(contextPath, data.data(), data.size());
    }
};

RTTI_BEGIN_TYPE_CLASS(SourceAssetMTLAssetLoader);
    RTTI_METADATA(ResourceSourceFormatMetadata).addSourceExtensions("mtl");
RTTI_END_TYPE();

///--

END_BOOMER_NAMESPACE_EX(assets)
