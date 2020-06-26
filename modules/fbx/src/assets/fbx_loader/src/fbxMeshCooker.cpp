/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fbx #]
***/

#include "build.h"

#include "fbxMeshCooker.h"
#include "fbxFileLoaderService.h"
#include "fbxFileData.h"

#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resourceCookingInterface.h"
#include "base/containers/include/inplaceArray.h"

#include "base/geometry/include/mesh.h"
#include "base/geometry/include/meshStreamBuilder.h"

namespace fbx
{

    //--

    RTTI_BEGIN_TYPE_CLASS(MeshManifest);
        RTTI_METADATA(base::res::ResourceManifestExtensionMetadata).extension("mesh.meta");
        RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("FBX Import SetupMetadata");
     RTTI_PROPERTY(m_alignToPivot).editable("Align imported geometry to the pivot of the root exported node");
    RTTI_END_TYPE();

    MeshManifest::MeshManifest()
        : m_alignToPivot(true)
    {
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(MeshCooker);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<base::mesh::Mesh>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("fbx").addSourceExtension("FBX");
        RTTI_METADATA(base::res::ResourceCookerVersionMetadata).version(0);
    RTTI_END_TYPE();

    //--

    MeshCooker::MeshCooker()
    {
    }

    base::res::ResourcePtr MeshCooker::cook(base::res::IResourceCookerInterface& cooker) const
    {
        auto importedScene = base::GetService<FileLoadingService>()->loadScene(cooker);
        if (!importedScene)
        {
            TRACE_ERROR("Failed to load scene from import file");
            return false;
        }

        //---

        // build the geometry
        SkeletonBuilder skeleton;
        MaterialMapper materials;
        /*base::Array<rendering::content::GeometryChunkGroupPtr> chunks;
        if (!buildChunkGroups(cooker, *importedScene, chunks, skeleton, materials))
            return false;

        // build the material
        base::Array<rendering::content::MeshMaterial> meshMaterials;
        if (!buildMaterialData(cooker, *importedScene, materials, meshMaterials))
            return false;*/

        //---

        // TODO
        return base::RefPtr<base::mesh::Mesh>();
    }

#if 0
    bool MeshCooker::buildChunkGroups(base::res::IResourceCookerInterface& cooker, const LoadedFile& sourceGeometry, base::Array<rendering::content::GeometryChunkGroupPtr>& outGroups, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials) const
    {
        base::Task task(2, "");

        // build the scaling matrix
        auto manifest = cooker.loadManifestFile<MeshManifest>();
        auto fileToWorld = manifest->calcAssetToEngineConversionMatrix(GeometryImportUnits::Meters, GeometryImportSpace::RightHandZUp);

        // start building the mesh
        rendering::content::MeshGeometryBuilder geometry;

        // extract meshes from all nodes
        {
            base::Task task(sourceGeometry.nodes().size(), "Extracting geometry...");
            for (auto node  : sourceGeometry.nodes())
            {
                base::Task localTask(1,  "");

                if (node->type == DataNodeType::VisualMesh && node->lodIndex == 0)
                {
                    node->exportToMeshBuilder(sourceGeometry, fileToWorld, geometry, outSkeleton, outMaterials, false);
                }
            }
        }

        // extract data
        {
            base::Task task(1, "Packing data into streams");
            auto chunk = geometry.extractData();
            if (!chunk)
                return false;

            outGroups.pushBack(chunk);
        }

        return true;
    }

    bool MeshCooker::buildMaterialData(base::res::IResourceCookerInterface& cooker, const LoadedFile& sourceGeometry, const MaterialMapper& materials, base::Array<rendering::content::MeshMaterial>& outMaterials) const
    {
        base::StringBuf selfImportPath = cooker.queryResourcePath().path();

        // process the materials
        {
            base::Task task(materials.m_materials.size(), "Processing materials");
            for (auto material  : materials.m_materials)
            {
                auto materialName = base::StringBuf(material->GetName());

                base::Task task(1, base::TempString("{}", materialName));

                // build a material path
                base::res::ResourcePathBuilder pathBuilder;
                pathBuilder.path(selfImportPath);
                pathBuilder.param("Material", materialName.c_str());

                // load material
                rendering::MaterialInstanceRef baseMaterialRef;
                {
                    if (auto material = base::LoadResource<rendering::content::MaterialInstance>(pathBuilder.buildPath()))
                        baseMaterialRef.reset(material);
                }

                // build a reference to material
                if (baseMaterialRef.empty())
                    baseMaterialRef.resetToPathOnlyNoLoading(pathBuilder.buildPath(), rendering::content::MaterialInstance::GetStaticClass());

                // set base material only
                auto& entry = outMaterials.emplaceBack();
                entry.name = base::StringID(materialName.c_str());
                entry.m_material = baseMaterialRef;
            }
        }

        return true;
    }
#endif

} // fbx

