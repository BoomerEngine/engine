/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fbx #]
***/

#include "build.h"

#include "fbxMeshImporter.h"
#include "fbxMaterialImporter.h"
#include "fbxFileLoaderService.h"
#include "fbxFileData.h"

#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/material/include/renderingMaterialInstance.h"

#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resource/include/resource.h"
#include "base/containers/include/inplaceArray.h"
#include "base/resource/include/resourceTags.h"

#include "import/mesh_loader/include/renderingMeshCooker.h"

namespace fbx
{

    //--

    RTTI_BEGIN_TYPE_CLASS(FBXMeshImportConfig);
        //RTTI_METADATA(base::res::ResourceManifestExtensionMetadata).extension("mesh.meta");
        //RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("FBX Import SetupMetadata");
        RTTI_CATEGORY("FBXTuning");
        RTTI_PROPERTY(m_applyNodeTransform).editable("Export meshes in world space instead of centered at pivot").overriddable();
        RTTI_PROPERTY(m_forceNodeSkin).editable("Skin all meshes to parent nodes").overriddable();
        RTTI_PROPERTY(m_flipUV).editable("Flip V channel of the UVs").overriddable();
    RTTI_END_TYPE();

    FBXMeshImportConfig::FBXMeshImportConfig()
    {
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(MeshImporter);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<rendering::Mesh>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("fbx").addSourceExtension("FBX");
        RTTI_METADATA(base::res::ResourceCookerVersionMetadata).version(0);
        RTTI_METADATA(base::res::ResourceImporterConfigurationClassMetadata).configurationClass<FBXMeshImportConfig>();
    RTTI_END_TYPE();

    //--

    static bool BuildModels(base::IProgressTracker& progress, const LoadedFile& sourceGeometry, const DataMeshExportSetup& config, DataNodeMesh& outModel, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials)
    {
        const auto& nodes = sourceGeometry.nodes();

        for (uint32_t i=0; i<nodes.size(); ++i)
        {
            const auto& node = nodes[i];

            if (node->m_type == DataNodeType::VisualMesh)
            {
                // update status, also support cancellation
                progress.reportProgress(i, nodes.size(), base::TempString("Processing node '{}'", node->m_name));
                if (progress.checkCancelation())
                    return false;

                // export geometry
                node->exportToMeshModel(progress, sourceGeometry, config, outModel, outSkeleton, outMaterials);
            }
        }

        return true;
    }

    //--

    MeshImporter::MeshImporter()
    {}

    base::RefPtr<rendering::MaterialImportConfig> MeshImporter::createMaterialImportConfig(const rendering::MeshImportConfig& cfg, base::StringView name) const
    {
        auto ret = base::RefNew<FBXMaterialImportConfig>();
        ret->m_materialName = base::StringBuf(name);
        ret->markPropertyOverride("materialName"_id);
        return ret;
    }

    base::res::ResourcePtr MeshImporter::importResource(base::res::IResourceImporterInterface& importer) const
    {
        // load the FBX data
        auto importedScene = base::rtti_cast<fbx::LoadedFile>(importer.loadSourceAsset(importer.queryImportPath()));
        if (!importedScene)
        {
            TRACE_ERROR("Failed to load scene from import file");
            return false;
        }

        // get the configuration
        auto importConfig = importer.queryConfigration<FBXMeshImportConfig>();

        // create output mesh
        auto existingMesh = base::rtti_cast<rendering::Mesh>(importer.existingData());

        //---

        // build the scaling matrix
        auto assetToEngine = importConfig->calcAssetToEngineConversionMatrix(rendering::MeshImportUnits::Centimeters, rendering::MeshImportSpace::LeftHandYUp);

        // setup config
        DataMeshExportSetup meshExportConfig;
        meshExportConfig.assetToEngine = assetToEngine;
        meshExportConfig.applyNodeTransform = importConfig->m_applyNodeTransform;
        meshExportConfig.flipFace = importConfig->flipFaces;
        meshExportConfig.flipUV = importConfig->m_flipUV;
        meshExportConfig.forceSkinToNode = importConfig->m_forceNodeSkin;

        // extract the unpacked meshes from the FBX
        SkeletonBuilder skeleton;
        MaterialMapper materials;
        DataNodeMesh mesh;
        BuildModels(importer, *importedScene, meshExportConfig, mesh, skeleton, materials);

        // prepare mesh export setup
        rendering::MeshInitData initData;

        // pack mesh streams into render chunks
        rendering::BuildChunks(mesh.chunks, *importConfig, importer, initData.chunks, initData.bounds);

        //----

        // create default LODs
        bool resetLODs = true;
        if (resetLODs || !existingMesh)
        {
            // TODO: import FbxLODGroup

            // find max LOD level
            uint32_t numLods = 1;
            {
                uint32_t lodMask = 1;
                for (const auto& chunk : initData.chunks)
                    lodMask |= chunk.detailMask;

                lodMask /= 2;
                while (lodMask)
                {
                    numLods += 1;
                    lodMask /= 2;
                }
            }
            TRACE_INFO("Found '{}' LODs in mesh", numLods);

            const auto lodStep = 100.0f / (float)numLods;
            
            for (uint32_t i=0; i<numLods; ++i)
            {
                auto& defaultRange = initData.detailLevels.emplaceBack();
                defaultRange.rangeMin = i * lodStep;
                defaultRange.rangeMax = defaultRange.rangeMin + lodStep;
            }
        }
        else
        {
            initData.detailLevels = existingMesh->detailLevels();
        }

        //--

        // create materials
        for (uint32_t i : materials.materials.indexRange())
        {
            const auto materialName = materials.materials[i];

            auto& entry = initData.materials.emplaceBack();
            entry.name = base::StringID(materialName);
            entry.material = buildSingleMaterial(importer, *importConfig, materialName, "", i, existingMesh);
        }
        
        //--

        // copy other settings
        if (existingMesh)
        {
            initData.visibilityDistanceMultiplier = existingMesh->visibilityDistanceMultiplier();
            initData.visibilityDistanceOverride = existingMesh->visibilityDistanceOverride();
            initData.visibilityGroup = existingMesh->visibilityGroup();
        }

        //--
        
        // attach payloads
        return base::RefNew<rendering::Mesh>(std::move(initData));
    }

} // fbx

