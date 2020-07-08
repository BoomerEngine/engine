/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooker #]
***/

#include "build.h"
#include "renderingMeshCooker.h"
#include "rendernigMeshMaterialManifest.h"
#include "rendernigMeshPackingManifest.h"

#include "base/geometry/include/mesh.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "rendering/material/include/renderingMaterialInstance.h"
#include "rendering/material/include/renderingMaterialTemplate.h"

namespace rendering
{
    //---

    // cooker for creating a rendering mesh from a generic geometry mesh
    class ASSETS_MESH_LOADER_API GenericMeshCooker : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GenericMeshCooker, base::res::IResourceCooker);

    public:
        virtual void reportManifestClasses(base::Array<base::SpecificClassType<base::res::IResourceManifest>>& outManifestClasses) const override final
        {
            outManifestClasses.pushBackUnique(MeshMaterialBindingManifest::GetStaticClass());
            outManifestClasses.pushBackUnique(MeshPackingManifest::GetStaticClass());
        }

        virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override final
        {
            auto importPath = cooker.queryResourcePath();

            // load the source mesh data
            auto sourceMesh = cooker.loadDependencyResource<base::mesh::Mesh>(importPath);
            if (!sourceMesh)
            {
                TRACE_ERROR("Unable to load source mesh from '{}'", importPath);
                return nullptr;
            }

            // load manifests
            MeshCookingSettings settings;
            settings.packingManifest = cooker.loadManifestFile<MeshPackingManifest>();
            settings.materialManifest = cooker.loadManifestFile<MeshMaterialBindingManifest>();

            // cook data
            auto mesh = CookMesh(*sourceMesh, settings, cooker);
            if (!mesh)
            {
                TRACE_ERROR("Unable to cook rendering mesh from '{}'", importPath);
                return nullptr;
            }

            // TODO: additional mesh post processing - physics packing/etc

            return mesh;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(GenericMeshCooker);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<rendering::Mesh>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceClass<base::mesh::Mesh>();
    RTTI_END_TYPE();

    //--
    
} // rendering
