/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import #]
***/

#include "build.h"
#include "importerMtl.h"
#include "importerObj.h"
#include "fileMtl.h"
#include "fileObj.h"

#include "import/mesh_loader/include/renderingMeshCooker.h"

#include "engine/mesh/include/streamBuilder.h"
#include "engine/material/include/materialInstance.h"
#include "engine/mesh/include/mesh.h"

#include "core/io/include/fileHandle.h"
#include "core/io/include/io.h"
#include "core/app/include/localServiceContainer.h"
#include "core/resource/include/resource.h"
#include "core/resource/include/loader.h"
#include "core/containers/include/inplaceArray.h"
#include "core/resource/include/tags.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

ConfigProperty<bool> cvAllowWavefrontThreads("Wavefront.Importer", "AllowThreads", true);

//--
RTTI_BEGIN_TYPE_ENUM(OBJMeshAttributeMode);
    RTTI_ENUM_OPTION_WITH_HINT(Always, "Attribute will be always emitted, even if it contains no data (does not make sense but zeros compress well, so..)");
    RTTI_ENUM_OPTION_WITH_HINT(IfPresentAnywhere, "Attribute will be emitted to a mesh if at least one input group from .obj contains it");
    RTTI_ENUM_OPTION_WITH_HINT(IfPresentEverywhere, "Attribute will be emitted to a mesh ONLY if ALL groups in a build group contain it (to avoid uninitialized data));");
    RTTI_ENUM_OPTION_WITH_HINT(Always, "Attribute will never be emitted");
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_CLASS(OBJMeshImportConfig);
    RTTI_OLD_NAME("wavefront::OBJMeshImportConfig");
    RTTI_CATEGORY("Topology");
    RTTI_PROPERTY(forceTriangles).editable("Force triangles at output, even if we had quads").overriddable();
    RTTI_CATEGORY("Mesh streams");
    RTTI_PROPERTY(emitNormals).editable("Should we emit the loaded normals").overriddable();
    RTTI_PROPERTY(emitUVs).editable("Should we emit the loaded UVs").overriddable();
    RTTI_PROPERTY(emitColors).editable("Should we emit the loaded colors").overriddable();
    RTTI_CATEGORY("Texture mapping");
    RTTI_PROPERTY(flipUV).editable("Flip V channel of the UVs").overriddable();
    RTTI_PROPERTY(uvScale).editable("Additional UV scale to apply").overriddable();
    RTTI_CATEGORY("Selective import");
    RTTI_PROPERTY(objectFilter).editable("Import only those objects").overriddable();
    RTTI_PROPERTY(groupFilter).editable("Import only those groups").overriddable();
RTTI_END_TYPE();

OBJMeshImportConfig::OBJMeshImportConfig()
{
}

//--

RTTI_BEGIN_TYPE_CLASS(OBJMeshImporter);
    RTTI_OLD_NAME("wavefront::OBJMeshImporter");
    RTTI_METADATA(ResourceImportedClassMetadata).addClass<Mesh>();
    RTTI_METADATA(ResourceSourceFormatMetadata).addSourceExtension("obj");
    RTTI_METADATA(ResourceCookerVersionMetadata).version(3);
    RTTI_METADATA(ResourceImporterConfigurationClassMetadata).configurationClass<OBJMeshImportConfig>();
RTTI_END_TYPE();

OBJMeshImporter::OBJMeshImporter()
{
}

enum class GroupBuildModelType
{
    Visual,
    ConvexCollision,
    TriangleCollision,
    Occlusion,
};

struct GroupBuildRenderGroupKey
{
    uint16_t material = 0; // index into the material table

    MeshStreamMask streams = MeshStreamMaskFromType(MeshStreamType::Position_3F);
    MeshTopologyType topology = MeshTopologyType::Triangles;

    INLINE GroupBuildRenderGroupKey() {};
    INLINE GroupBuildRenderGroupKey(const GroupBuildRenderGroupKey& other) = default;
    INLINE GroupBuildRenderGroupKey& operator=(const GroupBuildRenderGroupKey& other) = default;

    INLINE bool operator==(const GroupBuildRenderGroupKey& other) const
    {
        return (streams == other.streams) && (topology == other.topology) && (material == other.material);
    }
};

struct GroupBuildRenderGroup
{
    GroupBuildRenderGroupKey key;
    Array<const GroupChunk*> sourceChunks;
};

struct GroupBuildModel
{
    GroupBuildModelType type = GroupBuildModelType::Visual;
    uint32_t detailMask = 1;

    Array<GroupBuildRenderGroup> groups;
        
    GroupBuildRenderGroup* getBestGroup(const GroupBuildRenderGroupKey& key)
    {
        for (auto& group : groups)
            if (group.key == key)
                return &group;

        auto& group = groups.emplaceBack();
        group.key = key;
        return &group;
    }
};

struct GroupBuildModelList
{
    Array<GroupBuildModel> models;

    GroupBuildModel* getModel(GroupBuildModelType type, uint32_t mask)
    {
        for (auto& model : models)
            if (model.type == type && model.detailMask == mask)
                return &model;

        auto& model = models.emplaceBack();
        model.detailMask = mask;
        model.type = type;
        return &model;
    }
};

static bool DetermineIfAttributeShouldBeAllowed(OBJMeshAttributeMode mode, bool anyHasIt, bool allHasIt)
{
    switch (mode)
    {
        case OBJMeshAttributeMode::Always: return true;
        case OBJMeshAttributeMode::IfPresentAnywhere: return anyHasIt;
        case OBJMeshAttributeMode::IfPresentEverywhere: return allHasIt;
        case OBJMeshAttributeMode::Never: return false;
    }

    return false;
}

static GroupBuildRenderGroupKey DetermineGroupKey(const GroupChunk& chunk, OBJMeshAttributeMode uvMode, OBJMeshAttributeMode colorMode, OBJMeshAttributeMode normalMode, bool forceTriangles)
{
    GroupBuildRenderGroupKey key;
    key.material = chunk.material;

    // determine topology
    auto quadsPossible = (chunk.commonFaceVertexCount) == 4;
    if (quadsPossible && !forceTriangles )
        key.topology = MeshTopologyType::Quads;
    else
        key.topology = MeshTopologyType::Triangles;

    // streams
    key.streams = MeshStreamMaskFromType(MeshStreamType::Position_3F);
    if (DetermineIfAttributeShouldBeAllowed(uvMode, chunk.attributeMask & AttributeBit::UVStream, chunk.attributeMask & AttributeBit::UVStream))
        key.streams |= MeshStreamMaskFromType(MeshStreamType::TexCoord0_2F);
    if (DetermineIfAttributeShouldBeAllowed(normalMode, chunk.attributeMask & AttributeBit::NormalStream, chunk.attributeMask & AttributeBit::NormalStream))
        key.streams |= MeshStreamMaskFromType(MeshStreamType::Normal_3F);
    if (DetermineIfAttributeShouldBeAllowed(colorMode, chunk.attributeMask & AttributeBit::ColorStream, chunk.attributeMask & AttributeBit::ColorStream))
        key.streams |= MeshStreamMaskFromType(MeshStreamType::Color0_4U8);

    return key;
}
    
static GroupBuildModelType DetermineModelType(StringView name, uint8_t& outLodIndex)
{
    if (name.endsWithNoCase("_LOD0"))
    {
        outLodIndex = 0;
        return GroupBuildModelType::Visual;
    }
    else if (name.endsWithNoCase("_LOD1"))
    {
        outLodIndex = 1;
        return GroupBuildModelType::Visual;
    }
    else if (name.endsWithNoCase("_LOD2"))
    {
        outLodIndex = 2;
        return GroupBuildModelType::Visual;
    }
    else if (name.endsWithNoCase("_LOD3"))
    {
        outLodIndex = 3;
        return GroupBuildModelType::Visual;
    }
    else if (name.endsWithNoCase("_LOD4"))
    {
        outLodIndex = 4;
        return GroupBuildModelType::Visual;
    }
    else if (name.endsWithNoCase("_LOD5"))
    {
        outLodIndex = 5;
        return GroupBuildModelType::Visual;
    }
    else if (name.endsWithNoCase("_LOD6"))
    {
        outLodIndex = 6;
        return GroupBuildModelType::Visual;
    }
    else if (name.endsWithNoCase("_LOD7"))
    {
        outLodIndex = 7;
        return GroupBuildModelType::Visual;
    }
    else if (name.endsWithNoCase("_Col") || name.beginsWithNoCase("UCX_"))
    {
        return GroupBuildModelType::ConvexCollision;
    }
    else if (name.endsWithNoCase("_TriCol"))
    {
        return GroupBuildModelType::TriangleCollision;
    }
    else if (name.endsWithNoCase("_Occlusion") || name.beginsWith("OCC_"))
    {
        return GroupBuildModelType::Occlusion;
    }

    outLodIndex = 0;
    return GroupBuildModelType::Visual;
}

static void PrepareGroupBuildList(const SourceAssetOBJ& data, OBJMeshAttributeMode uvMode, OBJMeshAttributeMode colorMode, OBJMeshAttributeMode normalMode, StringView objectFilter, StringView groupFilter, bool forceTriangles, GroupBuildModelList& outModelList)
{
    outModelList.models.reserve(data.groups().size());

    for (auto& obj : data.objects())
    {
        if (!objectFilter.empty() && 0 != objectFilter.caseCmp(obj.name))
            continue;

        for (uint32_t i = 0; i < obj.numGroups; ++i)
        {
            auto& group = data.groups().typedData()[obj.firstGroup + i];

            if (!groupFilter.empty() && 0 != groupFilter.caseCmp(group.name))
                continue;

            uint8_t lodIndex = 0;
            auto groupType = DetermineModelType(group.name, lodIndex);

            auto* model = outModelList.getModel(groupType, 1U << lodIndex);

            for (uint32_t k = 0; k < group.numChunks; ++k)
            {
                const auto* groupChunk = data.chunks().typedData() + group.firstChunk + k;
                const auto groupChunkKey = DetermineGroupKey(*groupChunk, uvMode, colorMode, normalMode, forceTriangles);

                auto* group = model->getBestGroup(groupChunkKey);
                group->sourceChunks.pushBack(groupChunk);
            }
        }
    }
}

//--

static Box CalculateGeometryBounds(const SourceAssetOBJ& data, const GroupBuildModelList& buildList, const BaseTransformation& assetToEngine)
{
    struct PerChunkBox
    {
        const GroupChunk* chunk = nullptr;
        Box bounds;
        bool valid = true;
    };

    Array<PerChunkBox> jobs;
    jobs.reserve(data.chunks().size());

    for (const auto& model : buildList.models)
    {
        for (const auto& group : model.groups)
        {
            for (const auto* chunk : group.sourceChunks)
            {
                auto& job = jobs.emplaceBack();
                job.chunk = chunk;
            }
        }
    }

    RunFiberForFeach<PerChunkBox>("ComputeChunkBounds", jobs, -2, [&assetToEngine, &data](PerChunkBox& job)
        {
            const auto* chunk = job.chunk;

            auto positions = data.positions();

            auto face = data.faces() + chunk->firstFace;
            auto faceEnd = face + chunk->numFaces;

            auto faceIndices = data.faceIndices() + chunk->firstFaceIndex;

            float minX = FLT_MAX;
            float minY = FLT_MAX;
            float minZ = FLT_MAX;
            float maxX = -FLT_MAX;
            float maxY = -FLT_MAX;
            float maxZ = -FLT_MAX;

            const float validBounds = 1000000.0f;

            while (face < faceEnd)
            {
                for (uint32_t i = 0; i < face->numVertices; ++i)
                {
                    auto pos = assetToEngine.transformPoint(positions[faceIndices[0]]);

                    DEBUG_CHECK(pos.x >= -validBounds && pos.x <= validBounds);
                    DEBUG_CHECK(pos.y >= -validBounds && pos.y <= validBounds);
                    DEBUG_CHECK(pos.z >= -validBounds && pos.z <= validBounds);

                    minX = std::min<float>(minX, pos.x);
                    minY = std::min<float>(minY, pos.y);
                    minZ = std::min<float>(minZ, pos.z);

                    maxX = std::max<float>(maxX, pos.x);
                    maxY = std::max<float>(maxY, pos.y);
                    maxZ = std::max<float>(maxZ, pos.z);

                    //job.x += pos.x;
                    //job.y += pos.y;
                    //job.z += pos.z;

                    faceIndices += chunk->numAttributes;
                }

                //count += face->numVertices;
                face += 1;
            }
                
            if (minX < -validBounds || minY < -validBounds || minZ < -validBounds || maxX > validBounds || maxY > validBounds || maxZ > validBounds)
            {
                job.valid = false;
            }
            else
            {
                job.valid = true;
                job.bounds.min.x = minX;
                job.bounds.min.y = minY;
                job.bounds.min.z = minZ;
                job.bounds.max.x = maxX;
                job.bounds.max.y = maxY;
                job.bounds.max.z = maxZ;
            }

        });

    Box bounds;
    for (const auto& job : jobs)
        if (job.valid)
            bounds.merge(job.bounds);

    return bounds;
}

//--

static uint32_t CountVerticesNeededChunk(const MeshTopologyType top, const SourceAssetOBJ& data, const GroupChunk& sourceChunk)
{
    uint32_t numWrittenVertices = 0;

    TRACE_INFO("Wavefront chunk '{}': A:{} F:{} FI:{}", data.materialReferences()[sourceChunk.material].name, sourceChunk.numAttributes, sourceChunk.numFaces, sourceChunk.numFaceIndices);

    DEBUG_CHECK_EX(sourceChunk.numFaceIndices % sourceChunk.numAttributes == 0, "Strange number of face indices");
    if (top == MeshTopologyType::Triangles)
    {
        if (sourceChunk.commonFaceVertexCount == 3)
        {
            numWrittenVertices = (sourceChunk.numFaceIndices / sourceChunk.numAttributes);
        }
        else if (sourceChunk.commonFaceVertexCount == 4)
        {
            numWrittenVertices = 6 * ((sourceChunk.numFaceIndices / sourceChunk.numAttributes) / 4);
        }
        else
        {
            auto face  = data.faces() + sourceChunk.firstFace;
            auto faceEnd  = face + sourceChunk.numFaces;

            while (face < faceEnd)
            {
                numWrittenVertices += (face->numVertices - 2) * 3;
                ++face;
            }
        }
    }
    else if (top == MeshTopologyType::Quads)
    {
        DEBUG_CHECK(sourceChunk.commonFaceVertexCount == 4);
        numWrittenVertices = sourceChunk.numFaceIndices / sourceChunk.numAttributes;
    }

    return numWrittenVertices;
}

static uint32_t CountVerticesNeeded(const MeshTopologyType top, const SourceAssetOBJ& data, const Array<const GroupChunk*>& sourceChunks)
{
    uint32_t numVertices = 0;

    for (auto chunk  : sourceChunks)
        numVertices += CountVerticesNeededChunk(top, data, *chunk);

    return numVertices;
}

template< typename T >
static void FillDefaultDataZero(const MeshTopologyType top, const SourceAssetOBJ& data, const GroupChunk& sourceChunk, T*& writeRawPtr)
{
    auto vertexCount = CountVerticesNeededChunk(top, data, sourceChunk);
    memset(writeRawPtr, 0, sizeof(T) * vertexCount);
    writeRawPtr += vertexCount;
}

template< typename T >
static void FillDefaultDataOnes(const MeshTopologyType top, const SourceAssetOBJ& data, const GroupChunk& sourceChunk, T*& writeRawPtr)
{
    auto vertexCount = CountVerticesNeededChunk(top, data, sourceChunk);
    memset(writeRawPtr, 255, sizeof(T) * vertexCount);
    writeRawPtr += vertexCount;
}

template< typename T >
static void FlipFaces(T* writePtr, const T* writeEndPtr, const MeshTopologyType top)
{
    if (top == MeshTopologyType::Triangles)
    {
        while (writePtr < writeEndPtr)
        {
            std::swap(writePtr[0], writePtr[2]);
            writePtr += 3;
        }
    }
    else if (top == MeshTopologyType::Quads)
    {
        while (writePtr < writeEndPtr)
        {
            std::swap(writePtr[0], writePtr[3]);
            std::swap(writePtr[1], writePtr[2]);
            writePtr += 4;
        }
    }
}

template< typename T >
static void ExtractStreamData(uint32_t attributeIndex, const void* readRawPtr, T*& writeRawPtr, const GroupChunk& sourceChunk, const MeshTopologyType top, const SourceAssetOBJ& data)
{
    auto readPtr  = (const T*)readRawPtr;
    auto writePtr  = (T*)writeRawPtr;

    auto faceIndices  = data.faceIndices() + sourceChunk.firstFaceIndex;
    auto faceIndicesEnd  = faceIndices + sourceChunk.numFaceIndices;

    if (top == MeshTopologyType::Triangles)
    {
        auto step = sourceChunk.numAttributes;
        if (sourceChunk.commonFaceVertexCount == 3)
        {
            while (faceIndices < faceIndicesEnd)
            {
                *writePtr++ = readPtr[faceIndices[attributeIndex]];
                faceIndices += step;
            }
        }
        else if (sourceChunk.commonFaceVertexCount == 4)
        {
            while (faceIndices < faceIndicesEnd)
            {
                *writePtr++ = readPtr[faceIndices[attributeIndex ]];
                *writePtr++ = readPtr[faceIndices[attributeIndex + step]];
                *writePtr++ = readPtr[faceIndices[attributeIndex + 2*step]];

                *writePtr++ = readPtr[faceIndices[attributeIndex]];
                *writePtr++ = readPtr[faceIndices[attributeIndex + 2*step]];
                *writePtr++ = readPtr[faceIndices[attributeIndex + 3*step]];

                faceIndices += 4 * step;
            }
        }
        else
        {
            auto face  = data.faces() + sourceChunk.firstFace;
            auto faceEnd  = face + sourceChunk.numFaces;

            while (face < faceEnd)
            {
                auto prev = attributeIndex + step;
                auto cur = prev + step;
                for (uint32_t i = 2; i < face->numVertices; ++i)
                {
                    *writePtr++ = readPtr[faceIndices[attributeIndex]];
                    *writePtr++ = readPtr[faceIndices[prev]];
                    *writePtr++ = readPtr[faceIndices[cur]];
                    prev += step;
                    cur += step;
                }

                faceIndices += face->numVertices * sourceChunk.numAttributes;
                ++face;
            }
        }
    }
    else if (top == MeshTopologyType::Quads)
    {
        ASSERT_EX(sourceChunk.commonFaceVertexCount == 4, "Invalid vertex count for a face");

        auto step = sourceChunk.numAttributes;
        while (faceIndices < faceIndicesEnd)
        {
            *writePtr++ = readPtr[faceIndices[attributeIndex]];
            faceIndices += step;
        }
    }
    else
    {
        DEBUG_CHECK(!"Invalid topology");
    }

    writeRawPtr = writePtr;
}

static void ProcessSingleChunk(MeshTopologyType top, const SourceAssetOBJ& data, const BaseTransformation& assetToEngine, const Array<const GroupChunk*>& sourceChunks, MeshStreamMask streams, bool flipFaces, bool flipUV, const Vector2& uvScale, MeshRawChunk& outChunk)
{
    PC_SCOPE_LVL0(ProcessSingleChunk);

    struct SourceChunkJobInfo
    {
        uint32_t vertexCount = 0;
        const GroupChunk* sourceChunk = nullptr;
        Vector3* positionWritePtr = nullptr;

        char normalAttributeIndex = -1;
        char uvAttributeIndex = -1;
        char colorAttributeIndex = -1;

        Vector3* normalWritePtr = nullptr;
        Vector2* uvWritePtr = nullptr;
        Color* colorWritePtr = nullptr;
    };

    // get data pointers to the source OBJ data (all vertices are using absolute indices so this can be set only once)
    auto positions = data.positions();
    auto normals = data.normals();
    auto uvs = data.uvs();
    auto colors = data.colors();

    // validate
    for (const auto* sourceChunk : sourceChunks)
    {
        const auto* f = data.faces() + sourceChunk->firstFace;
        DEBUG_CHECK(sourceChunk->firstFace + sourceChunk->numFaces <= data.numFaces());
        const auto* fi = data.faceIndices() + sourceChunk->firstFaceIndex;
        const auto* maxFi = fi + sourceChunk->numFaceIndices;
        DEBUG_CHECK(sourceChunk->firstFaceIndex + sourceChunk->numFaceIndices <= data.numFaceIndices());
        const auto numFaces = sourceChunk->numFaces;
        for (uint32_t i=0; i<numFaces; ++f, ++i)
        { 
            uint8_t numAttributes = 0;

            if (f->attributeMask & 1) numAttributes += 1;
            if (f->attributeMask & 2) numAttributes += 1;
            if (f->attributeMask & 4) numAttributes += 1;
            if (f->attributeMask & 8) numAttributes += 1;

            for (uint32_t j = 0; j < f->numVertices; ++j)
            {
                DEBUG_CHECK(fi < maxFi);

                if (f->attributeMask & 1)
                {
                    const auto index = *fi++;
                    DEBUG_CHECK(index < data.numPosition());
                }

                if (f->attributeMask & 2)
                {
                    const auto index = *fi++;
                    DEBUG_CHECK(index < data.numUVS());
                }

                if (f->attributeMask & 4)
                {
                    const auto index = *fi++;
                    DEBUG_CHECK(index < data.numNormals());
                }

                if (f->attributeMask & 8)
                {
                    const auto index = *fi++;
                    DEBUG_CHECK(index < data.numColors());
                }
            }
        }
    }

    // count total needed vertices
    auto numVertices = CountVerticesNeeded(top, data, sourceChunks);

    // count vertices in the source chunks
    MeshRawStreamBuilder builder(top);
    builder.reserveVertices(numVertices, streams);
    builder.numVeritces = numVertices; // we use all what we reserved

    // prepare source jobs
    InplaceArray<SourceChunkJobInfo, 64> sourceJobsInfo;
    sourceJobsInfo.reserve(sourceChunks.size());
    {
        // get write pointers, each chunk will append data
        auto writePos = builder.vertexData<MeshStreamType::Position_3F>();
        auto writeUV = builder.vertexData<MeshStreamType::TexCoord0_2F>();
        auto writeNormals = builder.vertexData<MeshStreamType::Normal_3F>();
        auto writeColors = builder.vertexData<MeshStreamType::Color0_4U8>();
        for (auto chunk : sourceChunks)
        {
            uint32_t runningAttributeIndex = 0;

            auto& jobInfo = sourceJobsInfo.emplaceBack();
            jobInfo.sourceChunk = chunk;
            jobInfo.vertexCount = CountVerticesNeededChunk(top, data, *chunk);

            // extract positions
            {
                if (streams & MeshStreamMaskFromType(MeshStreamType::Position_3F))
                {
                    jobInfo.positionWritePtr = writePos;
                    writePos += jobInfo.vertexCount;
                }
                runningAttributeIndex += 1;
            }

            // extract UVs
            if (chunk->attributeMask & AttributeBit::UVStream)
            {
                if (streams & MeshStreamMaskFromType(MeshStreamType::TexCoord0_2F))
                {
                    jobInfo.uvAttributeIndex = runningAttributeIndex;
                    jobInfo.uvWritePtr = writeUV;
                    writeUV += jobInfo.vertexCount;
                }
                runningAttributeIndex += 1;
            }
            else if (streams & MeshStreamMaskFromType(MeshStreamType::TexCoord2_2F))
            {
                jobInfo.uvWritePtr = writeUV;
                writeUV += jobInfo.vertexCount;
            }

            // extract normals
            if (chunk->attributeMask & AttributeBit::NormalStream)
            {
                if (streams & MeshStreamMaskFromType(MeshStreamType::Normal_3F))
                {
                    jobInfo.normalAttributeIndex = runningAttributeIndex;
                    jobInfo.normalWritePtr = writeNormals;
                    writeNormals += jobInfo.vertexCount;
                }

                runningAttributeIndex += 1;
            }
            else if (streams & MeshStreamMaskFromType(MeshStreamType::Normal_3F))
            {
                jobInfo.normalWritePtr = writeNormals;
                writeNormals += jobInfo.vertexCount;
            }

            // extract colors
            if (chunk->attributeMask & AttributeBit::ColorStream)
            {
                if (streams & MeshStreamMaskFromType(MeshStreamType::Color0_4U8))
                {
                    jobInfo.colorAttributeIndex = runningAttributeIndex;
                    jobInfo.colorWritePtr = writeColors;
                    writeColors += jobInfo.vertexCount;
                }
                runningAttributeIndex += 1;
            }
            else if (streams & MeshStreamMaskFromType(MeshStreamType::Color0_4U8))
            {
                jobInfo.colorWritePtr = writeColors;
                writeColors += jobInfo.vertexCount;
            }
        }
    }

    // copy out the attributes for each chunk
    auto jobCounter = CreateFence("ChunkExportStream", sourceJobsInfo.size());
    RunChildFiber("ChunkExportStream").invocations(sourceJobsInfo.size()) << [jobCounter, flipUV, uvScale, flipFaces, top, &assetToEngine, &data, streams, positions, uvs, colors, normals, &sourceJobsInfo, &builder](FIBER_FUNC)
    {
        const auto& jobInfo = sourceJobsInfo[index];

        // extract positions
        {
            if (streams & MeshStreamMaskFromType(MeshStreamType::Position_3F))
            {
                auto* startPos = jobInfo.positionWritePtr;
                auto* writePos = startPos;
                ExtractStreamData(0, positions, writePos, *jobInfo.sourceChunk, top, data);

                if (flipFaces)
                    FlipFaces(jobInfo.positionWritePtr, writePos, top);

                while (startPos < writePos)
                {
                    *startPos = assetToEngine.transformPoint(*startPos);
                    startPos += 1;
                }
            }
        }

        // extract UVs
        if (auto* writeUV = jobInfo.uvWritePtr)
        {
            if (jobInfo.uvAttributeIndex != -1)
            {
                auto* startUV = writeUV;
                ExtractStreamData(jobInfo.uvAttributeIndex, uvs, writeUV, *jobInfo.sourceChunk, top, data);

                if (flipFaces)
                    FlipFaces(jobInfo.uvWritePtr, writeUV, top);

                if (flipUV)
                {
                    while (startUV < writeUV)
                    {
                        startUV->y = 1.0f - startUV->y;
                        startUV->x = startUV->x * uvScale.x;
                        startUV->y = startUV->y * uvScale.y;
                        startUV += 1;
                    }
                }
                else
                {
                    while (startUV < writeUV)
                    {
                        startUV->x = startUV->x * uvScale.x;
                        startUV->y = startUV->y * uvScale.y;
                        startUV += 1;
                    }
                }
            }
            else
            {
                FillDefaultDataZero(top, data, *jobInfo.sourceChunk, writeUV);
            }
        }

        // extract normals
        if (auto* writeNormals = jobInfo.normalWritePtr)
        {
            if (jobInfo.normalAttributeIndex != -1)
            {
                auto* startNormal = writeNormals;
                ExtractStreamData(jobInfo.normalAttributeIndex, normals, writeNormals, *jobInfo.sourceChunk, top, data);

                while (startNormal < writeNormals)
                {
                    *startNormal = assetToEngine.transformVector(*startNormal).normalized();
                    startNormal += 1;
                }

                if (flipFaces)
                    FlipFaces(jobInfo.normalWritePtr, writeNormals, top);
            }
            else
                FillDefaultDataZero(top, data, *jobInfo.sourceChunk, writeNormals);
        }

        // extract colors
        if (auto* writeColors = jobInfo.colorWritePtr)
        {
            if (jobInfo.colorAttributeIndex != -1)
            {
                ExtractStreamData(jobInfo.colorAttributeIndex, colors, writeColors, *jobInfo.sourceChunk, top, data);

                if (flipFaces)
                    FlipFaces(jobInfo.colorWritePtr, writeColors, top);
            }
            else
            {
                FillDefaultDataOnes(top, data, *jobInfo.sourceChunk, writeColors);
            }
        }

        SignalFence(jobCounter);
    };

    WaitForFence(jobCounter);

    // extract chunk data
    builder.extract(outChunk);
}

//--

void OBJMeshImporter::buildMaterials(const SourceAssetOBJ& data, const Mesh* existingMesh, IResourceImporterInterface& importer, const GroupBuildModelList& exportGeometry, Array<int> &outSourceToExportMaterialIndexMapping, Array<MeshMaterial>& outExportMaterials) const
{
    auto meshGeometryManifest = importer.queryConfigration<OBJMeshImportConfig>();

    // find max material index actually used inside chunks
    uint32_t maxSourceMaterialIndex = 0;
    for (auto& sourceChunk : data.chunks())
        maxSourceMaterialIndex = std::max<uint32_t>(maxSourceMaterialIndex, sourceChunk.material);

    // prepare the mapping table
    outSourceToExportMaterialIndexMapping.clear();
    outSourceToExportMaterialIndexMapping.resizeWith(maxSourceMaterialIndex + 1, -1);

    // look at the content we cant to export and make sure the necessary materials will be exported
    for (const auto& model : exportGeometry.models)
    {
        // we are not interested on materials used on the non-visual groups
        if (model.type != GroupBuildModelType::Visual)
            continue;

        for (const auto& group : model.groups)
        {
            for (const auto* chunk : group.sourceChunks)
            {
                // add material to export list on first use
                if (-1 == outSourceToExportMaterialIndexMapping[chunk->material])
                {
                    const auto& sourceMaterial = data.materialReferences()[chunk->material];

                    auto materialIndex = outExportMaterials.size();
                    outSourceToExportMaterialIndexMapping[chunk->material] = materialIndex;
                    TRACE_INFO("Mapped source material '{}' ({}) at export index {}", sourceMaterial.name, chunk->material, materialIndex);

                    auto& exportMaterial = outExportMaterials.emplaceBack();
                    exportMaterial.name = StringID(sourceMaterial.name.view());
                    exportMaterial.material = buildSingleMaterial(importer, *meshGeometryManifest, sourceMaterial.name, data.materialLibraryFileName(), chunk->material, existingMesh);
                }
            }
        }
    }

    TRACE_INFO("Exported {} material of {} total in file", outExportMaterials.size(), data.materialReferences().size());
}

struct PackingJob
{
    Array<const GroupChunk*> sourceChunks;
    MeshChunk chunk;
    MeshTopologyType topology;
    MeshStreamMask streams;
};

static MeshVertexFormat ChooseVertexFormat(const GroupBuildRenderGroup& group)
{
    return MeshVertexFormat::Static;
}
        
static void PreparePackingJobs(const GroupBuildModelList& exportGeometry, const Array<int>& sourceToExportMaterialIndexMapping, Array<PackingJob>& outJobs)
{
    uint32_t numChunks = 0;
    for (const auto& model : exportGeometry.models)
        for (const auto& group : model.groups)
            numChunks += 1;

    outJobs.reserve(numChunks);

    for (const auto& model : exportGeometry.models)
    {
        if (model.type != GroupBuildModelType::Visual)
            continue;

        for (const auto& group : model.groups)
        {
            auto& job = outJobs.emplaceBack();
            job.topology = group.key.topology;
            job.streams = group.key.streams;
            job.chunk.vertexFormat = ChooseVertexFormat(group);
            job.chunk.materialIndex = sourceToExportMaterialIndexMapping[group.key.material];
            job.chunk.detailMask = model.detailMask;
            job.chunk.renderMask = (uint32_t)MeshChunkRenderingMaskBit::DEFAULT;
            job.sourceChunks = group.sourceChunks;
        }
    }
}

static void BuildRenderChunks(const SourceAssetOBJ& sourceData, const Matrix assetToEngineTransform, const OBJMeshImportConfig& config, IResourceImporterInterface& importer, const GroupBuildModelList& exportGeometry, const Array<int>& sourceToExportMaterialIndexMapping, Array<MeshChunk>& outChunks, Box& outBounds)
{
    ScopeTimer packingTime;

    // prepare list of packing jobs
    Array<PackingJob> packingJobs;
    PreparePackingJobs(exportGeometry, sourceToExportMaterialIndexMapping, packingJobs);
    TRACE_INFO("Grouped chunks into {} packing jobs", packingJobs.size());

    // output chunks
    Array<MeshRawChunk> rawChunks;
    rawChunks.resize(packingJobs.size());

    // convert wavefront data into mesh streams
    RunFiberLoop("ProcessModels", packingJobs.size(), -2, [&packingJobs, &rawChunks, &sourceData, &assetToEngineTransform, &config](uint32_t jobIndex)
        {
            const auto& job = packingJobs[jobIndex];
            ProcessSingleChunk(job.topology, sourceData, assetToEngineTransform, job.sourceChunks, job.streams, config.flipFaces, config.flipUV, config.uvScale, rawChunks[jobIndex]);
            rawChunks[jobIndex].detailMask = job.chunk.detailMask;
            rawChunks[jobIndex].renderMask = (MeshChunkRenderingMask)job.chunk.renderMask;
            rawChunks[jobIndex].materialIndex = job.chunk.materialIndex;
        });

    // pack mesh streams into render chunks
    BuildChunks(rawChunks, config, importer, outChunks, outBounds);
}

static void BuildDistanceLevels(const GroupBuildModelList& exportGeometry, const Matrix assetToEngineTransform, const OBJMeshImportConfig& config, Array<MeshDetailLevel>& outDetailLevels)
{
    uint32_t detailMask = 1;
    for (const auto& model : exportGeometry.models)
        detailMask |= model.detailMask;

    uint32_t numLods = std::clamp<uint32_t>(1 + FloorLog2(detailMask), 1, 8);

    // TODO!
    for (uint32_t i=0; i<numLods; ++i)
    {
        auto& lod = outDetailLevels.emplaceBack();
        lod.rangeMin = 50.0f * i;
        lod.rangeMax = lod.rangeMin + 50.0f;
    }
}

RefPtr<MaterialImportConfig> OBJMeshImporter::createMaterialImportConfig(const MeshImportConfig& cfg, StringView name) const
{
    auto ret = RefNew<MTLMaterialImportConfig>();
    ret->m_materialName = StringBuf(name);
    ret->markPropertyOverride("materialName"_id);
    return ret;
}

ResourcePtr OBJMeshImporter::importResource(IResourceImporterInterface& importer) const
{
    // load source data from OBJ format
    auto sourceFilePath = importer.queryImportPath();
    auto sourceGeometry = rtti_cast<SourceAssetOBJ>(importer.loadSourceAsset(importer.queryImportPath()));
    if (!sourceGeometry)
        return nullptr;

    // get the configuration for mesh import
    auto meshGeometryManifest = importer.queryConfigration<OBJMeshImportConfig>();
    auto existingData = rtti_cast<Mesh>(importer.existingData());

    // calculate asset transformation to engine space
    auto assetToEngineTransform = meshGeometryManifest->calcAssetToEngineConversionMatrix(1.0f, MeshImportSpace::RightHandYUp); // spec: right handed Y up
    auto allowThreads = meshGeometryManifest->allowThreads && cvAllowWavefrontThreads.get();

    // should we swap faces ?
    auto swapFaces = meshGeometryManifest->flipFaces ^ (assetToEngineTransform.det() < 0.0f);

    ///--

    // TODO: add filtering to export only requested object
    GroupBuildModelList buildList;
    PrepareGroupBuildList(*sourceGeometry, 
        meshGeometryManifest->emitUVs, meshGeometryManifest->emitColors, meshGeometryManifest->emitNormals, 
        meshGeometryManifest->objectFilter, meshGeometryManifest->groupFilter,
        meshGeometryManifest->forceTriangles, buildList);

    // calculate the mesh boundary
    MeshInitData exportData;
    exportData.bounds = CalculateGeometryBounds(*sourceGeometry, buildList, assetToEngineTransform);

    // export materials
    // TODO: only used ones
    Array<int> sourceToExportMaterialMapping;
    buildMaterials(*sourceGeometry, existingData, importer, buildList, sourceToExportMaterialMapping, exportData.materials);

    // export render chunks
    BuildRenderChunks(*sourceGeometry, assetToEngineTransform, *meshGeometryManifest, importer, buildList, sourceToExportMaterialMapping, exportData.chunks, exportData.bounds);

    // export LOD settings
    BuildDistanceLevels(buildList, assetToEngineTransform, *meshGeometryManifest, exportData.detailLevels);

    // collision shapes
    // TODO: extract the collision shapes
    //Array<MeshCollisionShape> shapes;

    // export bones
    //Array<MeshBone> bones;

    // return mesh
    return RefNew<Mesh>(std::move(exportData));
}

#if 0
content::PhysicsDataPtr OBJMeshImporter::buildPhysicsData(const CookingParams& ctx, const content::GeometryData& geometryData) const
{
    // get the rendering blob, we will use it to build physics
    auto renderingBlob = geometryData.renderingBlob();
    if (!renderingBlob)
        return nullptr;

    // no physics
    if (!m_physicsSettings)
        return nullptr;

    // get the file name part
    auto sourceFilePath = importer.queryResourcePath().path();

    // get the lookup token
    auto physicsSettingsCRC = m_physicsSettings->calcSettingsCRC();
    auto lookupToken = StringBuf(TempString("PhysicsData_{}_{}", Hex(renderingBlob->dataCRC()), Hex(physicsSettingsCRC)));

    // load stuff from cache
    data::GeometryBlobShapeMesh renderingBlobMesh(*renderingBlob);
    auto physicsContainer = importer.cacheData<content::PhysicsDataCachedContent>(sourceFilePath, lookupToken, [this, sourceFilePath, &renderingBlobMesh]() -> RefPtr<content::PhysicsDataCachedContent>
    {
        // create data
        auto physicsData = m_physicsSettings->buildFromMesh(renderingBlobMesh);
        if (!physicsData)
            return nullptr;

        auto physicsContainer = RefNew<content::PhysicsDataCachedContent>();
        physicsContainer->data = physicsData;
        physicsData->parent(physicsContainer);
        return physicsContainer;
    });

    // create the mesh access interface
    if (physicsContainer && physicsContainer->data)
        return physicsContainer->data->copy();

    // no data
    return nullptr;
}
#endif



/*void GenerateSkyDome()
{
    StringBuilder str;
    BuilderOBJ obj(str);

    auto numVerticalSegments = 20;
    auto numAngularSegments = 72; // divisible by 24 and 36
    auto radius = 1.0f;

    Array<BuilderOBJ::WriteVertex> previousVertices;
    for (int i=-1; i<=numVerticalSegments; ++i)
    {
        float y = std::max<int>(0,i) / (float)numVerticalSegments;
        float theta = HALFPI * y;

        Array<BuilderOBJ::WriteVertex> currentVertices;
        for (uint32_t j=0; j<=numAngularSegments; ++j)
        {
            float x = j / (float) numAngularSegments;
            float phi = TWOPI * x;

            auto& v = currentVertices.emplaceBack();
            v.uv = obj.writeUV(x, y);

            auto pos = i < 0 ?
                    Vector3(cosf(phi), sinf(phi), -1.0f) :
                    Vector3(cosf(phi) * cosf(theta), sinf(phi) * cosf(theta), sinf(theta));
            v.pos = obj.writePosition(pos.x, pos.y, pos.z);

            auto n = pos.normalized();
            v.n = obj.writeNormal(n.x, n.y, n.z);
        }

        if (i >= 0)
        {
            for (uint32_t j=0; j<numAngularSegments; ++j)
            {
                obj.writeFace(currentVertices[j], currentVertices[j+1], previousVertices[j+1]);
                obj.writeFace(currentVertices[j], previousVertices[j+1], previousVertices[j]);
            }
        }

        previousVertices = currentVertices;
    }

    auto path = AbsolutePath::Build(L"/home/rexdex/skydome.obj");
    SaveFileFromString(path, str.toString());
}*/

END_BOOMER_NAMESPACE_EX(assets)


