/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooker #]
***/

#include "build.h"
#include "rendernigMeshMaterialManifest.h"
#include "rendering/material/include/renderingMaterialInstance.h"

namespace rendering
{
    //--

    RTTI_BEGIN_TYPE_CLASS(MeshMaterialBindingSingleMaterialProperty);
        RTTI_PROPERTY(name);
        RTTI_PROPERTY(data);
    RTTI_END_TYPE();

    //--

    RTTI_BEGIN_TYPE_CLASS(MeshMaterialBindingSingleMaterialData);
        RTTI_PROPERTY(name);
        RTTI_PROPERTY(baseMaterial);
        RTTI_PROPERTY(properties);
    RTTI_END_TYPE();

    void MeshMaterialBindingSingleMaterialData::applyOn(rendering::MaterialInstance& material) const
    {
        //material.removeAllParameters();
        material.baseMaterial(baseMaterial);

        for (const auto& param : properties)
            material.writeParameterRaw(param.name, param.data.data(), param.data.type());
    }

    bool MeshMaterialBindingSingleMaterialData::removeProperty(base::StringID name)
    {
        for (uint32_t i = 0; i < properties.size(); ++i)
        {
            if (properties[i].name == name)
            {
                properties.erase(i);
                return true;
            }
        }

        return false;
    }

    bool MeshMaterialBindingSingleMaterialData::writeProperty(base::StringID name, const base::Variant& data)
    {
        if (name && data)
        {
            for (auto& param : properties)
            {
                if (param.name == name)
                {
                    if (param.data == data)
                        return false;

                    param.data = data;
                    return true;
                }
            }

            auto& newProp = properties.emplaceBack();
            newProp.name = name;
            newProp.data = data;
            return true;
        }

        return false;
    }

    static bool HasParamAlreadyInBase(base::StringID name, const rendering::MaterialInstance& material, const rendering::MaterialInstance* baseMaterialData)
    {
        if (!baseMaterialData)
            return false;

        for (const auto& materialParam : material.parameters())
        {
            if (materialParam.name == name)
            {
                for (const auto& baseMaterialParam : baseMaterialData->parameters())
                {
                    if (baseMaterialParam.name == name)
                    {
                        return baseMaterialParam.value == materialParam.value;
                    }
                }

                break;
            }
        }

        return false;
    }

    bool MeshMaterialBindingSingleMaterialData::captureFrom(const rendering::MaterialInstance& material, const rendering::MaterialInstance* baseMaterialData)
    {
        bool changed = false;

        if (baseMaterial != material.baseMaterial())
        {
            baseMaterial = material.baseMaterial();
            changed = true;
        }

        for (const auto& param : material.parameters())
        {
            if (param.name)
            {
                if (HasParamAlreadyInBase(param.name, material, baseMaterialData))
                {
                    changed |= removeProperty(param.name);
                }
                else
                {
                    changed |= writeProperty(param.name, param.value);
                }
            }
        }

        return changed;
    }

    //--
     
    RTTI_BEGIN_TYPE_CLASS(MeshMaterialBindingManifest);
        RTTI_METADATA(base::res::ResourceManifestExtensionMetadata).extension("material.meta");
        RTTI_PROPERTY(m_materials);
    RTTI_END_TYPE();

    MeshMaterialBindingManifest::MeshMaterialBindingManifest()
    {}

    MeshMaterialBindingSingleMaterialData* MeshMaterialBindingManifest::getMaterial(base::StringID name)
    {
        if (!name)
            return nullptr;

        for (auto& entry : m_materials)
            if (entry.name == name)
                return &entry;

        auto& entry = m_materials.emplaceBack();
        entry.name = name;

        markModified();
        return &entry;
    }

    const MeshMaterialBindingSingleMaterialData* MeshMaterialBindingManifest::findMaterial(base::StringID name) const
    {
        if (!name)
            return nullptr;

        for (const auto& entry : m_materials)
            if (entry.name == name)
                return &entry;

        return nullptr;
    }

    bool MeshMaterialBindingManifest::captureMaterial(base::StringID name, const rendering::MaterialInstance& material, const rendering::MaterialInstance* baseMaterial)
    {
        if (auto* data = getMaterial(name))
        {
            if (data->captureFrom(material, baseMaterial))
            {
                markModified();
                return true;
            }
        }

        return false;
    }

    bool MeshMaterialBindingManifest::applyMaterial(base::StringID name, rendering::MaterialInstance& material) const
    {
        if (const auto* data = findMaterial(name))
        {
            data->applyOn(material);
            material.createMaterialProxy();
            return true;
        }

        return false;
    }

    //---
    
} // rendering
