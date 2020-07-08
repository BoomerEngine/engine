/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooker #]
***/

#pragma once

#include "base/resource/include/resource.h"

namespace rendering
{
    //--

    /// data for single material
    struct ASSETS_MESH_LOADER_API MeshMaterialBindingSingleMaterialProperty
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MeshMaterialBindingSingleMaterialProperty);

        base::StringID name;
        base::Variant data;
    };

    //--

    /// data for single material
    struct ASSETS_MESH_LOADER_API MeshMaterialBindingSingleMaterialData
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(MeshMaterialBindingSingleMaterialData);

        base::StringID name;
        MaterialRef baseMaterial;
        base::Array<MeshMaterialBindingSingleMaterialProperty> properties;

        void applyOn(rendering::MaterialInstance& material) const;
        bool captureFrom(const rendering::MaterialInstance& material, const rendering::MaterialInstance* baseMaterial = nullptr);
        bool writeProperty(base::StringID name, const base::Variant& data);
        bool removeProperty(base::StringID name);
    };

    //--

    /// manifest with material assignments for rendering meshes
    class ASSETS_MESH_LOADER_API MeshMaterialBindingManifest : public base::res::IResourceManifest
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshMaterialBindingManifest, base::res::IResourceManifest);

    public:
        MeshMaterialBindingManifest();

        const MeshMaterialBindingSingleMaterialData* findMaterial(base::StringID name) const;

        MeshMaterialBindingSingleMaterialData* getMaterial(base::StringID name);

        bool captureMaterial(base::StringID name, const rendering::MaterialInstance& material, const rendering::MaterialInstance* baseMaterial = nullptr);

        bool applyMaterial(base::StringID name, rendering::MaterialInstance& material) const;

    private:
        base::Array<MeshMaterialBindingSingleMaterialData> m_materials;
    };

    //--

} // rendering