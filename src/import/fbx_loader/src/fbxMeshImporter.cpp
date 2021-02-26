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

#include "engine/mesh/include/renderingMesh.h"
#include "engine/material/include/renderingMaterialInstance.h"

#include "core/io/include/ioFileHandle.h"
#include "core/app/include/localServiceContainer.h"
#include "core/resource/include/resource.h"
#include "core/containers/include/inplaceArray.h"
#include "core/resource/include/resourceTags.h"

#include "import/mesh_loader/include/renderingMeshCooker.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

RTTI_BEGIN_TYPE_CLASS(FBXMeshImportConfig);
    //RTTI_METADATA(res::ResourceManifestExtensionMetadata).extension("mesh.meta");
    //RTTI_METADATA(res::ResourceDescriptionMetadata).description("FBX Import SetupMetadata");
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
    RTTI_METADATA(res::ResourceCookedClassMetadata).addClass<Mesh>();
    RTTI_METADATA(res::ResourceSourceFormatMetadata).addSourceExtension("fbx").addSourceExtension("FBX");
    RTTI_METADATA(res::ResourceCookerVersionMetadata).version(0);
    RTTI_METADATA(res::ResourceImporterConfigurationClassMetadata).configurationClass<FBXMeshImportConfig>();
RTTI_END_TYPE();

//--

static bool BuildModels(IProgressTracker& progress, const FBXFile& sourceGeometry, const DataMeshExportSetup& config, DataNodeMesh& outModel, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials)
{
    const auto& nodes = sourceGeometry.nodes();

    for (uint32_t i=0; i<nodes.size(); ++i)
    {
        const auto& node = nodes[i];

        if (node->m_type == DataNodeType::VisualMesh)
        {
            // update status, also support cancellation
            progress.reportProgress(i, nodes.size(), TempString("Processing node '{}'", node->m_name));
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

RefPtr<MaterialImportConfig> MeshImporter::createMaterialImportConfig(const MeshImportConfig& cfg, StringView name) const
{
    auto ret = RefNew<FBXMaterialImportConfig>();
    ret->m_materialName = StringBuf(name);
    ret->markPropertyOverride("materialName"_id);
    return ret;
}

res::ResourcePtr MeshImporter::importResource(res::IResourceImporterInterface& importer) const
{
    // load the FBX data
    auto importedScene = rtti_cast<FBXFile>(importer.loadSourceAsset(importer.queryImportPath()));
    if (!importedScene)
    {
        TRACE_ERROR("Failed to load scene from import file");
        return false;
    }

    // get the configuration
    auto importConfig = importer.queryConfigration<FBXMeshImportConfig>();

    // create output mesh
    auto existingMesh = rtti_cast<Mesh>(importer.existingData());

    //---

    // build the scaling matrix
    auto assetToEngine = importConfig->calcAssetToEngineConversionMatrix(MeshImportUnits::Centimeters, MeshImportSpace::LeftHandYUp);

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
    MeshInitData initData;

    // pack mesh streams into render chunks
    BuildChunks(mesh.chunks, *importConfig, importer, initData.chunks, initData.bounds);

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
        entry.name = StringID(materialName);
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
    return RefNew<Mesh>(std::move(initData));
}

END_BOOMER_NAMESPACE_EX(assets)

