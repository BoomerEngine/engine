/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooker #]
***/

#include "build.h"

#include "renderingMeshCooker.h"
#include "renderingMeshCookerChunks.h"
#include "renderingMeshImportConfig.h"

#include "engine/material/include/materialInstance.h"
#include "engine/material/include/materialTemplate.h"
#include "engine/texture/include/texture.h"
#include "core/resource/include/loader.h"

#undef TRACE_INFO
#define TRACE_INFO TRACE_WARNING

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

MeshVertexFormat SelectBestVertexFormat(const ImportChunk& chunk, const MeshImportConfig& settings)
{
    return MeshVertexFormat::Static;
}

//--

static void ExtractImportChunks(const Array<MeshRawChunk>& sourceMesh, ImportChunkRegistry& outImportChunks)
{
    outImportChunks.bounds = Box();

    for (const auto& sourceChunk : sourceMesh)
    {
        auto importChunk = RefNew<ImportChunk>(sourceChunk);
        outImportChunks.importChunks.pushBack(importChunk);
        outImportChunks.bounds.merge(sourceChunk.bounds);
    }

    TRACE_INFO("Found {} import chunk(s)", outImportChunks.importChunks.size());
}

//--

static int GenerateOrRemove(bool hasData, MeshDataRecalculationMode mode)
{
    switch (mode)
    {
        case MeshDataRecalculationMode::Never: 
            return 0; // do not generate, keep as is

        case MeshDataRecalculationMode::WhenMissing:
            return hasData ? 0 : 1; // generate data if we don't have it

        case MeshDataRecalculationMode::Always:
            return 1; // always generate data

        case MeshDataRecalculationMode::Remove:
            return -1; // remove data
    }

    return 0; // keep as is
}

template< typename T >
static void FlipVectors(T* ptr, uint32_t count)
{
    if (ptr)
    {
        auto* endPtr = ptr + count;
        while (ptr < endPtr)
        {
            *ptr = -*ptr;
            ptr++;
        }
    }
}

static bool CalculateNormals(const ImportChunkRegistry& importChunks, const MeshImportConfig& settings, IProgressTracker& progress)
{        
    Array<ImportChunk*> importChunksToRecalculate;

    const auto recalculationMode = settings.m_normalRecalculation;
    for (auto& chunk : importChunks.importChunks)
    {
        if (const auto op = GenerateOrRemove(chunk->hasNormals(), recalculationMode))
        {
            if (op < 0)
            {
                chunk->removeNormals();
            }
            else if (op > 0)
            {
                importChunksToRecalculate.pushBack(chunk);
            }
        }
    }

    RunFiberForFeach<ImportChunk*>("RecomputeVertexNormals", importChunksToRecalculate, 0, [&progress](ImportChunk* chunk)
        {
            if (!progress.checkCancelation())
                chunk->computeFlatNormals();
        });
        
    if (settings.m_flipNormals)
    {
        RunFiberForFeach<RefPtr<ImportChunk>>("FlipNormals", importChunks.importChunks, 0, [&progress](ImportChunk* chunk)
            {
                if (!progress.checkCancelation())
                {
                    auto normalData = (Vector3*) chunk->vertexStreamData(MeshStreamType::Normal_3F);
                    FlipVectors(normalData, chunk->numVertices);
                }
            });
    }

    return !progress.checkCancelation();
}

//--

static bool CalculateTangents(const ImportChunkRegistry& importChunks, const MeshImportConfig& settings, IProgressTracker& progress)
{
    Array<ImportChunk*> importChunkToRecompute;
    const auto recalculationMode = settings.m_tangentsRecalculation;
    for (auto& chunk : importChunks.importChunks)
    {
        if (const auto op = GenerateOrRemove(chunk->hasTangents(), recalculationMode))
        {
            if (op > 0)
            {
                importChunkToRecompute.pushBack(chunk);
            }
            else if (op < 0)
            {
                chunk->removeTangentSpace();
            }
        }
    }

    RunFiberForFeach<ImportChunk*>("CalculateTangentSpace", importChunkToRecompute, 0, [&settings, &progress](ImportChunk* chunk)
        {
            if (!progress.checkCancelation())
                chunk->computeTangentSpace(settings.m_tangentsAngularThreshold);
        });

    if (settings.m_flipTangent)
    {
        RunFiberForFeach<RefPtr<ImportChunk>>("FlipTangents", importChunks.importChunks, 0, [&progress](ImportChunk* chunk)
            {
                if (!progress.checkCancelation())
                {
                    auto normalData = (Vector3*) chunk->vertexStreamData(MeshStreamType::Tangent_3F);
                    FlipVectors(normalData, chunk->numVertices);
                }
            });
    }

    if (settings.m_flipBitangent)
    {
        RunFiberForFeach<RefPtr<ImportChunk>>("FlipTangents", importChunks.importChunks, 0, [&progress](ImportChunk* chunk)
            {
                if (!progress.checkCancelation())
                {
                    auto normalData = (Vector3*) chunk->vertexStreamData(MeshStreamType::Binormal_3F);
                    FlipVectors(normalData, chunk->numVertices);
                }
            });
    }

    return !progress.checkCancelation();
}

//--

static bool CalculateUVS(const ImportChunkRegistry& importChunks, const MeshImportConfig& settings)
{
    // TODO!
    return true;
}

//--

static void GenerateBuildChunks(const ImportChunkRegistry& importChunks, BuildChunkRegistry& outBuildChunks, const MeshImportConfig& settings)
{
    uint32_t numSkippedChunks = 0;
    for (const auto& sourceChunk : importChunks.importChunks)
    {
        const auto format = SelectBestVertexFormat(*sourceChunk, settings);
        if (auto* buildChunk = outBuildChunks.findBuildChunk(format, sourceChunk->materialIndex, sourceChunk->renderMask, sourceChunk->detailMask))
        {
            buildChunk->addChunk(*sourceChunk);
        }
        else
        {
            numSkippedChunks += 1;
        }
    }

    TRACE_INFO("Created {} build chunk(s) from {} source chunk(s) ({} skipped)", 
        outBuildChunks.m_buildChunks.size(), importChunks.importChunks.size(), numSkippedChunks);
    
    for (uint32_t i : outBuildChunks.m_buildChunks.indexRange())
    {
        const auto& buildChunk = outBuildChunks.m_buildChunks[i];
        TRACE_INFO("BuildChunk[{}]: material {}, format {}, renderMask {}, detailMask {}", i, buildChunk->m_material, buildChunk->m_format, Hex(buildChunk->m_renderMask), Hex(buildChunk->m_detailMask));
    }
}

//--

static bool PackData(const BuildChunkRegistry& builder, const MeshImportConfig& settings, IProgressTracker& progress)
{
    // collect chunks with quantization bounds
    Box quantizationBounds;
    for (const auto& buildChunk : builder.m_buildChunks)
        if (buildChunk->m_quantizationGroup != -1)
            for (const auto& sourceChunk : buildChunk->m_sourceChunks)
                quantizationBounds.merge(sourceChunk->bounds);
    TRACE_INFO("Quantization bounds: [{},{},{}] - [{},{},{}]",
        quantizationBounds.min.x, quantizationBounds.min.y, quantizationBounds.min.z,
        quantizationBounds.max.x, quantizationBounds.max.y, quantizationBounds.max.z);

    // setup quantization
    MeshVertexQuantizationHelper quantization(quantizationBounds);

    // setup quantization for each build chunk
    // TODO: quantization groups!
    for (const auto& buildChunk : builder.m_buildChunks)
    {
        buildChunk->m_quantizationOffset = quantization.quantizatonOffset();
        buildChunk->m_quantizationScale = quantization.quantizatonScale_22_22_20();
    }

    // pack build chunks
    {
        ScopeTimer timer;
        RunFiberForFeach<RefPtr<BuildChunk>>("PackMeshChunk", builder.m_buildChunks, 0, [&quantization, &builder, &progress](BuildChunk* chunk)
            {
                if (!progress.checkCancelation())
                    chunk->pack(quantization, progress);
            });

        if (progress.checkCancelation())
            return false;

        uint32_t totalVertexDataSize = 0;
        uint32_t totalIndexDataSize = 0;
        for (const auto& chunk : builder.m_buildChunks)
        {
            totalVertexDataSize += chunk->m_packedVertexData.size();
            totalIndexDataSize += chunk->m_packedIndexData.size();
        }

        TRACE_INFO("Packed {} chunks into total {} vertex and {} index of data in {}", builder.m_buildChunks.size(), MemSize(totalVertexDataSize), MemSize(totalIndexDataSize), timer);
        return true;
    }
}

//--

/*void ExportMaterials(const Mesh& sourceMesh, const MeshImportConfig& settings, Array<MeshMaterial>& outExportMaterials)
{
    outExportMaterials.reserve(sourceMesh.materials().size());
    for (const auto& sourceMaterial : sourceMesh.materials())
    {
        auto& exportInfo = outExportMaterials.emplaceBack();
        exportInfo.name = sourceMaterial.name;

        exportInfo.baseMaterial = RefNew<MaterialInstance>();

        if (sourceMaterial.flags.test(MeshMaterialFlagBit::Unlit))
        {
            exportInfo.baseMaterial->baseMaterial(resDefaultMaterialTemplate.loadAndGetAsRef());
        }
        else
        {
            if (sourceMaterial.parameters.findVariant("RoughnessSpecularMap"_id))
            {
                exportInfo.baseMaterial->baseMaterial(resLitCombinedRSMaterialTemplate.loadAndGetAsRef());
            }
            else
            {
                exportInfo.baseMaterial->baseMaterial(resLitMaterialTemplate.loadAndGetAsRef());
            }
        }

        for (const auto& param : sourceMaterial.parameters.parameters())
        {
            if (param.data.type() == GetTypeObject<StringBuf>())
            {
                StringBuf texturePath;
                if (param.data.get(texturePath))
                {
                    const auto texture = LoadResource<ITexture>(texturePath);
                    exportInfo.baseMaterial->writeParameter(param.name, texture);
                    TRACE_INFO("Bound texture '{}' to '{}'", texture.key(), param.name);
                }
            }
        }

        auto loader = GetService<LoadingService>()->loader();
        exportInfo.material = rtti_cast<MaterialInstance>(exportInfo.baseMaterial->clone(nullptr, loader));

        // apply material overrides on material as specified in the manifest
        if (const auto materialInfo = settings.materialManifest->findMaterial(exportInfo.name))
            materialInfo->applyOn(*exportInfo.material);
    }
}*/

//--

void ExportChunks(const BuildChunkRegistry& builder, Array<MeshChunk>& outChunks)
{
    outChunks.reserve(builder.m_buildChunks.size());

    for (auto sourceChunk : builder.m_buildChunks)
    {
        auto& exportChunk = outChunks.emplaceBack();
        exportChunk.detailMask = sourceChunk->m_detailMask;
        exportChunk.renderMask = sourceChunk->m_renderMask;
        exportChunk.indexCount = sourceChunk->m_finalIndexCount;
        exportChunk.vertexCount = sourceChunk->m_finalVertexCount;
        exportChunk.materialIndex = sourceChunk->m_material;
        exportChunk.vertexFormat = sourceChunk->m_format;
        exportChunk.quantizationOffset = sourceChunk->m_quantizationOffset;
        exportChunk.quantizationScale = sourceChunk->m_quantizationScale;
        exportChunk.packedVertexData.bind(sourceChunk->m_packedVertexData, CompressionType::LZ4HC);
        exportChunk.packedIndexData.bind(sourceChunk->m_packedIndexData, CompressionType::LZ4HC);
        exportChunk.unpackedVertexSize = sourceChunk->m_unpackedVertexDataSize;
        exportChunk.unpackedIndexSize = sourceChunk->m_unpackedIndexDataSize;
    }
}

//--

bool BuildChunks(const Array<MeshRawChunk>& sourceChunks, const MeshImportConfig& settings, IProgressTracker& progressTracker, Array<MeshChunk>& outRenderChunks, Box& outBounds)
{
    PC_SCOPE_LVL0(BuildChunks);

    // generate import chunks
    ImportChunkRegistry importChunks;
    ExtractImportChunks(sourceChunks, importChunks);

    // generate/replace attributes
    if (!CalculateNormals(importChunks, settings, progressTracker))
        return false;
    if (!CalculateTangents(importChunks, settings, progressTracker))
        return false;
    if (!CalculateUVS(importChunks, settings))
        return false;

    // prepare the build list
    BuildChunkRegistry buildChunks;
    GenerateBuildChunks(importChunks, buildChunks, settings);

    // pack the data
    if (!PackData(buildChunks, settings, progressTracker))
        return nullptr;

    // compute bounds
    outBounds.clear();
    for (const auto& chunk : sourceChunks)
        outBounds.merge(chunk.bounds);

    // prevent invalid bounds
    if (outBounds.empty())
        outBounds = Box(Vector3(0, 0, 0), 1.0f);

    // export final chunks
    ExportChunks(buildChunks, outRenderChunks);
    return true;
}
     
//---
    
END_BOOMER_NAMESPACE_EX(assets)
