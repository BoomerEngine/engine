/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: format #]
***/

#pragma once

#include "base/containers/include/hashMap.h"
#include "base/resource/include/resource.h"
#include "base/resource_compiler/include/importSourceAsset.h"

namespace wavefront
{

    /// MTL material map
    struct ASSETS_OBJ_LOADER_API MaterialMap
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialMap);

    public:
        base::StringBuf m_path; // relative to source asset file

        bool m_blendU = true;
        bool m_blendV = true;
        bool m_clamp = false;
        float m_bumpMultiplier = 1.0f;
        float m_lodBias = 0.0f;
        bool m_colorCorrection = 1.0f;
        char m_scalarChannel = 0;
        float m_valueOffset = 0.0f;
        float m_valueGain = 1.0f;
        base::Vector3 m_uvOffset = base::Vector3::ZERO();
        base::Vector3 m_uvScale = base::Vector3::ONE();
    };

    /// MTL material
    struct ASSETS_OBJ_LOADER_API Material
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(Material);

    public:
        base::StringBuf m_name;

        base::Color m_colorAmbient = base::Color::WHITE;
        base::Color m_colorDiffuse = base::Color::WHITE;
        base::Color m_colorSpecular = base::Color::WHITE;
        base::Color m_colorEmissive = base::Color::WHITE;
        base::Color m_colorTransmission = base::Color::WHITE;

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
    class ASSETS_OBJ_LOADER_API FormatMTL : public base::res::ISourceAsset
    {
        RTTI_DECLARE_VIRTUAL_CLASS(FormatMTL, base::res::ISourceAsset);

    public:
        FormatMTL();
        FormatMTL(const base::Array<Material>& materials);

        /// all of the defined materials
        INLINE const base::Array<Material>& materials() const { return m_materials; }

        //--

        const Material* findMaterial(base::StringView name) const;

        //--

    private:
        base::Array<Material> m_materials;
        base::HashMap<base::StringBuf, uint32_t> m_materialMap;

        static bool ParseColor(base::StringParser& str, base::Color& outColor);
        static bool ParseMaterial(base::StringParser& str, Material& outMat);
        static bool ParseMaterialMap(base::StringParser& str, MaterialMap& outMap);

        //--

        virtual uint64_t calcMemoryUsage() const override;

        virtual void onPostLoad() override;
        void buildMaterialMap();
    };

    //--

    /// load from object file
    extern ASSETS_OBJ_LOADER_API FormatMTLPtr LoadMaterials(base::StringView contextName, const void* data, uint64_t dataSize);

    //--

} // wavefront
