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
#include "fbxFileData.h"

#include "engine/mesh/include/mesh.h"
#include "engine/material/include/materialInstance.h"

#include "core/io/include/ioFileHandle.h"
#include "core/app/include/localServiceContainer.h"
#include "core/resource/include/resource.h"
#include "core/containers/include/inplaceArray.h"
#include "core/resource/include/resourceTags.h"

#include "import/mesh_loader/include/renderingMeshCooker.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

#pragma optimize ("", off)

//--

RTTI_BEGIN_TYPE_CLASS(FBXMeshImportConfig);
    RTTI_OLD_NAME("fbx::FBXMeshImportConfig");
    //RTTI_METADATA(res::ResourceManifestExtensionMetadata).extension("mesh.meta");
    //RTTI_METADATA(res::ResourceDescriptionMetadata).description("FBX Import SetupMetadata");
    RTTI_CATEGORY("FBXTuning");
    RTTI_PROPERTY(m_importAtRootSpace).editable("Apply the inital transform of the root's chilren nodes").overriddable();
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
    Array<ExportNode> exportNodes;
    sourceGeometry.collectExportNodes(config.applyRootTransform, exportNodes);

    for (auto i : exportNodes.indexRange())
    {
        const auto& node = exportNodes[i];
        const auto* data = node.data;

        // update status, also support cancellation
        progress.reportProgress(i, exportNodes.size(), TempString("Processing node '{}'", data->m_name));
        if (progress.checkCancelation())
            return false;

        if (data->m_type == DataNodeType::VisualMesh)
            data->exportToMeshModel(progress, sourceGeometry, node.localToAsset, config, outModel, outSkeleton, outMaterials);
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

static const uint32_t CalcTrianglesForLOD(const MeshInitData& data, uint32_t lodIndex)
{
    const auto lodMask = 1U << lodIndex;

    uint32_t totalCount = 0;
    for (const auto& chunk : data.chunks)
        if (chunk.detailMask & lodMask)
            totalCount += chunk.indexCount / 3;
    
    return totalCount;
}

res::ResourcePtr MeshImporter::importResource(res::IResourceImporterInterface& importer) const
{
    // get the configuration
    auto importConfig = importer.queryConfigration<FBXMeshImportConfig>();

    // create output mesh
    auto existingMesh = rtti_cast<Mesh>(importer.existingData());

    // setup config
    DataMeshExportSetup meshExportConfig;
    meshExportConfig.applyRootTransform = importConfig->m_importAtRootSpace;
    meshExportConfig.flipFace = importConfig->flipFaces;
    meshExportConfig.flipUV = importConfig->m_flipUV;
    meshExportConfig.forceSkinToNode = importConfig->m_forceNodeSkin;

    SkeletonBuilder skeleton;
    MaterialMapper materials;
    DataNodeMesh mesh;

    // import the LODs
    Array<RefPtr<FBXFile>> fbxFiles;
    for (uint32_t i = 0; i < 8; ++i)
    {
        // assemble the load path
        StringBuf loadPath = importer.queryImportPath();
        if (i > 0)
        {
            auto coreDir = loadPath.view().baseDirectory();
            auto coreExt = loadPath.view().extensions();
            auto coreName = loadPath.view().fileStem();

            coreName = coreName.beforeLastNoCaseOrFull("_L0");
            loadPath = TempString("{}{}_L{}.{}", coreDir, coreName, i, coreExt);
        }

        // load the FBX data
        auto importedScene = rtti_cast<FBXFile>(importer.loadSourceAsset(loadPath));
        if (!importedScene)
        {
            if (i == 0)
            {
                TRACE_ERROR("Failed to load scene from import file");
                return false;
            }

            break;
        }

        // keep around
        fbxFiles.pushBack(importedScene);

        // build the scaling matrix
        auto assetToEngine = importConfig->calcAssetToEngineConversionMatrix(importedScene->scaleFactor() / 100.0f, importedScene->space());
        meshExportConfig.assetToEngine = assetToEngine;

        // extract the unpacked meshes from the FBX
        const auto firstChunk = mesh.chunks.size();
        BuildModels(importer, *importedScene, meshExportConfig, mesh, skeleton, materials);

        // set LOD
        if (i > 0)
        {
            if (i == 1)
            {
                for (uint32_t j = 0; j < firstChunk; ++j)
                {
                    auto& chunk = mesh.chunks[j];
                    chunk.detailMask = 1;
                }
            }

            for (uint32_t j = firstChunk; j < mesh.chunks.size(); ++j)
            {
                auto& chunk = mesh.chunks[j];
                chunk.detailMask = 1U << i;
            }
        }
    }

    // pack mesh streams into render chunks
    MeshInitData initData;
    BuildChunks(mesh.chunks, *importConfig, importer, initData.chunks, initData.bounds);

    //----

    // create default LODs
    bool resetLODs = true;
    if (resetLODs || !existingMesh)
    {
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

        const auto baseLODSize = std::max<float>(1.0f, initData.bounds.size().maxValue());
        const auto baseLODScaleFactor = std::clamp<float>(std::log2(baseLODSize) / 10.0f, 0.0f, 1.0f);

        const auto referenceVisibilityRange = 100.0f;

        const auto totalVisibilityScaleFactor = 1.0f + baseLODScaleFactor * 2.0f;
        const auto totalVisibilityDistance = std::max<float>(referenceVisibilityRange, baseLODSize * totalVisibilityScaleFactor);

        // calculate LOD contributions
        InplaceArray<float, 10> lodDistanceFactors;
        float distanceFactorSum = 1.0f;
        {
            auto prevLODTriangles = std::max<uint32_t>(1, CalcTrianglesForLOD(initData, 0));

            // LOD0
            lodDistanceFactors.pushBack(1.0f);

            // extra lods
            for (uint32_t i = 1; i < numLods; ++i)
            {
                const auto currentLODTriangles = std::max<uint32_t>(1, CalcTrianglesForLOD(initData, i));
                const auto geometryReductionFactor = std::sqrt(std::max<float>(1.0f, prevLODTriangles / (float)currentLODTriangles)); // sqrt(triangle diff) -> 4x less triangles -> 2 distance, 9x less triangles -> 3x distance

                lodDistanceFactors.pushBack(geometryReductionFactor);
                distanceFactorSum += geometryReductionFactor;
                prevLODTriangles = currentLODTriangles;
            }

            // extend last LOD
            if (numLods > 1)
            {
                const float extraLastLODDistance = baseLODScaleFactor * 2.0f;
                auto& lastLOD = lodDistanceFactors.back();
                lastLOD += extraLastLODDistance;
                distanceFactorSum += extraLastLODDistance;
            }
        }

        // create LODs
        float accumulatedDistanceFactor = 0.0f;
        float prevDistance = 0.0f;
        for (uint32_t i=0; i<numLods; ++i)
        {
            auto& defaultRange = initData.detailLevels.emplaceBack();
            defaultRange.rangeMin = prevDistance;

            accumulatedDistanceFactor += lodDistanceFactors[i];
            defaultRange.rangeMax = baseLODSize + (accumulatedDistanceFactor / distanceFactorSum) * totalVisibilityDistance;
            prevDistance = defaultRange.rangeMax;
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

