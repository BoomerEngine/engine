/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#include "build.h"
#include "wavefrontMeshCooker.h"
#include "wavefrontFormatOBJ.h"
#include "wavefrontFormatMTL.h"

#include "base/io/include/ioFileHandle.h"
#include "base/io/include/utils.h"
#include "base/io/include/ioSystem.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resources/include/resourceCookingInterface.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resourceLoader.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/containers/include/inplaceArray.h"

#include "base/geometry/include/mesh.h"
#include "base/geometry/include/meshStreamBuilder.h"
#include "base/geometry/include/meshManifest.h"

namespace wavefront
{
    //--

    base::ConfigProperty<bool> cvAllowWavefrontThreads("Wavefront.Importer", "AllowThreads", true);

    //--

    RTTI_BEGIN_TYPE_ENUM(MeshGrouppingMethod);
        RTTI_ENUM_OPTION_WITH_HINT(Model, "The whole model is considered \"one mesh\", fastest for processing looses all context data");
        RTTI_ENUM_OPTION_WITH_HINT(Object, "Separate Wavefront \"o Object01\" entries are considered to be meshes");
        RTTI_ENUM_OPTION_WITH_HINT(Group, "Separate Wavefront \"g Part01\" entries are considered to be meshes, creates the most amount of separate meshes");
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(MeshAttributeMode);
        RTTI_ENUM_OPTION_WITH_HINT(Always, "Attribute will be always emitted, even if it contains no data (does not make sense but zeros compress well, so..)");
        RTTI_ENUM_OPTION_WITH_HINT(IfPresentAnywhere, "Attribute will be emitted to a mesh if at least one input group from .obj contains it");
        RTTI_ENUM_OPTION_WITH_HINT(IfPresentEverywhere, "Attribute will be emitted to a mesh ONLY if ALL groups in a build group contain it (to avoid uninitialized data));");
        RTTI_ENUM_OPTION_WITH_HINT(Always, "Attribute will never be emitted");
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_CLASS(MeshManifest);
        RTTI_CATEGORY("Topology");
        RTTI_PROPERTY(groupingMethod).editable("How to arrange the obj groups into mesh models");
        RTTI_PROPERTY(forceTriangles).editable("Force triangles at output, even if we had quads");
        RTTI_CATEGORY("Mesh streams");
        RTTI_PROPERTY(emitNormals).editable("Should we emit the loaded normals");
        RTTI_PROPERTY(emitUVs).editable("Should we emit the loaded UVs");
        RTTI_PROPERTY(emitColors).editable("Should we emit the loaded colors");
        RTTI_CATEGORY("Texture mapping");
        RTTI_PROPERTY(flipUV).editable("Flip V channel of the UVs");        
    RTTI_END_TYPE();

    MeshManifest::MeshManifest()
    {}

    //--

    RTTI_BEGIN_TYPE_CLASS(MeshCooker);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<base::mesh::Mesh>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("obj");
        RTTI_METADATA(base::res::ResourceCookerVersionMetadata).version(3);
    RTTI_END_TYPE();

    MeshCooker::MeshCooker()
    {
    }

    struct GroupBuildList
    {
        base::StringBuf targetName;
        base::Array<const GroupChunk*> sourceChunks;

        base::mesh::MeshStreamMask streams = base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Position_3F);
        base::mesh::MeshTopologyType topology = base::mesh::MeshTopologyType::Triangles;

        base::Vector3 boxMin = base::Vector3::ZERO();
        base::Vector3 boxMax = base::Vector3::ZERO();
        base::Vector3 vertexCenter = base::Vector3::ZERO();

        base::Vector3 pivot = base::Vector3::ZERO();
    };

    struct UniqueNameBuilder
    {
        UniqueNameBuilder()
        {
            m_nameCounts.reserve(64);
        }

        base::StringBuf buildUniqueName(base::StringView<char> name)
        {
            auto shortName = name.trim().trimTailNumbers();
            if (shortName.empty())
                shortName = "group";

            if (auto count  = m_nameCounts.find(shortName))
            {
                *count += 1;
                return base::StringBuf(base::TempString("{}{}", shortName, *count));
            }
            else
            {
                m_nameCounts.set(base::StringBuf(shortName), 0);
                return base::StringBuf(base::TempString("{}0", shortName));
            }
        }

        base::HashMap<base::StringBuf, uint32_t> m_nameCounts;
    };

    static void PrepareGroupBuildList(const FormatOBJ& data, MeshGrouppingMethod method, base::Array<GroupBuildList>& outBuildList)
    {
        UniqueNameBuilder nameBuilder;

        // generate build chunks
        if (method == MeshGrouppingMethod::Model)
        {
            auto& item = outBuildList.emplaceBack();
            item.targetName = "model";

            item.sourceChunks.reserve(data.chunks().size());
            for (auto& chunk : data.chunks())
                item.sourceChunks.pushBack(&chunk);
        }
        else if (method == MeshGrouppingMethod::Object)
        {
            outBuildList.reserve(data.objects().size());

            for (auto& obj : data.objects())
            {
                auto& item = outBuildList.emplaceBack();
                item.targetName = nameBuilder.buildUniqueName(obj.name);

                uint32_t numChunks = 0;
                for (uint32_t i = 0; i < obj.numGroups; ++i)
                    numChunks += data.groups()[obj.firstGroup + i].numChunks;

                item.sourceChunks.reserve(numChunks);
                for (uint32_t i = 0; i < obj.numGroups; ++i)
                {
                    auto& group = data.groups()[obj.firstGroup + i];
                    for (uint32_t j = 0; j < group.numChunks; ++j)
                    {
                        auto chunk  = data.chunks().typedData() + group.firstChunk + j;
                        item.sourceChunks.pushBack(chunk);
                    }
                }
            }
        }
        else if (method == MeshGrouppingMethod::Group)
        {
            outBuildList.reserve(data.groups().size());

            for (auto& obj : data.objects())
            {
                for (uint32_t i = 0; i < obj.numGroups; ++i)
                {
                    auto& group = data.groups().typedData()[obj.firstGroup + i];

                    auto& item = outBuildList.emplaceBack();
                    item.targetName = nameBuilder.buildUniqueName(group.name);
                    item.sourceChunks.reserve(group.numChunks);

                    for (uint32_t k = 0; k < group.numChunks; ++k)
                    {
                        auto chunk  = data.chunks().typedData() + group.firstChunk + k;
                        item.sourceChunks.pushBack(chunk);
                    }
                }
            }
        }
    }

    //--

    static bool DetermineIfAttributeShouldBeAllowed(MeshAttributeMode mode, bool anyHasIt, bool allHasIt)
    {
        switch (mode)
        {
            case MeshAttributeMode::Always: return true;
            case MeshAttributeMode::IfPresentAnywhere: return anyHasIt;
            case MeshAttributeMode::IfPresentEverywhere: return allHasIt;
            case MeshAttributeMode::Never: return false;
        }

        return false;
    }

    static void DetermineAttributeMasks(base::Array<GroupBuildList>& groups, MeshAttributeMode uvMode, MeshAttributeMode colorMode, MeshAttributeMode normalMode, bool forceTriangles)
    {
        for (auto& group : groups)
        {
            // check all input groups and chunks
            uint8_t attributeOrMask = 0;
            uint8_t attributeAndMask = 0xFF;
            uint8_t minVertexCount = 0xFF;
            uint8_t maxVertexCount = 0;
            for (auto src  : group.sourceChunks)
            {
                attributeOrMask |= src->attributeMask;
                attributeAndMask &= src->attributeMask;
                minVertexCount = std::min(minVertexCount, src->commonFaceVertexCount);
                maxVertexCount = std::max(maxVertexCount, src->commonFaceVertexCount);
            }

            // determine topology
            auto quadsPossible = (minVertexCount == 4) && (maxVertexCount == 4);
            if (quadsPossible && !forceTriangles )
                group.topology = base::mesh::MeshTopologyType::Quads;
            else
                group.topology = base::mesh::MeshTopologyType::Triangles;

            // streams
            group.streams = base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Position_3F);
            if (DetermineIfAttributeShouldBeAllowed(uvMode, attributeOrMask & AttributeBit::UVStream, attributeAndMask & AttributeBit::UVStream))
                group.streams |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::TexCoord0_2F);
            if (DetermineIfAttributeShouldBeAllowed(normalMode, attributeOrMask & AttributeBit::NormalStream, attributeAndMask & AttributeBit::NormalStream))
                group.streams |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Normal_3F);
            if (DetermineIfAttributeShouldBeAllowed(colorMode, attributeOrMask & AttributeBit::ColorStream, attributeAndMask & AttributeBit::ColorStream))
                group.streams |= base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Color0_4U8);
        }
    }

    //--

    static void CalculateGeometricalCenter(const FormatOBJ& data, const base::Matrix& assetToEngine, GroupBuildList& buildList)
    {
        auto positions  = data.positions();

        struct PerChunkBox
        {
            const GroupChunk* chunk = nullptr;
            base::Box bounds;
            double centerX = 0.0f;
            double centerY = 0.0f;
            double centerZ = 0.0f;
            uint32_t count = 0;
            bool valid = true;
        };

        base::Array<PerChunkBox> jobs;
        jobs.reserve(buildList.sourceChunks.size());
        for (auto chunk : buildList.sourceChunks)
        {
            auto& job = jobs.emplaceBack();
            job.chunk = chunk;
        }

        auto jobDone = Fibers::GetInstance().createCounter("ComputeChunkBounds", jobs.size());
        RunChildFiber("ComputeChunkBounds").invocations(jobs.size()) << [jobDone, &jobs, positions, &assetToEngine, &data](FIBER_FUNC)
        {
            auto& job = jobs[index];
            const auto* chunk = job.chunk;

            auto face = data.faces() + chunk->firstFace;
            auto faceEnd = face + chunk->numFaces;

            auto faceIndices = data.faceIndices() + chunk->firstFaceIndex;

            float minX = FLT_MAX;
            float minY = FLT_MAX;
            float minZ = FLT_MAX;
            float maxX = -FLT_MAX;
            float maxY = -FLT_MAX;
            float maxZ = -FLT_MAX;

            while (face < faceEnd)
            {
                for (uint32_t i = 0; i < face->numVertices; ++i)
                {
                    auto pos = assetToEngine.transformPoint(positions[faceIndices[0]]);

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


            const float validBounds = 1000000.0f;
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

            Fibers::GetInstance().signalCounter(jobDone);
        };

        Fibers::GetInstance().waitForCounterAndRelease(jobDone);

        bool firstChunk = true;
        for (const auto& job : jobs)
        {
            if (!job.valid)
            {
                TRACE_ERROR("Chunk '{}' has invalid bouds, removing from build", data.materialReferences()[job.chunk->material].name);
                buildList.sourceChunks.remove(job.chunk);
            }
            else if (firstChunk)
            {
                buildList.boxMin = job.bounds.min;
                buildList.boxMax = job.bounds.max;
                firstChunk = false;
            }
            else
            {
                buildList.boxMin = base::Min(buildList.boxMin, job.bounds.min);
                buildList.boxMax = base::Max(buildList.boxMax, job.bounds.max);
            }
        }

        /*double x = 0.0f;
        double y = 0.0f;
        double z = 0.0f;
        uint32_t count = 0;

        if (count > 0)
        {
            x /= (double)count;
            y /= (double)count;
            z /= (double)count;
        }

        buildList.vertexCenter.x = (float)x;
        buildList.vertexCenter.y = (float)y;
        buildList.vertexCenter.z = (float)z;

       

        // compute "center of gravity" for all the vertices, this is the pivot, all vertices will be moved to this space
        // NOTE: this is required to avoid situation when all meshes are built far away from 0,0,0 and mesh instances have a position of 0,0,0
        // NOTE: this is imperfect as we don't have a true positions of the original objects but it's a fair guess
        // NOTE: if 0,0,0 is inside the mesh bounding box we assume that we don't want to pivot it and it's ok as it is
        float margin = 0.1f;
        if (base::Box(buildList.boxMin, buildList.boxMax).extruded(margin).contains(base::Vector3::ZERO()))
            buildList.pivot = base::Vector3::ZERO();
        else*/
        buildList.vertexCenter = base::Vector3::ZERO();
        buildList.pivot = buildList.vertexCenter;
    }

    void CalcGeometryCenters(const FormatOBJ& sourceGeometry, const base::Matrix& assetToEngine, base::Array<GroupBuildList>& buildList, bool allowThreads)
    {
        PC_SCOPE_LVL1(CalcGeometryCenters);

        // compute centers of build groups
        if (buildList.size() > 1 && allowThreads)
        {
            auto signal = Fibers::GetInstance().createCounter("GeometryBounds", buildList.size());

            RunChildFiber("CalcGeometryBounds").invocations(buildList.size()) << [signal, &assetToEngine, &buildList, &sourceGeometry](FIBER_FUNC)
            {
                CalculateGeometricalCenter(sourceGeometry, assetToEngine, buildList[index]);
                Fibers::GetInstance().signalCounter(signal);
            };

            Fibers::GetInstance().waitForCounterAndRelease(signal);
        }
        else
        {
            for (auto& entry : buildList)
                CalculateGeometricalCenter(sourceGeometry, assetToEngine, entry);
        }
    }

    //--

    static uint32_t CountVerticesNeededChunk(const base::mesh::MeshTopologyType top, const FormatOBJ& data, const GroupChunk& sourceChunk)
    {
        uint32_t numWrittenVertices = 0;

        TRACE_INFO("Wavefront chunk '{}': A:{} F:{} FI:{}", data.materialReferences()[sourceChunk.material].name, sourceChunk.numAttributes, sourceChunk.numFaces, sourceChunk.numFaceIndices);

        DEBUG_CHECK_EX(sourceChunk.numFaceIndices % sourceChunk.numAttributes == 0, "Strange number of face indices");
        if (top == base::mesh::MeshTopologyType::Triangles)
        {
            if (sourceChunk.commonFaceVertexCount == 3)
            {
                numWrittenVertices = (sourceChunk.numFaceIndices / sourceChunk.numAttributes);
                //TRACE_INFO(" CASE 0: {} {} {}", numWrittenVertices, sourceChunk.numFaceIndices, sourceChunk.numAttributes);
            }
            else if (sourceChunk.commonFaceVertexCount == 4)
            {
                numWrittenVertices = 6 * ((sourceChunk.numFaceIndices / sourceChunk.numAttributes) / 4);
                //TRACE_INFO(" CASE 1: {} {} {}", numWrittenVertices, sourceChunk.numFaceIndices, sourceChunk.numAttributes);
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

                //TRACE_INFO(" CASE 2: {}", numWrittenVertices);
            }
        }
        else if (top == base::mesh::MeshTopologyType::Quads)
        {
            DEBUG_CHECK(sourceChunk.commonFaceVertexCount == 4);
            numWrittenVertices = sourceChunk.numFaceIndices / sourceChunk.numAttributes;
            //TRACE_INFO(" CASE 3: {}", sourceChunk.numFaceIndices);
        }

        return numWrittenVertices;
    }

    static uint32_t CountVerticesNeeded(const base::mesh::MeshTopologyType top, const FormatOBJ& data, const base::Array<const GroupChunk*>& sourceChunks)
    {
        uint32_t numVertices = 0;

        for (auto chunk  : sourceChunks)
            numVertices += CountVerticesNeededChunk(top, data, *chunk);

        return numVertices;
    }

    template< typename T >
    static void FillDefaultDataZero(const base::mesh::MeshTopologyType top, const FormatOBJ& data, const GroupChunk& sourceChunk, T*& writeRawPtr)
    {
        auto vertexCount = CountVerticesNeededChunk(top, data, sourceChunk);
        memset(writeRawPtr, 0, sizeof(T) * vertexCount);
        writeRawPtr += vertexCount;
    }

    template< typename T >
    static void FillDefaultDataOnes(const base::mesh::MeshTopologyType top, const FormatOBJ& data, const GroupChunk& sourceChunk, T*& writeRawPtr)
    {
        auto vertexCount = CountVerticesNeededChunk(top, data, sourceChunk);
        memset(writeRawPtr, 255, sizeof(T) * vertexCount);
        writeRawPtr += vertexCount;
    }

    template< typename T >
    static void FlipFaces(T* writePtr, const T* writeEndPtr, const base::mesh::MeshTopologyType top)
    {
        if (top == base::mesh::MeshTopologyType::Triangles)
        {
            while (writePtr < writeEndPtr)
            {
                std::swap(writePtr[0], writePtr[2]);
                writePtr += 3;
            }
        }
        else if (top == base::mesh::MeshTopologyType::Quads)
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
    static void ExtractStreamData(uint32_t attributeIndex, const void* readRawPtr, T*& writeRawPtr, const GroupChunk& sourceChunk, const base::mesh::MeshTopologyType top, const FormatOBJ& data)
    {
        auto readPtr  = (const T*)readRawPtr;
        auto writePtr  = (T*)writeRawPtr;

        auto faceIndices  = data.faceIndices() + sourceChunk.firstFaceIndex;
        auto faceIndicesEnd  = faceIndices + sourceChunk.numFaceIndices;

        if (top == base::mesh::MeshTopologyType::Triangles)
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
        else if (top == base::mesh::MeshTopologyType::Quads)
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

    static void ProcessSingleChunk(base::mesh::MeshTopologyType top, const FormatOBJ& data, const base::Matrix& assetToEngine, const base::Array<const GroupChunk*>& sourceChunks, base::mesh::MeshStreamMask streams, bool flipFaces, bool flipUV, base::mesh::MeshModelChunk& outChunk)
    {
        PC_SCOPE_LVL0(ProcessSingleChunk);

        struct SourceChunkJobInfo
        {
            uint32_t vertexCount = 0;
            const GroupChunk* sourceChunk = nullptr;
            base::Vector3* positionWritePtr = nullptr;

            char normalAttributeIndex = -1;
            char uvAttributeIndex = -1;
            char colorAttributeIndex = -1;

            base::Vector3* normalWritePtr = nullptr;
            base::Vector2* uvWritePtr = nullptr;
            base::Color* colorWritePtr = nullptr;
        };

        // get data pointers to the source OBJ data (all vertices are using absolute indices so this can be set only once)
        auto positions = data.positions();
        auto normals = data.normals();
        auto uvs = data.uvs();
        auto colors = data.colors();

        // count total needed vertices
        auto numVertices = CountVerticesNeeded(top, data, sourceChunks);

        // count vertices in the source chunks
        base::mesh::MeshStreamBuilder builder(top);
        builder.reserveVertices(numVertices, streams);
        builder.numVeritces = numVertices; // we use all what we reserved

        // prepare source jobs
        base::InplaceArray<SourceChunkJobInfo, 64> sourceJobsInfo;
        sourceJobsInfo.reserve(sourceChunks.size());
        {
            // get write pointers, each chunk will append data
            auto writePos = builder.vertexData<base::mesh::MeshStreamType::Position_3F>();
            auto writeUV = builder.vertexData<base::mesh::MeshStreamType::TexCoord0_2F>();
            auto writeNormals = builder.vertexData<base::mesh::MeshStreamType::Normal_3F>();
            auto writeColors = builder.vertexData<base::mesh::MeshStreamType::Color0_4U8>();
            for (auto chunk : sourceChunks)
            {
                uint32_t runningAttributeIndex = 0;

                auto& jobInfo = sourceJobsInfo.emplaceBack();
                jobInfo.sourceChunk = chunk;
                jobInfo.vertexCount = CountVerticesNeededChunk(top, data, *chunk);

                // extract positions
                {
                    if (streams & base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Position_3F))
                    {
                        jobInfo.positionWritePtr = writePos;
                        writePos += jobInfo.vertexCount;
                    }
                    runningAttributeIndex += 1;
                }

                // extract UVs
                if (chunk->attributeMask & AttributeBit::UVStream)
                {
                    if (streams & base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::TexCoord0_2F))
                    {
                        jobInfo.uvAttributeIndex = runningAttributeIndex;
                        jobInfo.uvWritePtr = writeUV;
                        writeUV += jobInfo.vertexCount;
                    }
                    runningAttributeIndex += 1;
                }
                else if (streams & base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::TexCoord2_2F))
                {
                    jobInfo.uvWritePtr = writeUV;
                    writeUV += jobInfo.vertexCount;
                }

                // extract normals
                if (chunk->attributeMask & AttributeBit::NormalStream)
                {
                    if (streams & base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Normal_3F))
                    {
                        jobInfo.normalAttributeIndex = runningAttributeIndex;
                        jobInfo.normalWritePtr = writeNormals;
                        writeNormals += jobInfo.vertexCount;
                    }

                    runningAttributeIndex += 1;
                }
                else if (streams & base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Normal_3F))
                {
                    jobInfo.normalWritePtr = writeNormals;
                    writeNormals += jobInfo.vertexCount;
                }

                // extract colors
                if (chunk->attributeMask & AttributeBit::ColorStream)
                {
                    if (streams & base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Color0_4U8))
                    {
                        jobInfo.colorAttributeIndex = runningAttributeIndex;
                        jobInfo.colorWritePtr = writeColors;
                        writeColors += jobInfo.vertexCount;
                    }
                    runningAttributeIndex += 1;
                }
                else if (streams & base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Color0_4U8))
                {
                    jobInfo.colorWritePtr = writeColors;
                    writeColors += jobInfo.vertexCount;
                }
            }
        }

        // copy out the attributes for each chunk
        auto jobCounter = Fibers::GetInstance().createCounter("ChunkExportStream", sourceJobsInfo.size());
        RunChildFiber("ChunkExportStream").invocations(sourceJobsInfo.size()) << [jobCounter, flipUV, flipFaces, top, &assetToEngine, &data, streams, positions, uvs, colors, normals, &sourceJobsInfo, &builder](FIBER_FUNC)
        {
            const auto& jobInfo = sourceJobsInfo[index];

            // extract positions
            {
                if (streams & base::mesh::MeshStreamMaskFromType(base::mesh::MeshStreamType::Position_3F))
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

            Fibers::GetInstance().signalCounter(jobCounter);
        };

        Fibers::GetInstance().waitForCounterAndRelease(jobCounter);

        // extract chunk data
        builder.extract(outChunk);
    }

    //--

    static void MapTexture(base::res::IResourceCookerInterface& cooker, StringView<char> mtlLibraryPath, base::VariantTable& outParams, StringView<char> coreName, const MaterialMap& info)
    {
        if (info.m_path)
        {
            base::StringBuf resolvedTexturePath;
            if (cooker.queryResolvedPath(info.m_path, mtlLibraryPath, true, resolvedTexturePath))
            {
                TRACE_INFO("Resolved texture '{}' as '{}' (for {})", info.m_path, resolvedTexturePath, coreName);
                outParams.setValue<base::StringBuf>(base::StringID(coreName), resolvedTexturePath);
            }

            // TODO: reset ?
        }
    }

    void MeshCooker::reportManifestClasses(base::Array<base::SpecificClassType<base::res::IResourceManifest>>& outManifestClasses) const
    {
        outManifestClasses.pushBackUnique(MeshManifest::GetStaticClass());
    }

    base::res::ResourceHandle MeshCooker::cook(base::res::IResourceCookerInterface& cooker) const
    {
        // load source data from OBJ format
        auto sourceFilePath = cooker.queryResourcePath().path(); // drop decorators
        auto sourceGeometry = cooker.loadDependencyResource<wavefront::FormatOBJ>(sourceFilePath);
        if (!sourceGeometry)
            return nullptr;

        // load manifest for mesh backing
        auto meshGeometryManifest = cooker.loadManifestFile<MeshManifest>();

        // calculate the transform to apply to source data
        // TODO: move to config file!
        auto defaultSpace = base::mesh::MeshImportSpace::LeftHandYUp;
        if (cooker.queryResourcePath().path().beginsWith("engine/"))
            defaultSpace = base::mesh::MeshImportSpace::RightHandZUp;

        // group into target objects
        base::Array<GroupBuildList> buildList;
        PrepareGroupBuildList(*sourceGeometry, meshGeometryManifest->groupingMethod, buildList);
        DetermineAttributeMasks(buildList, meshGeometryManifest->emitUVs, meshGeometryManifest->emitColors, meshGeometryManifest->emitNormals, meshGeometryManifest->forceTriangles);
        TRACE_INFO("Build list contains '{}' meshes to process", buildList.size());

        // calculate asset transformation to engine space
        auto assetToEngineTransform = meshGeometryManifest->calcAssetToEngineConversionMatrix(base::mesh::MeshImportUnits::Meters, defaultSpace);
        auto allowThreads = meshGeometryManifest->allowThreads && cvAllowWavefrontThreads.get();

        // should we swap faces ?
        auto swapFaces = meshGeometryManifest->flipFaces ^ (assetToEngineTransform.det() < 0.0f);

        // should we swap the UVs ?
        auto flipUVs = meshGeometryManifest->flipUV;

        // calculate all geometry centers
        CalcGeometryCenters(*sourceGeometry, assetToEngineTransform, buildList, allowThreads);

        // find max material index actually used inside chunks
        uint32_t maxMaterialIndex = 0;
        for (auto& sourceChunk : sourceGeometry->chunks())
            maxMaterialIndex = std::max<uint32_t>(maxMaterialIndex, sourceChunk.material);

        // load material library
        FormatMTLPtr materialLibrary;
        base::StringBuf fullMaterialLibraryPath;
        if (sourceGeometry->materialLibraryFileName())
        {
            if (cooker.queryResolvedPath(sourceGeometry->materialLibraryFileName(), cooker.queryResourcePath().path(), true, fullMaterialLibraryPath))
            {
                uint64_t size = 0;
                if (cooker.queryFileInfo(fullMaterialLibraryPath, nullptr, &size, nullptr) && size)
                {
                    if (materialLibrary = cooker.loadDependencyResource<FormatMTL>(base::res::ResourcePath(fullMaterialLibraryPath)))
                    {
                        TRACE_INFO("Using material library from '{}'", fullMaterialLibraryPath);
                    }
                    else
                    {
                        TRACE_WARNING("Material library at '{}' could not be lodaed", fullMaterialLibraryPath);
                    }
                }
                else
                {
                    TRACE_WARNING("Material library at '{}' does not exist and will not be lodaed", fullMaterialLibraryPath);
                }
            }
            else
            {
                TRACE_WARNING("Unable to find material library file '{}'", sourceGeometry->materialLibraryFileName());
            }
        }
        else
        {
            TRACE_WARNING("No material library specified for model, no materials will be defined");
        }

        // export materials
        base::Array<base::mesh::MeshMaterial> buildMaterials;
        {
            buildMaterials.reserve(buildList.size());

            for (auto& material : sourceGeometry->materialReferences())
            {
                auto& buildMaterial = buildMaterials.emplaceBack();
                buildMaterial.name = base::StringID(material.name.view());

                const auto sourceMaterial = materialLibrary ? materialLibrary->findMaterial(material.name) : nullptr;
                if (sourceMaterial)
                {
                    // setup material type
                    buildMaterial.flags -= base::mesh::MeshMaterialFlagBit::Simple;
                    if (sourceMaterial->m_illumMode < 2)
                        buildMaterial.flags |= base::mesh::MeshMaterialFlagBit::Unlit;
                    else if (sourceMaterial->m_illumMode >= 2)
                        buildMaterial.flags -= base::mesh::MeshMaterialFlagBit::Unlit;

                    // material "class", can be used (with config files) to redefine how imported parameters are mapped to material templates
                    buildMaterial.materialClass = base::StringID(base::TempString("WavefrontIllum{}", sourceMaterial->m_illumMode));

                    // setup properties
                    MapTexture(cooker, fullMaterialLibraryPath, buildMaterial.parameters, "BaseColorMap", sourceMaterial->m_mapDiffuse);
                    MapTexture(cooker, fullMaterialLibraryPath, buildMaterial.parameters, "ColorMap", sourceMaterial->m_mapDiffuse);
                    MapTexture(cooker, fullMaterialLibraryPath, buildMaterial.parameters, "DiffuseMap", sourceMaterial->m_mapDiffuse);
                    MapTexture(cooker, fullMaterialLibraryPath, buildMaterial.parameters, "BumpMap", sourceMaterial->m_mapBump);
                    MapTexture(cooker, fullMaterialLibraryPath, buildMaterial.parameters, "NormalMap", sourceMaterial->m_mapNormal);
                    MapTexture(cooker, fullMaterialLibraryPath, buildMaterial.parameters, "MaskMap", sourceMaterial->m_mapDissolve);
                    MapTexture(cooker, fullMaterialLibraryPath, buildMaterial.parameters, "EmissiveMap", sourceMaterial->m_mapEmissive);
                    MapTexture(cooker, fullMaterialLibraryPath, buildMaterial.parameters, "MetallicMap", sourceMaterial->m_mapMetallic);

                    if (!sourceMaterial->m_mapRoughnessSpecularity.m_path.empty())
                    {
                        MapTexture(cooker, fullMaterialLibraryPath, buildMaterial.parameters, "RoughnessSpecularMap", sourceMaterial->m_mapRoughnessSpecularity);
                    }
                    else
                    {
                        MapTexture(cooker, fullMaterialLibraryPath, buildMaterial.parameters, "SpecularMap", sourceMaterial->m_mapSpecular);
                        MapTexture(cooker, fullMaterialLibraryPath, buildMaterial.parameters, "RoughnessMap", sourceMaterial->m_mapRoughness);
                    }

                    buildMaterial.parameters.setValue<base::Color>("AmbientColor"_id, sourceMaterial->m_colorAmbient);
                    buildMaterial.parameters.setValue<base::Color>("DiffuseColor"_id, sourceMaterial->m_colorDiffuse);
                    buildMaterial.parameters.setValue<base::Color>("EmissiveColor"_id, sourceMaterial->m_colorEmissive);
                    buildMaterial.parameters.setValue<base::Color>("SpecularColor"_id, sourceMaterial->m_colorSpecular);
                    buildMaterial.parameters.setValue<base::Color>("TransmissionColor"_id, sourceMaterial->m_colorTransmission);
                    buildMaterial.parameters.setValue<uint32_t>("IllumMode"_id, sourceMaterial->m_illumMode);

                    buildMaterial.parameters.setValue<float>("DissolveCenter"_id, sourceMaterial->m_dissolveCenter);
                    buildMaterial.parameters.setValue<float>("DissolveHalo"_id, sourceMaterial->m_dissolveHalo);
                    buildMaterial.parameters.setValue<float>("SpecularPower"_id, sourceMaterial->m_specularExp);
                    buildMaterial.parameters.setValue<float>("Sharpness"_id, sourceMaterial->m_sharpness);
                    buildMaterial.parameters.setValue<float>("OpticalDensity"_id, sourceMaterial->m_opticalDensity);

                }
                else
                {
                    buildMaterial.materialClass = "WavefrontUndefined"_id;
                    buildMaterial.flags |= base::mesh::MeshMaterialFlagBit::Undefined;
                }
            }
        }

        // create as many models as we have in the build list
        base::Array<base::mesh::MeshModel> buildModels;
        buildModels.resize(buildList.size());

        // pack data
        {
            base::ScopeTimer packingTime;

            // create chunk building tasks
            auto workDoneSignal = Fibers::GetInstance().createCounter("MeshPacking", buildList.size());
            RunChildFiber("BuildGroupPacking").invocations(buildList.size()) << [swapFaces, flipUVs, workDoneSignal, maxMaterialIndex, &buildList, &sourceGeometry, &assetToEngineTransform, &buildModels](FIBER_FUNC)
            {
                const auto& entry = buildList[index];

                // prepare list for grouping materials
                base::Array<base::Array<const GroupChunk*>> materialChunks;
                materialChunks.resize(maxMaterialIndex + 1);

                // group chunks
                for (auto sourceChunk : entry.sourceChunks)
                    materialChunks[sourceChunk->material].pushBack(sourceChunk);

                // count used materials
                base::InplaceArray<uint16_t, 64> materialIndicesInThisChunk;
                for (uint32_t materialIndex = 0; materialIndex <= maxMaterialIndex; materialIndex++)
                    if (!materialChunks[materialIndex].empty())
                        materialIndicesInThisChunk.pushBack(materialIndex);

                // generate chunks in the model for each material
                auto& buildModel = buildModels[index];
                buildModel.name = base::StringID(entry.targetName);
                buildModel.chunks.resize(materialIndicesInThisChunk.size());
                buildModel.bounds.min = entry.boxMin;
                buildModel.bounds.max = entry.boxMax;

                // create the chunk building jobs
                auto localJobSignal = Fibers::GetInstance().createCounter("MeshChunkPacking", materialIndicesInThisChunk.size());
                RunChildFiber("MeshChunkPacking").invocations(materialIndicesInThisChunk.size()) << [swapFaces, flipUVs, localJobSignal, &materialIndicesInThisChunk, &buildModel, &materialChunks, &sourceGeometry, &assetToEngineTransform, &entry](FIBER_FUNC)
                {
                    const auto materialIndex = materialIndicesInThisChunk[index];
                    const auto& chunksPerMaterial = materialChunks[materialIndex];

                    auto& buildChunk = buildModel.chunks[index];
                    buildChunk.topology = entry.topology;
                    buildChunk.materialIndex = materialIndex;
                    buildChunk.streamMask = entry.streams;

                    ProcessSingleChunk(buildChunk.topology, *sourceGeometry, assetToEngineTransform, chunksPerMaterial, buildChunk.streamMask, swapFaces, flipUVs, buildChunk);
                    Fibers::GetInstance().signalCounter(localJobSignal);
                };

                Fibers::GetInstance().waitForCounterAndRelease(localJobSignal);
                Fibers::GetInstance().signalCounter(workDoneSignal);
            };

            // wait for all work done
            base::ScopeTimer waitTime;
            Fibers::GetInstance().waitForCounterAndRelease(workDoneSignal);
            TRACE_INFO("Packed {} chunks in {} ({} wait)", sourceGeometry->chunks().size(), packingTime, waitTime);
        }

        // one LOD
        base::Array<base::mesh::MeshDetailRange> buildRanges;
        {
            auto& lod = buildRanges.emplaceBack();
            lod.showDistance = 0.0f;
            lod.hideDistance = 100.0f; // TODO: compute based on real distance needed
        }

        // collision shapes
        // TODO: extract the collision shapes
        base::Array<base::mesh::MeshCollisionShape> shapes;

        // export
        base::Array<base::mesh::MeshBone> bones;
        return base::CreateSharedPtr<base::mesh::Mesh>(std::move(buildMaterials), std::move(buildModels), std::move(buildRanges), std::move(bones), std::move(shapes));
    }

#if 0
    rendering::content::PhysicsDataPtr MeshCooker::buildPhysicsData(const base::res::CookingParams& ctx, const rendering::content::GeometryData& geometryData) const
    {
        // get the rendering blob, we will use it to build physics
        auto renderingBlob = geometryData.renderingBlob();
        if (!renderingBlob)
            return nullptr;

        // no physics
        if (!m_physicsSettings)
            return nullptr;

        // get the file name part
        auto sourceFilePath = cooker.queryResourcePath().path();

        // get the lookup token
        auto physicsSettingsCRC = m_physicsSettings->calcSettingsCRC();
        auto lookupToken = base::StringBuf(base::TempString("PhysicsData_{}_{}", Hex(renderingBlob->dataCRC()), Hex(physicsSettingsCRC)));

        // load stuff from cache
        rendering::data::GeometryBlobShapeMesh renderingBlobMesh(*renderingBlob);
        auto physicsContainer = cooker.cacheData<rendering::content::PhysicsDataCachedContent>(sourceFilePath, lookupToken, [this, sourceFilePath, &renderingBlobMesh]() -> base::RefPtr<rendering::content::PhysicsDataCachedContent>
        {
            // create data
            auto physicsData = m_physicsSettings->buildFromMesh(renderingBlobMesh);
            if (!physicsData)
                return nullptr;

            auto physicsContainer = base::CreateSharedPtr<rendering::content::PhysicsDataCachedContent>();
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
        base::StringBuilder str;
        rendering::BuilderOBJ obj(str);

        auto numVerticalSegments = 20;
        auto numAngularSegments = 72; // divisible by 24 and 36
        auto radius = 1.0f;

        base::Array<rendering::BuilderOBJ::WriteVertex> previousVertices;
        for (int i=-1; i<=numVerticalSegments; ++i)
        {
            float y = std::max<int>(0,i) / (float)numVerticalSegments;
            float theta = HALFPI * y;

            base::Array<rendering::BuilderOBJ::WriteVertex> currentVertices;
            for (uint32_t j=0; j<=numAngularSegments; ++j)
            {
                float x = j / (float) numAngularSegments;
                float phi = TWOPI * x;

                auto& v = currentVertices.emplaceBack();
                v.uv = obj.writeUV(x, y);

                auto pos = i < 0 ?
                        base::Vector3(cosf(phi), sinf(phi), -1.0f) :
                        base::Vector3(cosf(phi) * cosf(theta), sinf(phi) * cosf(theta), sinf(theta));
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

        auto path = base::io::AbsolutePath::Build(L"/home/rexdex/skydome.obj");
        base::io::SaveFileFromString(path, str.toString());
    }*/


} // mesh


