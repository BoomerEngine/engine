/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: format #]
***/

#include "build.h"
#include "wavefrontFormatOBJ.h"

#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/stringParser.h"
#include "base/containers/include/hashSet.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/containers/include/pagedBuffer.h"

namespace wavefront
{
    namespace parser
    {

        base::ConfigProperty<bool> cvAllowUnrecognizedControlWords("Wavefront.Parser", "AllowUnrecognizedControlWords", true);
        base::ConfigProperty<int> cvMaxWavefrontParsingThreads("Wavefront.Parser", "MaxWavefrontParsingThreads", -2); // all possible but keep 2 on side

        struct DocumentRange
        {
            const char* firstLine;
            const char* endOfLastLine;

            uint32_t positionIndex;
            uint32_t normalIndex;
            uint32_t uvIndex;
            uint32_t colorIndex;
            uint32_t faceIndex;
            uint32_t faceIndexIndex;

        };

        struct BuildData
        {

        };

        static const char* EatName(const char* readPtr, const char* endPtr, base::StringView& outName)
        {
            auto start = readPtr;
            while (readPtr < endPtr && *readPtr != '\n')
                ++readPtr;
            outName = base::StringView(start, readPtr).trim();
            return readPtr;
        }

        struct WorkGroupState : public base::IReferencable
        {
            base::StringView objectName = "object";
            base::StringView groupName = "group";
            base::StringView materialName = "material";
            const char* startPtr = nullptr;
            const char* endPtr = nullptr;
            uint32_t lineCount = 0;
            uint32_t firstLine = 0;

            base::PagedBufferTyped<Position> parsedPositions;
            base::PagedBufferTyped<UV> parsedUVs;
            base::PagedBufferTyped<Normal> parsedNormals;
            base::PagedBufferTyped<Color> parsedColors;
            base::PagedBufferTyped<uint32_t> parsedFaceIndices;
            base::PagedBufferTyped<Face> parsedFaces;

            INLINE bool empty() const { return startPtr >= endPtr || 0 == lineCount; }
            INLINE void init(const char* ptr, uint32_t line) { startPtr = ptr; firstLine = line; }
            INLINE void extend(const char* ptr) { endPtr = ptr; lineCount += 1;  }
            INLINE void reset() { startPtr = endPtr = nullptr; lineCount = 0; }
        };

        struct WorkGroup : public WorkGroupState
        {
            uint32_t parsedPositionsOffset = 0;
            uint32_t parsedUVOffset = 0;
            uint32_t parsedNormalsOffset = 0;
            uint32_t parsedColorsOffset = 0;
            uint32_t parsedFaceIndicesOffset = 0;
            uint32_t parsedFacesOffset = 0;

            base::PagedBufferTyped<Position> parsedPositions;
            base::PagedBufferTyped<UV> parsedUVs;
            base::PagedBufferTyped<Normal> parsedNormals;
            base::PagedBufferTyped<Color> parsedColors;
            base::PagedBufferTyped<uint32_t> parsedFaceIndices;
            base::PagedBufferTyped<Face> parsedFaces;
        };

        struct WorkGroupQueue
        {
            uint32_t size() const
            {
                auto l = base::CreateLock(lock);
                return jobs.size();
            }

            auto begin() const
            {
                return jobs.begin();
            }

            auto end() const
            {
                return jobs.end();
            }

            base::RefPtr<WorkGroup> popJob()
            {
                auto l = base::CreateLock(lock);
                if (currentJob < jobs.size())
                    return jobs[currentJob++];
                return nullptr;
            }

            void append(const base::RefPtr<WorkGroup>& job)
            {
                if (job)
                {
                    auto l = base::CreateLock(lock);
                    jobs.pushBack(job);
                }
            }

        private:
            base::Array<base::RefPtr<WorkGroup>> jobs;
            uint32_t currentJob = 0;
            base::SpinLock lock;
        };

        void EmitWorkGroup(WorkGroupState* state, WorkGroupQueue& outJobs)
        {
            if (!state->empty())
            {
                auto group = base::RefNew<WorkGroup>();
                group->objectName = state->objectName;
                group->groupName = state->groupName;
                group->materialName = state->materialName;
                group->startPtr = state->startPtr;
                group->endPtr = state->endPtr;
                group->lineCount = state->lineCount;
                group->firstLine = state->firstLine;
                outJobs.append(group);
            }
        }

        static void SafeMatch(float& ret, StringView txt)
        {
            if (txt.match(ret) != MatchResult::OK)
            {
                ret = atof(TempString("{}", txt).c_str());
                //TRACE_WARNING("Unable to parse numerical value from '{}', falled back to CRT, got {}", txt, ret);
            }
        }

        void ParseWorkGroup(StringView contextName, WorkGroup& group)
        {
            const auto dataSize = group.endPtr - group.startPtr;

            auto* readPtr = group.startPtr;
            auto* endPtr = group.endPtr;
            uint32_t lineIndex = group.firstLine;
            while (readPtr < endPtr)
            {
                // skip white spaces
                if (*readPtr <= ' ')
                {
                    while (readPtr < endPtr && *readPtr <= ' ')
                    {
                        if (*readPtr == '\n') lineIndex++;
                        ++readPtr;
                    }
                }
                // skip comment
                else if (*readPtr == '#')
                {
                    while (readPtr < endPtr && *readPtr != '\n')
                        ++readPtr;
                    lineIndex += 1;
                }

                // find end of line
                auto lineStart = readPtr;
                while (readPtr < endPtr)
                {
                    if (*readPtr == '\n')
                        break;
                    readPtr += 1;
                    lineIndex += 1;
                }

                // get the line
                auto line = base::StringView(lineStart, readPtr);
                if (readPtr < endPtr)
                    readPtr += 1;

                // trim whitepsaces
                line = line.trim();

                // empty line ?
                if (line.length() < 2)
                    continue;

                // process the line
                if (line.data()[0] == 'v')
                {
                    base::InplaceArray<base::StringView, 10> parts;
                    line.slice(" ", false, parts);

                    if (parts.size() >= 1)
                    {
                        const auto ctrlChar = parts[0].data()[1];
                        if (ctrlChar == ' ' && parts.size() == 4)
                        {
                            auto* v = group.parsedPositions.allocSingle();
                            SafeMatch(v->x, parts[1]);
                            SafeMatch(v->y, parts[2]);
                            SafeMatch(v->z, parts[3]);
                        }
                        else if (ctrlChar == 't' && parts.size() >= 3)
                        {
                            auto* v = group.parsedUVs.allocSingle();
                            SafeMatch(v->x, parts[1]);
                            SafeMatch(v->y, parts[2]);
                        }
                        else if (ctrlChar == 'n' && parts.size() == 4)
                        {
                            auto* v = group.parsedNormals.allocSingle();
                            SafeMatch(v->x, parts[1]);
                            SafeMatch(v->y, parts[2]);
                            SafeMatch(v->z, parts[3]);
                        }
                        else if (ctrlChar == 'c' && parts.size() == 5)
                        {
                            auto* v = group.parsedColors.allocSingle();
                            parts[1].match(v->r);
                            parts[2].match(v->g);
                            parts[3].match(v->b);
                            parts[4].match(v->a);
                        }
                        else
                        {
                            TRACE_ERROR("{}({}): error: Invalid data type at line {}", contextName, lineIndex, line);
                        }
                    }
                    else
                    {
                        TRACE_ERROR("{}({}): error: Invalid element count at line {}", contextName, lineIndex, line);
                    }
                }
                else if (line.data()[0] == 'f')
                {
                    base::InplaceArray<base::StringView, 10> vertices;
                    line.subString(2).slice(" ", false, vertices);

                    if (vertices.size() >= 3)
                    {
                        uint8_t validAttributeMask = 0;
                        base::InplaceArray<uint32_t, 32> numericalIndices;
                        bool faceError = false;
                        for (const auto& vert : vertices)
                        {
                            if (vert == "f")
                                continue;

                            base::InplaceArray<base::StringView, 10> indices;
                            vert.slice("/", false, indices);

                            uint8_t indexIndex = 0;
                            for (const auto& index : indices)
                            { 
                                uint32_t val = 0;
                                if (base::MatchResult::OK == index.match(val))
                                {
                                    validAttributeMask |= 1 << indexIndex;
                                    val -= 1;
                                    numericalIndices.pushBack(val);
                                }
                                else if (!index.empty())
                                {
                                    TRACE_ERROR("{}({}): error: Error parsing verex index from '{}' at line {}", contextName, lineIndex, index, line);
                                    faceError = true;
                                    break;
                                }

                                indexIndex += 1;
                            }

                            if (faceError)
                                break;
                        }

                        if (!faceError)
                        {
                            const auto numAttributes = numericalIndices.size() / vertices.size();
                            if (numericalIndices.size() % numAttributes != 0)
                            {
                                TRACE_ERROR("{}({}): error: Inconsistent face attribute count at line {}", contextName, lineIndex, line);
                            }
                            else
                            {
                                group.parsedFaceIndices.writeLarge(numericalIndices.typedData(), numericalIndices.size());

                                auto* f = group.parsedFaces.allocSingle();
                                f->attributeMask = validAttributeMask;
                                f->numVertices = vertices.size();
                                f->smoothGroup = 1;
                            }
                        }
                    }
                    else
                    {
                        TRACE_ERROR("{}({}): error: Invalid face vertex count at line {}", contextName, lineIndex, line);
                    }
                }
                else if (line.data()[0] == 's')
                {

                }
                else
                {
                    TRACE_ERROR("{}({}): error: Invalid wavefront tag at line {}", contextName, lineIndex, line);
                }
            }

            TRACE_INFO("Processing group {}.{}.{} ({} bytes): {}v {}vt {}vn {}vc {}f {}fi",
                group.objectName, group.groupName, group.materialName, MemSize(dataSize),
                group.parsedPositions.size(), group.parsedUVs.size(), group.parsedNormals.size(), group.parsedColors.size(),
                group.parsedFaces.size(), group.parsedFaceIndices.size());
        }

        void ProcessParsingJob(StringView contextName, WorkGroupQueue& queue, const base::fibers::WaitCounter& counter)
        {
            if (auto work = queue.popJob())
            {
                RunChildFiber("WavefrontParsing") << [contextName, work, &queue, counter](FIBER_FUNC)
                {
                    ParseWorkGroup(contextName, *work);
                    ProcessParsingJob(contextName, queue, counter);
                };
            }
            else
            {
                TRACE_INFO("No more work groups to process");
                Fibers::GetInstance().signalCounter(counter);
            }
        }

        struct TempGroup : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_WAVEFRONT)

        public:
            base::StringBuf name;
            base::Array<GroupChunk> chunks;            
        };

        struct TempObject : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_WAVEFRONT)

        public:
            base::StringBuf name;
            base::HashMap<base::StringBuf, TempGroup*> groups;
            base::Array<TempGroup*> groupList;

            TempGroup* group(base::StringView name)
            {
                DEBUG_CHECK(!name.empty());

                TempGroup* ret = nullptr;
                if (!groups.find(name, ret))
                {
                    ret = new TempGroup;
                    ret->name = base::StringBuf(name);
                    groups[ret->name] = ret;
                    groupList.pushBack(ret);
                }

                return ret;
            }

            ~TempObject()
            {
                groups.clearPtr();
            }
        };

        struct TempModel
        {
            base::HashMap<base::StringBuf, TempObject*> objects;
            base::Array<TempObject*> objectList;
            base::HashMap<base::StringBuf, uint16_t> materials;
            base::Array<base::StringBuf> materialList;

            uint16_t material(base::StringView name)
            {
                uint16_t ret = 0;
                if (!materials.find(name, ret))
                {
                    ret = (uint16_t)materialList.size();
                    materialList.emplaceBack(name);
                    materials[materialList.back()] = ret;
                }
                return ret;
            }

            TempObject* object(base::StringView name)
            {
                DEBUG_CHECK(!name.empty());

                TempObject* ret = nullptr;
                if (!objects.find(name, ret))
                {
                    ret = new TempObject;
                    ret->name = base::StringBuf(name);
                    objects[ret->name] = ret;
                    objectList.pushBack(ret);
                }

                return ret;
            }

            ~TempModel()
            {
                objects.clearPtr();
            }
        };

        FormatOBJPtr LoadFromBuffer(base::StringView contextName, const void* data, uint64_t dataSize, bool allowThreads/* = true*/)
        {
            auto readPtr  = (const char*)data;
            auto endPtr  = readPtr + dataSize;

            base::StringView materialLibraryName;
            WorkGroupQueue exportedWorkStates;
            auto workGroup = base::RefNew<WorkGroupState>();
            base::ScopeTimer totalTime;

            // scan content for work groups
            {
                base::ScopeTimer timer;
                uint32_t lineCount = 1;
                while (readPtr < endPtr)
                {
                    // skip white spaces
                    if (*readPtr <= ' ')
                    {
                        while (readPtr < endPtr && *readPtr <= ' ')
                        {
                            if (*readPtr == '\n') lineCount++;
                            ++readPtr;
                        }
                    }
                    // skip comment
                    else if (*readPtr == '#')
                    {
                        while (readPtr < endPtr && *readPtr != '\n')
                            ++readPtr;
                        if (readPtr < endPtr)
                            ++readPtr;
                        lineCount += 1;
                    }

                    // identify block content
                    else if (*readPtr == 'v' || *readPtr == 'f' || *readPtr == 's')
                    {
                        if (workGroup->empty())
                            workGroup->init(readPtr, lineCount);

                        while (readPtr < endPtr && *readPtr != '\n')
                            ++readPtr;
                        if (readPtr < endPtr)
                            ++readPtr;
                        lineCount += 1;

                        workGroup->extend(readPtr);
                    }

                    // control words
                    else if (*readPtr == 'g')
                    {
                        base::StringView newName;
                        readPtr = EatName(readPtr + 2, endPtr, newName);

                        if (newName != workGroup->groupName)
                        {
                            EmitWorkGroup(workGroup, exportedWorkStates);
                            workGroup->reset();
                            workGroup->groupName = newName;
                        }

                        lineCount += 1;
                    }
                    else if (*readPtr == 'o')
                    {
                        base::StringView newName;
                        readPtr = EatName(readPtr + 2, endPtr, newName);

                        if (newName != workGroup->objectName)
                        { 
                            EmitWorkGroup(workGroup, exportedWorkStates);
                            workGroup->reset();
                            workGroup->objectName = newName;
                        }

                        lineCount += 1;
                    }
                    else if (*readPtr == 'u')
                    {
                        base::StringView newName;
                        readPtr = EatName(readPtr + 7, endPtr, newName);

                        if (newName != workGroup->materialName)
                        {
                            EmitWorkGroup(workGroup, exportedWorkStates);
                            workGroup->reset();
                            workGroup->materialName = newName;
                        }

                        lineCount += 1;
                    }
                    else if (*readPtr == 'm')
                    {
                        readPtr = EatName(readPtr + 7, endPtr, materialLibraryName);
                        lineCount += 1;
                    }
                    else
                    {
                        base::StringView word;
                        EatName(readPtr, endPtr, word);

                        while (readPtr < endPtr && *readPtr <= ' ')
                        {
                            if (*readPtr == '\n') lineCount++;
                            ++readPtr;
                        }

                        if (!cvAllowUnrecognizedControlWords.get())
                        {
                            TRACE_ERROR("{}({}): error: Unknown control world '{}'", contextName, lineCount, word);
                            return nullptr;
                        }
                    }
                }

                if (!workGroup->empty())
                {
                    EmitWorkGroup(workGroup, exportedWorkStates);
                    workGroup->reset();
                }

                TRACE_INFO("Scanned {} line(s) in {}, generated {} task groups", lineCount, timer, exportedWorkStates.size());
            }

            // process work groups
            if (allowThreads)
            {
                base::ScopeTimer timer;

                // determine number of threads to start on
                auto maxJobs = std::max<uint32_t>(1, std::min<uint32_t>(cvMaxWavefrontParsingThreads.get(), exportedWorkStates.size()));
                if (cvMaxWavefrontParsingThreads.get() <= 0)
                    maxJobs = std::max<int>(1, Fibers::GetInstance().workerThreadCount() + cvMaxWavefrontParsingThreads.get());

                // start processing jobs
                auto finishCounter = Fibers::GetInstance().createCounter("WavefrontParser", maxJobs);
                for (uint32_t i = 0; i < maxJobs; ++i)
                    ProcessParsingJob(contextName, exportedWorkStates, finishCounter);

                // wait for processing to finish
                Fibers::GetInstance().waitForCounterAndRelease(finishCounter);
                TRACE_INFO("Parsed {} work groups in {} ({} threads)", exportedWorkStates.size(), timer, maxJobs);
            }
            else
            {
                base::ScopeTimer timer;

                while (auto work = exportedWorkStates.popJob())
                    ParseWorkGroup(contextName, *work);

                TRACE_INFO("Parsed {} work groups in {}", exportedWorkStates.size(), timer);
            }

            // calculate final data size
            uint32_t totalPositionsDataSize = 0;
            uint32_t totalNormalsDataSize = 0;
            uint32_t totalUVDataSize = 0;
            uint32_t totalColorDataSize = 0;
            uint32_t totalFaceDataSize = 0;
            uint32_t totalFaceIndicesDataSize = 0;
            for (const auto& job : exportedWorkStates)
            {
                job->parsedPositionsOffset = totalPositionsDataSize;
                job->parsedUVOffset = totalUVDataSize;
                job->parsedNormalsOffset = totalNormalsDataSize;
                job->parsedColorsOffset = totalColorDataSize;
                job->parsedFaceIndicesOffset = totalFaceIndicesDataSize;
                job->parsedFacesOffset = totalFaceDataSize;

                totalPositionsDataSize += job->parsedPositions.dataSize();
                totalNormalsDataSize += job->parsedNormals.dataSize();
                totalUVDataSize += job->parsedUVs.dataSize();
                totalColorDataSize += job->parsedColors.dataSize();
                totalFaceDataSize += job->parsedFaces.dataSize();
                totalFaceIndicesDataSize += job->parsedFaceIndices.dataSize();
                /*TRACE_INFO("OBJ WorkGroup, pos {}@{}, uv {}@{}, normals {}@{}, color {}@{}, fi {}@{}, faces {}@{}",
                    job->parsedPositionsOffset, job->parsedPositions.dataSize(),
                    job->parsedUVOffset, job->parsedUVs.dataSize(),
                    job->parsedNormalsOffset, job->parsedNormals.dataSize(),
                    job->parsedColors, job->parsedColors.dataSize(),
                    job->totalFaceIndicesDataSize, job->parsedFaceIndices.d
                    ataSize(),
                    job->totalFaceDataSize, job->parsedFaces.dataSize());*/
            }            

            // allocate final data
            auto ret = base::RefNew<FormatOBJ>();
            ret->m_positions.init(POOL_WAVEFRONT, totalPositionsDataSize, 16);
            ret->m_numPositions = totalPositionsDataSize / sizeof(Position);
            ret->m_normals.init(POOL_WAVEFRONT, totalNormalsDataSize, 16);
            ret->m_numNormals = totalNormalsDataSize / sizeof(Normal);
            ret->m_uvs.init(POOL_WAVEFRONT, totalUVDataSize, 16);
            ret->m_numUVs = totalUVDataSize / sizeof(UV);
            ret->m_colors.init(POOL_WAVEFRONT, totalColorDataSize, 16);
            ret->m_numColors = totalColorDataSize / sizeof(Color);
            ret->m_faces.init(POOL_WAVEFRONT, totalFaceDataSize, 16);
            ret->m_numFaces = totalFaceDataSize / sizeof(Face);
            ret->m_faceIndices.init(POOL_WAVEFRONT, totalFaceIndicesDataSize, 16);
            ret->m_numFaceIndices = totalFaceIndicesDataSize / sizeof(uint32_t);

            // copy data
            for (const auto& job : exportedWorkStates)
            {
                job->parsedPositions.copy(ret->m_positions.data() + job->parsedPositionsOffset, job->parsedPositions.dataSize());
                job->parsedNormals.copy(ret->m_normals.data() + job->parsedNormalsOffset, job->parsedNormals.dataSize());
                job->parsedUVs.copy(ret->m_uvs.data() + job->parsedUVOffset, job->parsedUVs.dataSize());
                job->parsedColors.copy(ret->m_colors.data() + job->parsedColorsOffset, job->parsedColors.dataSize());
                job->parsedFaces.copy(ret->m_faces.data() + job->parsedFacesOffset, job->parsedFaces.dataSize());
                job->parsedFaceIndices.copy(ret->m_faceIndices.data() + job->parsedFaceIndicesOffset, job->parsedFaceIndices.dataSize());
            }

            ret->m_matLibFile = base::StringBuf(materialLibraryName);

            uint32_t totalDataSize = 0;
            totalDataSize += totalPositionsDataSize;
            totalDataSize += totalNormalsDataSize;
            totalDataSize += totalUVDataSize;
            totalDataSize += totalColorDataSize;
            totalDataSize += totalColorDataSize;

            // data stats
            TRACE_INFO("Loaded {} of OBJ data:", MemSize(totalDataSize));
            if (ret->m_numPositions) { TRACE_INFO("  Positions: {} ({})", ret->m_numPositions, MemSize(totalPositionsDataSize)); }
            if (ret->m_numNormals) { TRACE_INFO("  Normals: {} ({})", ret->m_numNormals, MemSize(totalNormalsDataSize)); }
            if (ret->m_numUVs) { TRACE_INFO("  UVs: {} ({})", ret->m_numUVs, MemSize(totalUVDataSize)); }
            if (ret->m_numColors) { TRACE_INFO("  Colors: {} ({})", ret->m_numColors, MemSize(totalColorDataSize)); }
            if (ret->m_numFaces) { TRACE_INFO("  Faces: {} ({})", ret->m_numFaces, MemSize(totalFaceDataSize)); }
            if (ret->m_numFaceIndices) { TRACE_INFO("  FaceIndices: {} ({})", ret->m_numFaceIndices, MemSize(totalFaceIndicesDataSize)); }

            // construct the objects
            TempModel model;
            uint32_t totalTriangles = 0;
            uint32_t totalChunks = 0;
            for (const auto& job : exportedWorkStates)
            {
                if (!job->parsedFaces.empty())
                {
                    auto* object = model.object(job->objectName);
                    auto* group = object->group(job->groupName);

                    GroupChunk chunk;
                    chunk.material = model.material(job->materialName);
                    chunk.numFaces = job->parsedFaces.size();
                    chunk.numFaceIndices = job->parsedFaceIndices.size();
                    chunk.firstFace = job->parsedFacesOffset / sizeof(Face);
                    chunk.firstFaceIndex = job->parsedFaceIndicesOffset / sizeof(uint32_t);
                    chunk.numAttributes = chunk.numFaceIndices / chunk.numFaces;

                    uint32_t totalVertexCount = 0;
                    const auto* facePtr = (const Face*)ret->m_faces.data() + chunk.firstFace;
                    const auto* faceEndPtr = facePtr + chunk.numFaces;
                    uint8_t andedFaceAttributeMask = 0xFF;
                    uint8_t oredFaceAttributeMask = 0;
                    while (facePtr < faceEndPtr)
                    {
                        andedFaceAttributeMask &= facePtr->attributeMask;
                        oredFaceAttributeMask |= facePtr->attributeMask;
                        totalVertexCount += facePtr->numVertices;
                        totalTriangles += (facePtr->numVertices - 2);
                        ++facePtr;
                    }

                    if (oredFaceAttributeMask != andedFaceAttributeMask)
                    {
                        TRACE_ERROR("Chunk {}.{}.{} has inconsident Type face attributes in each face {} != {}", job->objectName, job->groupName, job->materialName,
                            andedFaceAttributeMask, oredFaceAttributeMask);
                        continue;
                    }

                    if (totalVertexCount % chunk.numFaces == 0)
                        chunk.commonFaceVertexCount = totalVertexCount / chunk.numFaces;
                    else
                        chunk.commonFaceVertexCount = 0;

                    if (chunk.numFaceIndices % totalVertexCount != 0)
                    {
                        TRACE_ERROR("Chunk {}.{}.{} has inconsident number of face attributes in each face ({} face indices, {} face vertices)", job->objectName, job->groupName, job->materialName,
                            chunk.numFaceIndices, totalVertexCount);
                        continue;
                    }

                    chunk.numAttributes = chunk.numFaceIndices / totalVertexCount;
                    chunk.attributeMask = andedFaceAttributeMask;
                    group->chunks.pushBack(chunk);
                    totalChunks += 1;
                }
            }

            TRACE_INFO("Found {} materials", model.materialList.size());
            for (const auto& matInfo : model.materialList)
            {
                auto& mat = ret->m_materialRefs.emplaceBack();
                mat.name = matInfo;
                TRACE_INFO("  Material[{}]: '{}'", ret->m_materialRefs.size() - 1, matInfo);
            }

            TRACE_INFO("Found {} objects", model.objectList.size());
            for (const auto* srcObj : model.objectList)
            {
                TRACE_INFO("  Object[{}]: {} ({} groups)", ret->m_objects.size(), srcObj->name, srcObj->groupList.size());

                auto& obj = ret->m_objects.emplaceBack();

                obj.name = srcObj->name;
                obj.firstGroup = ret->m_groups.size();
                obj.numGroups = srcObj->groupList.size();

                for (const auto* srcGroup : srcObj->groupList)
                {
                    TRACE_INFO("    Group[{}]: {} ({} chunks)", ret->m_groups.size() - obj.firstGroup, srcGroup->name, srcGroup->chunks.size());

                    auto& group = ret->m_groups.emplaceBack();
                    group.name = srcGroup->name;
                    group.firstChunk = ret->m_chunks.size();
                    group.numChunks = srcGroup->chunks.size();

                    for (const auto& srcChunk : srcGroup->chunks)
                    {
                        TRACE_INFO("      Chunk[{}]: Material[{}]: '{}' ({} faces), {} v/f, {} attr",
                            ret->m_chunks.size() - group.firstChunk, srcChunk.material, model.materialList[srcChunk.material],
                            srcChunk.numFaces, srcChunk.commonFaceVertexCount, srcChunk.numAttributes);
                        ret->m_chunks.emplaceBack(srcChunk);
                    }
                }                
            }

            TRACE_INFO("Final triangle count: {} in {} chunks", totalTriangles, totalChunks);
            TRACE_INFO("Final parsing time: {}", totalTime);
            return ret;
        }

    } // parser

    FormatOBJPtr LoadObjectFile(base::StringView contextName, const void* data, uint64_t dataSize, bool allowThreads/* = true*/)
    {
        return parser::LoadFromBuffer(contextName, data, dataSize, allowThreads);
    }

    //--
        
} // wavefront

