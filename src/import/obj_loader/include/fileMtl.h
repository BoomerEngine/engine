/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: format #]
***/

#pragma once

#include "core/containers/include/hashMap.h"
#include "core/resource/include/resource.h"
#include "core/resource_compiler/include/importSourceAsset.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

/// MTL material map
struct IMPORT_OBJ_LOADER_API MaterialMap
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialMap);

public:
    StringBuf m_path; // relative to source asset file

    bool m_blendU = true;
    bool m_blendV = true;
    bool m_clamp = false;
    float m_bumpMultiplier = 1.0f;
    float m_lodBias = 0.0f;
    bool m_colorCorrection = 1.0f;
    char m_scalarChannel = 0;
    float m_valueOffset = 0.0f;
    float m_valueGain = 1.0f;
    Vector3 m_uvOffset = Vector3::ZERO();
    Vector3 m_uvScale = Vector3::ONE();
};

/// MTL material
struct IMPORT_OBJ_LOADER_API Material
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Material);

public:
    StringBuf m_name;

    Color m_colorAmbient = Color::WHITE;
    Color m_colorDiffuse = Color::WHITE;
    Color m_colorSpecular = Color::WHITE;
    Color m_colorEmissive = Color::WHITE;
    Color m_colorTransmission = Color::WHITE;

    MaterialMap m_mapAmbient;
    MaterialMap m_mapDiffuse;
    MaterialMap m_mapSpecular;
    MaterialMap m_mapRoughness;
    MaterialMap m_mapRoughnessSpecularity;
    MaterialMap m_mapMetallic;
    MaterialMap m_mapDissolve;
    MaterialMap m_mapEmissive;
    MaterialMap m_mapBump;
    MaterialMap m_mapNormal;

    int m_illumMode = 0;
    float m_dissolveCenter = 0.0f;
    float m_dissolveHalo = 0.0f;

    float m_specularExp = 10.0f;
    float m_sharpness = 60.0f;
    float m_opticalDensity = 1.5f;
};

/// unpacked format data for MTL file
/// can be stored in the import cache to speed up reimports from the same file
class IMPORT_OBJ_LOADER_API SourceAssetMTL : public ISourceAsset
{
    RTTI_DECLARE_VIRTUAL_CLASS(SourceAssetMTL, ISourceAsset);

public:
    SourceAssetMTL();
    SourceAssetMTL(const Array<Material>& materials);

    /// all of the defined materials
    INLINE const Array<Material>& materials() const { return m_materials; }

    //--

    const Material* findMaterial(StringView name) const;

    //--

private:
    Array<Material> m_materials;
    HashMap<StringBuf, uint32_t> m_materialMap;

    static bool ParseColor(StringParser& str, Color& outColor);
    static bool ParseMaterial(StringParser& str, Material& outMat);
    static bool ParseMaterialMap(StringParser& str, MaterialMap& outMap);

    //--

    virtual uint64_t calcMemoryUsage() const override;

    virtual void onPostLoad() override;
    void buildMaterialMap();
};

//--

/// load from object file
extern IMPORT_OBJ_LOADER_API SourceAssetMTLPtr LoadMaterials(StringView contextName, const void* data, uint64_t dataSize);

//--

END_BOOMER_NAMESPACE_EX(assets)
