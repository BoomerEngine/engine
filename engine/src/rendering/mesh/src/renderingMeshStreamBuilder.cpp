/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: streams #]
***/

#include "build.h"
#include "renderingMeshStreamBuilder.h"

namespace rendering
{
    ///--

    static base::mem::PoolID POOL_MESH_BUILDER("Engine.MeshBuilder");

    ///--

    MeshRawStreamBuilder::MeshRawStreamBuilder(MeshTopologyType topology /*= MeshTopologyType::Triangles*/)
        : m_topology(topology)
    {
        memset(m_verticesRaw, 0, sizeof(m_verticesRaw));
    }

    MeshRawStreamBuilder::MeshRawStreamBuilder(const MeshRawStreamBuilder& source)
        : m_topology(source.topology())
    {
        memset(m_verticesRaw, 0, sizeof(m_verticesRaw));

        reserveVertices(source.numVeritces, source.streams());
        //reserveIndices(source.numIndices);

        for (uint32_t i = 0; i < MAX_STREAMS; ++i)
        {
            if (m_verticesRaw[i] != nullptr)
            {
                auto dataSize = source.numVeritces * GetMeshStreamStride((MeshStreamType)i);
                memcpy(m_verticesRaw[i], source.m_verticesRaw[i], dataSize);
            }
        }

        /*if (m_indicesRaw != nullptr)
        {
            auto dataSize  = sizeof(uint32_t) * source.numIndices;
            memcpy(m_indicesRaw, source.m_indicesRaw, dataSize);
        }*/

        numVeritces = source.numVeritces;
        //numIndices = source.numIndices;
    }

    MeshRawStreamBuilder::~MeshRawStreamBuilder()
    {
        clear();
    }

    void MeshRawStreamBuilder::clear()
    {
        /*if (m_indicesRaw != nullptr)
        {
            if (m_ownedIndices)
                MemFree(m_indicesRaw);
            m_indicesRaw = nullptr;
            m_maxVeritces = 0;
        }*/

        for (uint32_t i = 0; i < MAX_STREAMS; ++i)
        {
            auto mask = MeshStreamMaskFromType((MeshStreamType)i);
            if (m_ownedStreams & mask)
            {
                MemFree(m_verticesRaw[i]);
            }
        }

        memset(m_verticesRaw, 0, sizeof(m_verticesRaw));
        m_maxVeritces = 0;

        //numIndices = 0;
        numVeritces = 0;
    }

    void MeshRawStreamBuilder::reserveVertices(uint32_t maxVertices, MeshStreamMask newStreamsToAllocate)
    {
        auto neededStreams = m_validStreams | newStreamsToAllocate;

        // allocate vertices
        if ((maxVertices > m_maxVeritces) || (m_ownedStreams != neededStreams))
        {
            for (uint32_t i = 0; i < MAX_STREAMS; ++i)
            {
                auto mask = MeshStreamMaskFromType((MeshStreamType)i);
                if (neededStreams & mask)
                {
                    // if we are allocating streams we don't own we have to create the whole buffer
                    auto stride = GetMeshStreamStride((MeshStreamType)i);
                    auto dataSize = maxVertices * stride;
                    if (m_verticesRaw[i] && 0 == (m_ownedStreams & mask))
                    {
                        auto newData  = MemAlloc(POOL_MESH_BUILDER, dataSize, 16);
                        memcpy(newData, m_verticesRaw[i], numVeritces * stride);
                        m_verticesRaw[i] = newData;
                    }
                    else
                    {
                        m_verticesRaw[i] = MemRealloc(POOL_MESH_BUILDER, m_verticesRaw[i], dataSize, 16);
                    }

                    // mask stream
                    m_validStreams |= mask;
                    m_ownedStreams |= mask;
                }
            }

            m_maxVeritces = maxVertices;
        }
    }

    /*void MeshRawStreamBuilder::reserveIndices(uint32_t maxIndices)
    {
        if (maxIndices > m_maxIndices || (maxIndices && !m_ownedIndices))
        {
            auto dataSize = sizeof(uint32_t) * maxIndices;
            if (m_ownedIndices)
            {
                m_indicesRaw = (uint32_t*)MemRealloc(POOL_MESH_BUILDER, m_indicesRaw, dataSize, 16);
                m_maxIndices = maxIndices;
            }
            else
            {
                auto newData = MemAlloc(POOL_MESH_BUILDER, dataSize, 16);
                memcpy(newData, m_indicesRaw, numIndices * sizeof(uint32_t));
                m_indicesRaw = (uint32_t*)newData;
                m_ownedIndices = true;
            }

            m_maxIndices = maxIndices;
        }
    }*/

    void MeshRawStreamBuilder::bind(const MeshRawChunk& chunk, MeshStreamMask meshStreamMask)
    {
        clear();

        m_validStreams = chunk.streamMask & meshStreamMask;
        m_ownedStreams = 0;

        for (auto& stream : chunk.streams)
        {
            auto mask = MeshStreamMaskFromType(stream.type);
            if (m_validStreams & mask)
                m_verticesRaw[(uint32_t)stream.type] = (void*)stream.data.data();
        }

        //m_indicesRaw = (uint32_t*)chunk.faces.data();
        //m_ownedIndices = false;

        //m_maxIndices = chunk.faces.size() / sizeof(uint32_t);
        m_maxVeritces = chunk.numVertices;

        numVeritces = m_maxVeritces;
        //numIndices = m_maxIndices;
    }

    /*void MeshRawStreamBuilder::makeIndexBufferResident()
    {
        if (!m_ownedIndices)
        {
            auto dataSize = sizeof(uint32_t) * m_maxIndices;
            auto newData  = MemAlloc(POOL_MESH_BUILDER, dataSize, 16);
            memcpy(newData, m_indicesRaw, numIndices * sizeof(uint32_t));
            m_indicesRaw = (uint32_t*)newData;
        }
    }*/

    void MeshRawStreamBuilder::makeVertexStreamResident(MeshStreamType type)
    {
        auto mask = MeshStreamMaskFromType(type);
        if (m_validStreams & mask)
        {
            if (0 == (m_ownedStreams & mask))
            {
                auto stride = GetMeshStreamStride(type);
                auto dataSize = m_maxVeritces * stride;

                auto newData  = MemAlloc(POOL_MESH_BUILDER, dataSize, 16);
                memcpy(newData, m_verticesRaw[(uint32_t)type], numVeritces * stride);
                m_verticesRaw[(uint32_t)type] = newData;

                m_ownedStreams |= mask;
            }
        }
    }

    void MeshRawStreamBuilder::migrate(MeshRawStreamBuilder&& source)
    {
        clear();

        memcpy(this, &source, sizeof(MeshRawStreamBuilder));
        memset(&source, 0, sizeof(MeshRawStreamBuilder));
    }

    template< typename T >
    static void ReindexStream(const uint32_t* indexPtr, const uint32_t* indexEndPtr, const void* vertexReadPtr, void* vertexWritePtr)
    {
        auto readDataPtr  = (const T*)vertexReadPtr;
        auto writeDataPtr  = (T*)vertexWritePtr;

        while (indexPtr < indexEndPtr)
            *writeDataPtr++ = readDataPtr[*indexPtr++];
    }

    static void ReindexStream(const void* indexRawPtr, uint32_t numIndices, const void* vertexReadPtr, void* vertexWritePtr, uint32_t stride)
    {
        auto indexPtr  = (const uint32_t*)indexRawPtr;
        auto indexEndPtr  = indexPtr + numIndices;

        switch (stride)
        {
            case 1: ReindexStream<uint8_t>(indexPtr, indexEndPtr, vertexReadPtr, vertexWritePtr); break;
            case 2: ReindexStream<uint16_t>(indexPtr, indexEndPtr, vertexReadPtr, vertexWritePtr); break;
            case 4: ReindexStream<uint32_t>(indexPtr, indexEndPtr, vertexReadPtr, vertexWritePtr); break;
            case 8: ReindexStream<base::Point>(indexPtr, indexEndPtr, vertexReadPtr, vertexWritePtr); break;
            case 12: ReindexStream<base::Vector3>(indexPtr, indexEndPtr, vertexReadPtr, vertexWritePtr); break;
            case 16: ReindexStream<base::Rect>(indexPtr, indexEndPtr, vertexReadPtr, vertexWritePtr); break;
            default: ASSERT(!"Unsupported stream data stride");
        }
    }

    /*void MeshRawStreamBuilder::generateAutomaticIndexBuffer(MeshTopologyType top, bool flipFaces)
    {
        reserveIndices(numVeritces);
        numIndices = numVeritces;

        if (top == MeshTopologyType::Triangles)
        {
            DEBUG_CHECK_EX(numVeritces % 3 == 0, "Something wrong with the index count");
            for (uint32_t i = 0; i < numVeritces; i += 3)
            {
                if (flipFaces)
                {
                    m_indicesRaw[i + 0] = i + 2;
                    m_indicesRaw[i + 1] = i + 1;
                    m_indicesRaw[i + 2] = i;
                }
                else
                {
                    m_indicesRaw[i + 0] = i;
                    m_indicesRaw[i + 1] = i + 1;
                    m_indicesRaw[i + 2] = i + 2;
                }
            }
        }
        else if (top == MeshTopologyType::Quads)
        {
            DEBUG_CHECK_EX(numVeritces % 4 == 0, "Something wrong with the index count");
            for (uint32_t i = 0; i < numVeritces; i += 4)
            {
                if (flipFaces)
                {
                    m_indicesRaw[i + 0] = i + 3;
                    m_indicesRaw[i + 1] = i + 2;
                    m_indicesRaw[i + 2] = i + 1;
                    m_indicesRaw[i + 3] = i;
                }
                else
                {
                    m_indicesRaw[i + 0] = i;
                    m_indicesRaw[i + 1] = i + 1;
                    m_indicesRaw[i + 2] = i + 2;
                    m_indicesRaw[i + 3] = i + 3;
                }
            }
        }
    }

    void MeshRawStreamBuilder::removeIndexBufferAndDuplicateVertices()
    {
        if (m_indicesRaw)
        {
            auto faceSize = (m_topology == MeshTopologyType::Triangles) ? 3 : 4;
            auto faceCount = numIndices / faceSize;
            ASSERT_EX((numIndices % faceSize) == 0, "Number of indices does not correspond to mesh topology");

            MeshRawStreamBuilder temp;
            temp.reserveVertices(faceCount * faceSize, m_validStreams);
            temp.numIndices = 0;
            temp.numVeritces = faceCount * faceSize;

            for (uint32_t i = 0; i < MAX_STREAMS; ++i)
            {
                auto stride = GetMeshStreamStride((MeshStreamType)i);
                auto mask = MeshStreamMaskFromType((MeshStreamType)i);
                if (m_validStreams & mask)
                    ReindexStream(m_indicesRaw, numIndices, m_verticesRaw[i], temp.m_verticesRaw[i], stride);
            }

            migrate(std::move(temp));
        }
    }*/

    void MeshRawStreamBuilder::expandQuadsToTriangles()
    {
        DEBUG_CHECK_EX(m_topology == MeshTopologyType::Quads, "Only quads can be expanded to triangles");

        if (m_topology == MeshTopologyType::Quads)
        {

        }
    }

    void MeshRawStreamBuilder::extract(MeshRawChunk& outChunk) const
    {
        outChunk.streamMask = m_validStreams;
        outChunk.topology = m_topology;
        outChunk.bounds.clear();

        uint32_t numStreams = 0;
        for (uint32_t i = 0; i < MAX_STREAMS; ++i)
        {
            auto mask = MeshStreamMaskFromType((MeshStreamType)i);
            if (m_validStreams & mask)
            {
                numStreams += 1;

                if (i == (int)MeshStreamType::Position_3F)
                {
                    const auto* readPos = (const base::Vector3*)m_verticesRaw[i];
                    const auto* readEndPos = readPos + numVeritces;
                    while (readPos < readEndPos)
                        outChunk.bounds.merge(*readPos++);
                }
            }
        }

        //outChunk.faces.reset();
        outChunk.streams.clear();
        outChunk.streams.reserve(numStreams);

        /*if (!m_indicesRaw)
        {
            auto faceSize = (m_topology == MeshTopologyType::Triangles) ? 3 : 4;
            auto faceCount = numVeritces / faceSize;
            ASSERT_EX((numVeritces % faceSize) == 0, "Number of indices does not correspond to mesh topology");

            outChunk.numVertices = numVeritces;
            outChunk.numFaces = faceCount;

            // export fake index buffer
            {
                auto dataSize = numIndices * sizeof(uint32_t);
                outChunk.faces = Buffer::Create(POOL_MESH_BUILDER, dataSize);

                auto writePtr = (uint32_t*)outChunk.faces.data();
                for (uint32_t i = 0; i < numVeritces; ++i)
                    *writePtr++ = i;
            }

            // TODO: merge

            for (uint32_t i = 0; i < MAX_STREAMS; ++i)
            {
                auto mask = MeshStreamMaskFromType((MeshStreamType)i);
                if (m_validStreams & mask)
                {
                    auto& outStream = outChunk.streams.emplaceBack();
                    outStream.type = (MeshStreamType)i;
                    outStream.data = Buffer::Create(POOL_MESH_BUILDER, GetMeshStreamStride(outStream.type) * numVeritces, 16, m_verticesRaw[i]);
                }
            }
        }
        else*/
        {
            auto faceSize = (m_topology == MeshTopologyType::Triangles) ? 3 : 4;
            auto faceCount = numVeritces / faceSize;
            DEBUG_CHECK_EX((numVeritces % faceSize) == 0, "Number of indices does not correspond to mesh topology");

            // TODO: additional merge ? we already have indexed data

            outChunk.numVertices = numVeritces;
            outChunk.numFaces = faceCount;

            //outChunk.faces = Buffer::Create(POOL_MESH_BUILDER, numIndices * sizeof(uint32_t), 4, m_indicesRaw);

            for (uint32_t i = 0; i < MAX_STREAMS; ++i)
            {
                auto mask = MeshStreamMaskFromType((MeshStreamType)i);
                if (m_validStreams & mask)
                {
                    auto& outStream = outChunk.streams.emplaceBack();
                    outStream.type = (MeshStreamType)i;
                    outStream.data = base::Buffer::Create(POOL_MESH_BUILDER, GetMeshStreamStride(outStream.type) * numVeritces, 16, m_verticesRaw[i]);
                }
            }
        }
    }

    ///--

#if 0

    static bool ComparePoint(const Vector3& a, const Vector3& b)
    {
        auto EPSILON = 0.0001f;
        return a.squareDistance(b) < (EPSILON * EPSILON);
    }

    static bool CompareVector(const Vector3& a, const Vector3& b)
    {
        auto EPSILON = 0.00001f;
        return a.squareDistance(b) < (EPSILON * EPSILON);
    }

    static bool CompareVector(const Vector2& a, const Vector2& b)
    {
        auto EPSILON = 0.00001f;
        return a.squareDistance(b) < (EPSILON * EPSILON);
    }

    template<typename T>
    static bool CompareAny(const T& a, const T& b)
    {
        return a == b;
    }


    bool MeshBuilderVertex::equal(MeshStreamMask streamMask, const MeshBuilderVertex& other) const
    {
        if (streamMask & MeshStreamMaskFromType(MeshStreamType::Position_3F))
            if (!ComparePoint(Position, other.Position))
                return false;

        if (streamMask & MeshStreamMaskFromType(MeshStreamType::Normal_3F))
            if (!CompareVector(Normal, other.Normal))
                return false;

        if (streamMask & MeshStreamMaskFromType(MeshStreamType::Tangent_3F))
            if (!CompareVector(Tangent, other.Tangent))
                return false;

        if (streamMask & MeshStreamMaskFromType(MeshStreamType::Binormal_3F))
            if (!CompareVector(Binormal, other.Binormal))
                return false;

        if (streamMask & MeshStreamMaskFromType(MeshStreamType::TexCoord0_2F))
            if (!CompareVector(TexCoord[0], other.TexCoord[0]))
                return false;

        if (streamMask & MeshStreamMaskFromType(MeshStreamType::TexCoord1_2F))
            if (!CompareVector(TexCoord[1], other.TexCoord[1]))
                return false;

        if (streamMask & MeshStreamMaskFromType(MeshStreamType::TexCoord2_2F))
            if (!CompareVector(TexCoord[2], other.TexCoord[2]))
                return false;

        if (streamMask & MeshStreamMaskFromType(MeshStreamType::TexCoord3_2F))
            if (!CompareVector(TexCoord[3], other.TexCoord[3]))
                return false;

        if (streamMask & MeshStreamMaskFromType(MeshStreamType::Color0_4U8))
            if (!CompareAny(Color[0], other.Color[0]))
                return false;

        if (streamMask & MeshStreamMaskFromType(MeshStreamType::Color1_4U8))
            if (!CompareAny(Color[1], other.Color[1]))
                return false;

        if (streamMask & MeshStreamMaskFromType(MeshStreamType::Color2_4U8))
            if (!CompareAny(Color[2], other.Color[2]))
                return false;

        if (streamMask & MeshStreamMaskFromType(MeshStreamType::Color3_4U8))
            if (!CompareAny(Color[3], other.Color[3]))
                return false;

        if ((streamMask & MeshStreamMaskFromType(MeshStreamType::SkinningIndices_4U8)) ||
            (streamMask & MeshStreamMaskFromType(MeshStreamType::SkinningWeights_4F)))
        {
            for (uint32_t i = 0; i < 4; ++i)
                if (!Skinning[i].equal(other.Skinning[i]))
                    return false;
        }

        if ((streamMask & MeshStreamMaskFromType(MeshStreamType::SkinningIndicesEx_4U8)) ||
            (streamMask & MeshStreamMaskFromType(MeshStreamType::SkinningWeightsEx_4F)))
        {
            for (uint32_t i = 4; i < 8; ++i)
                if (!Skinning[i].equal(other.Skinning[i]))
                    return false;
        }

        return true;
    }

    MeshGeometryBuilder::MeshGeometryBuilder()
            : m_currentSmoothGroup(1)
            , m_mem(POOL_MESH_BUILDER)
    {
        // reset to default state
        clear();
    }

    void MeshGeometryBuilder::clear()
    {
        m_mem.clear();
        m_allChunks.clear();
        m_allMaterials.clear();
        m_allDetailLevels.clear();
        m_connectivity.clear();
        m_currentChunk = nullptr;
        m_flipFaces = false;

        // reset transform
        m_stackTransform.clear();
        m_currentTransform.reset();

        // setup initial streams
        m_currentChunkSetup.m_streamMask = MeshStreamMaskFromType(MeshStreamType::Position_3F);
        m_currentChunkSetup.m_streamMask |= MeshStreamMaskFromType(MeshStreamType::TexCoord0_2F);
        m_currentChunkSetup.m_streamMask |= MeshStreamMaskFromType(MeshStreamType::Normal_3F);
        m_currentChunkSetup.m_streamMask |= MeshStreamMaskFromType(MeshStreamType::Tangent_3F);
        m_currentChunkSetup.m_streamMask |= MeshStreamMaskFromType(MeshStreamType::Binormal_3F);
        m_currentChunkSetup.m_streamMask |= MeshStreamMaskFromType(MeshStreamType::Color0_4U8);
    }

    LocalID MeshGeometryBuilder::addMaterial(const StringBuf &materialName)
    {
        ASSERT_EX(!materialName.empty(), "Material name cannot be empty");

        // look for material with given name
        for (uint32_t i = 0; i < m_allMaterials.size(); ++i)
        {
            auto mat = m_allMaterials[i];
            if (mat->name == materialName)
            {
                return i;
            }
        }

        // create new material
        auto mat = m_mem.create<Material>();
        mat->name = materialName;

        // add to list
        auto matIndex = m_allMaterials.size();
        m_allMaterials.pushBack(mat);

        // return material index
        return matIndex;
    }

    //--

    void MeshGeometryBuilder::selectMaterial(LocalID materialID)
    {
        ASSERT_EX(materialID < m_allMaterials.size(), "Invalid material ID");
        if (m_currentChunkSetup.m_material != materialID)
        {
            m_currentChunkSetup.m_material = materialID;
            m_currentChunk = nullptr;
        }
    }

    void MeshGeometryBuilder::toggleFaceFlip(bool flip)
    {
        m_flipFaces = flip;
    }


    void MeshGeometryBuilder::toggleStream(MeshStreamType stream, bool state)
    {
        // enable disable the stream
        auto newStreams = m_currentChunkSetup.m_streamMask;
        if (state)
            newStreams |= MeshStreamMaskFromType(stream);
        else
            newStreams -= MeshStreamMaskFromType(stream);

        // TODO: consider auto promoting the stream type to more consistent formats

        // reselect the chunk if the render streams changed
        if (m_currentChunkSetup.m_streamMask != newStreams)
        {
            m_currentChunkSetup.m_streamMask = newStreams;
            m_currentChunk = nullptr;
        }
    }

    void MeshGeometryBuilder::toggleUVGeneration(MeshStreamType stream, bool isEnabled /*= true*/, float worldScale /*= 1.0f*/)
    {
        UVGeneration *gen = nullptr;

        switch (stream)
        {
            case MeshStreamType::TexCoord0_2F:
                gen = &m_uvGeneration[0];
                break;
            case MeshStreamType::TexCoord1_2F:
                gen = &m_uvGeneration[1];
                break;
            case MeshStreamType::TexCoord2_2F:
                gen = &m_uvGeneration[2];
                break;
            case MeshStreamType::TexCoord3_2F:
                gen = &m_uvGeneration[3];
                break;
        }

        if (gen)
        {
            gen->enabled = isEnabled;
            gen->m_scale = worldScale;
        }
    }


    void MeshGeometryBuilder::toggleTangentSpaceComputation(bool enabled)
    {
        if (m_currentChunkSetup.m_generateTangents != enabled)
        {
            m_currentChunkSetup.m_generateTangents = enabled;
            m_currentChunk = nullptr;
        }
    }

    void MeshGeometryBuilder::toggleNormalsComputation(bool enabled)
    {
        if (m_currentChunkSetup.m_generateNormals != enabled)
        {
            m_currentChunkSetup.m_generateNormals = enabled;
            m_currentChunk = nullptr;
        }
    }

    void MeshGeometryBuilder::smoothGroup(uint32_t group)
    {
        m_currentSmoothGroup = group;
    }

    void MeshGeometryBuilder::prepareChunk()
    {
        // current chunk is valid
        if (m_currentChunk != nullptr)
            return;

        // calculate the setup hash
        auto setupHash = m_currentChunkSetup.calcKey();

        // try to find an existing chunk with such setup
        if (!m_chunkMap.find(setupHash, m_currentChunk))
        {
            // create new chunk
            m_currentChunk = m_mem.create<Chunk>();
            m_currentChunk->m_setup = m_currentChunkSetup;
            m_currentChunk->m_setupKey = setupHash;

            // keep around
            m_allChunks.pushBack(m_currentChunk);
            m_chunkMap.set(setupHash, m_currentChunk);
        }
    }

    void MeshGeometryBuilder::addTriangle(const MeshBuilderVertex &a, const MeshBuilderVertex &b, const MeshBuilderVertex &c)
    {
        // make sure there's chunk to render to
        if (m_currentChunk == nullptr)
            prepareChunk();

        // allocate face
        auto face = m_mem.create<Face>();
        if (m_currentTransform.m_flipFaces ^ m_flipFaces)
        {
            prepareVertex(a, face->vertices[2]);
            prepareVertex(b, face->vertices[1]);
            prepareVertex(c, face->vertices[0]);
        }
        else
        {
            prepareVertex(a, face->vertices[0]);
            prepareVertex(b, face->vertices[1]);
            prepareVertex(c, face->vertices[2]);
        }

        // prepare face wide stuff
        // NOTE: this may fail if the face is degenerated
        if (!prepareFace(*face))
            return;

        // map points in the mesh connectivity
        face->points[0] = m_connectivity.mapPoint(face->vertices[0].Position);
        face->points[1] = m_connectivity.mapPoint(face->vertices[1].Position);
        face->points[2] = m_connectivity.mapPoint(face->vertices[2].Position);
        face->m_faceId = m_connectivity.mapFace(face->points[0], face->points[1], face->points[2]);
        face->m_smoothGroup = m_currentSmoothGroup;

        // register in mapping
        if (face->m_faceId != -1)
        {
            ASSERT(face->m_faceId == (int) m_connectivityFaces.size());
            m_connectivityFaces.pushBack(face);
        }

        // link in the chunk face list
        m_currentChunk->m_faces.link(face);
    }

    void MeshGeometryBuilder::prepareVertex(const MeshBuilderVertex &src, MeshBuilderVertex &a)
    {
        // copy most of the data
        a = src;

        // transform the position
        a.Position = m_currentTransform.m_localToMesh.transformPoint(src.Position);

        // transform the existing normal
        if (m_currentChunk->hasStream(MeshStreamType::Normal_3F))
        {
            a.Normal = m_currentTransform.m_localToMeshNormals.transformVector(src.Normal);
            a.Normal.normalize();
        }

        // transform the existing tangent
        if (m_currentChunk->hasStream(MeshStreamType::Tangent_3F))
        {
            a.Tangent = m_currentTransform.m_localToMeshNormals.transformVector(src.Tangent);
            a.Tangent.normalize();
        }

        // transform the existing bitangent
        if (m_currentChunk->hasStream(MeshStreamType::Binormal_3F))
        {
            a.Binormal = m_currentTransform.m_localToMeshNormals.transformVector(src.Binormal);
            a.Binormal.normalize();
        }
    }

    void MeshGeometryBuilder::computeBoxUVProjects(const Vector3 &normal, float scale, const MeshBuilderVertex &a, const MeshBuilderVertex &b, const MeshBuilderVertex &c, Vector2 &outA, Vector2 &outB, Vector2 &outC)
    {
        // find best UV axis
        Vector3 axisU, axisV;
        if (normal.largestAxis() == 2)
        {
            axisU = Vector3::EX();
            axisV = Vector3::EY();
        }
        else if (normal.largestAxis() == 1)
        {
            axisU = Vector3::EX();
            axisV = Vector3::EZ();
        }
        else
        {
            axisU = Vector3::EY();
            axisV = Vector3::EZ();
        }

        // calculate the projected UV
        outA.x = Dot(axisU, a.Position) * scale;
        outA.y = Dot(axisV, a.Position) * scale;
        outB.x = Dot(axisU, b.Position) * scale;
        outB.y = Dot(axisV, b.Position) * scale;
        outC.x = Dot(axisU, c.Position) * scale;
        outC.y = Dot(axisV, c.Position) * scale;
    }

    bool MeshGeometryBuilder::prepareFace(Face &face)
    {
        // VERTICES
        auto va = &face.vertices[0];
        auto vb = &face.vertices[1];
        auto vc = &face.vertices[2];

        // EDGES
        auto ab = vb->Position - va->Position;
        auto ac = vc->Position - va->Position;

        // FACE NORMAL
        {
            auto n = Cross(ab, ac);

            // check if the face is not degenerated
            face.area = n.length();
            if (fabs(face.area) < 0.0000000001f)
                return false;

            // save face normal
            face.m_normal = n.normalized();
        }

        // UVs
        const MeshStreamMask UVStreams[] = {
                MeshStreamMaskFromType(MeshStreamType::TexCoord0_2F),
                MeshStreamMaskFromType(MeshStreamType::TexCoord1_2F),
                MeshStreamMaskFromType(MeshStreamType::TexCoord2_2F),
                MeshStreamMaskFromType(MeshStreamType::TexCoord3_2F)};
        for (uint32_t i = 0; i < ARRAY_COUNT(UVStreams); ++i)
        {
            if (m_uvGeneration[i].enabled && (m_currentChunk->m_setup.m_streamMask & UVStreams[i]))
            {
                computeBoxUVProjects(face.m_normal, m_uvGeneration[0].m_scale,
                                        face.vertices[0], face.vertices[1], face.vertices[2],
                                        face.vertices[0].TexCoord[i], face.vertices[1].TexCoord[i], face.vertices[2].TexCoord[i]);
            }
        }

        // TANGENTS
        if (m_currentChunk->m_setup.m_streamMask & MeshStreamMaskFromType(MeshStreamType::TexCoord0_2F))
        {
            // compute the UV vectors
            double s1 = vb->TexCoord[0].x - va->TexCoord[0].x;
            double t1 = vb->TexCoord[0].y - va->TexCoord[0].y;
            double s2 = vc->TexCoord[0].x - va->TexCoord[0].x;
            double t2 = vc->TexCoord[0].y - va->TexCoord[0].y;

            double det = (s1 * t2 - s2 * t1);
            if (det < -0.000001 || det > 0.000001)
            {
                face.m_tangent.x = (float) ((t2 * ab.x - t1 * ac.x) / det);
                face.m_tangent.y = (float) ((t2 * ab.y - t1 * ac.y) / det);
                face.m_tangent.z = (float) ((t2 * ab.z - t1 * ac.z) / det);
                face.m_tangent.normalize();

                face.m_bitangent.x = (float) ((s1 * ac.x - s2 * ab.x) / det);
                face.m_bitangent.y = (float) ((s1 * ac.y - s2 * ab.y) / det);
                face.m_bitangent.z = (float) ((s1 * ac.z - s2 * ab.z) / det);
                face.m_bitangent.normalize();
            }
        }

        // face is valid
        return true;
    }

    void MeshGeometryBuilder::pushTransform()
    {
        m_stackTransform.pushBack(m_currentTransform);
    }

    void MeshGeometryBuilder::popTransform()
    {
        if (!m_stackTransform.empty())
        {
            m_currentTransform = m_stackTransform.back();
            m_stackTransform.popBack();
        }
    }

    void MeshGeometryBuilder::transform(const Matrix &transform)
    {
        m_currentTransform.update(transform);
    }

    void MeshGeometryBuilder::transform(const Matrix &transform)
    {
        auto localToMesh = m_currentTransform.m_localToMesh;
        m_currentTransform.update(localToMesh * transform);
    }

    void MeshGeometryBuilder::translate(const Vector3 &offset)
    {
        auto localToMesh = m_currentTransform.m_localToMesh;
        localToMesh.translation(localToMesh.translation() + offset);
        m_currentTransform.update(localToMesh);
    }

    void MeshGeometryBuilder::translate(float x, float y, float z)
    {
        translate(Vector3(x, y, z));
    }

    void MeshGeometryBuilder::rotate(const Quat &rotation)
    {
        auto transformMatrix = rotation.toMatrix();
        transform(transformMatrix);
    }

    void MeshGeometryBuilder::rotate(const Angles &rotation)
    {
        rotate(rotation.toQuat());
    }

    void MeshGeometryBuilder::rotateWorldX(float degs)
    {
        rotate(Quat(Vector3::EX(), degs));
    }

    void MeshGeometryBuilder::rotateWorldY(float degs)
    {
        rotate(Quat(Vector3::EY(), degs));
    }

    void MeshGeometryBuilder::rotateWorldZ(float degs)
    {
        rotate(Quat(Vector3::EZ(), degs));
    }

    void MeshGeometryBuilder::scale(const Vector3 &scaling)
    {
        auto MIN_SCALE = 0.0001f;
        ASSERT_EX(fabsf(scaling.x) > MIN_SCALE, "X scale component is to small");
        ASSERT_EX(fabsf(scaling.y) > MIN_SCALE, "Y scale component is to small");
        ASSERT_EX(fabsf(scaling.z) > MIN_SCALE, "Z scale component is to small");

        auto localToMesh = m_currentTransform.m_localToMesh;
        localToMesh.scaleColumns(scaling).scaleTranslation(scaling);
        m_currentTransform.update(localToMesh);
    }

    void MeshGeometryBuilder::scale(float x, float y, float z)
    {
        scale(Vector3(x, y, z));
    }

    uint64_t MeshGeometryBuilder::ChunkSetup::calcKey() const
    {
        CRC64 crc;

        crc << m_streamMask;
        crc << m_material;
        crc << (m_generateNormals | m_generateTangents);
        crc << m_generateTangents;

        return crc.crc();
    }

    //--

    const RefPtr<GeometryChunkGroup> MeshGeometryBuilder::extractData() const
    {
        Task task(3, "");

        auto ret = RefNew<GeometryChunkGroup>();

        // compute normals on all elements that require normal computation
        {
            uint32_t numChunksToProcess = 0;
            for (uint32_t i = 0; i < m_allChunks.size(); ++i)
            {
                auto chunk = m_allChunks[i];
                if (chunk->m_setup.m_generateNormals)
                {
                    numChunksToProcess += 1;
                }
            }

            if (numChunksToProcess > 0)
            {
                Task task(numChunksToProcess, "Generating normals");

                for (uint32_t i = 0; i < m_allChunks.size(); ++i)
                {
                    auto chunk = m_allChunks[i];
                    if (chunk->m_setup.m_generateNormals)
                    {
                        Task task(1, TempString("Chunk {}", i));
                        generateNormals(*chunk, true, 45.0f);
                    }
                }
            }
        }

        // compute tangents on all elements that require normal computation
        {
            uint32_t numChunksToProcess = 0;
            for (uint32_t i = 0; i < m_allChunks.size(); ++i)
            {
                auto chunk = m_allChunks[i];
                if (chunk->m_setup.m_generateTangents)
                {
                    numChunksToProcess += 1;
                }
            }

            if (numChunksToProcess > 0)
            {
                Task task(numChunksToProcess, "Generating tangents");

                for (uint32_t i = 0; i < m_allChunks.size(); ++i)
                {
                    auto chunk = m_allChunks[i];
                    if (chunk->m_setup.m_generateTangents)
                    {
                        Task task(1, TempString("Chunk {}", i));
                        generateTangents(*chunk);
                    }
                }
            }
        }

        // build chunks
        {
            Task task(m_allChunks.size(), "");

            uint64_t totalMeshMemorySize = 0;
            TRACE_INFO("Generated {} chunks", m_allChunks.size());
            for (uint32_t i = 0; i < m_allChunks.size(); ++i)
            {
                Task task(1, TempString("Chunk {}", i));
                auto srcChunk = m_allChunks[i];

                // create target chunk
                auto &targetChunk = ret->m_chunks.emplaceBack(RefNew<GeometryChunk>());
                targetChunk->m_material = StringID(m_allMaterials[srcChunk->m_setup.m_material]->name.c_str());

                // pack vertex data
                auto streamBuilder = CreateUniquePtr<MeshBuilderVertexMerger>(srcChunk->m_setup.m_streamMask);
                for (auto face = srcChunk->m_faces.head; face != nullptr; face = face->next)
                    streamBuilder->addTriangle(face->vertices[0], face->vertices[1], face->vertices[2]);

                // extract streams
                streamBuilder->extractChunkStreams(targetChunk->m_streams);
                targetChunk->numVertices = streamBuilder->numVertices();
                targetChunk->numIndices = streamBuilder->numIndices();

                // nothing extracted, remove chunk
                if (targetChunk->numVertices == 0 || targetChunk->numIndices == 0)
                {
                    TRACE_INFO("Found empty chunk, it will not be exported");
                    ret->m_chunks.popBack();
                    continue;
                }

                // count memory size
                auto totalChunkMemorySize = targetChunk->dataSize();
                totalMeshMemorySize += totalChunkMemorySize;

                // stats
                TRACE_INFO("Generated {} streams ({}) of data for chunk {} ({} vertices, {} indices)",
                            targetChunk->m_streams.size(), MemSize(totalChunkMemorySize), i, targetChunk->numVertices, targetChunk->numIndices);
            }

            // total stats
            TRACE_INFO("Total mesh data size: {}", MemSize(totalMeshMemorySize));
        }

        return ret;
    }

    void MeshGeometryBuilder::generateNormals(Chunk& chunk, bool respectSmoothGroups, float maxSmoothAngle) const
    {
        // max angle for normal smoothing
        float maxNormalSmoothAngleCos = cos(DEG2RAD * (90.0f - maxSmoothAngle));

        // process faces
        for (auto face  = chunk.m_faces.head; face != nullptr; face = face->next)
        {
            if (face->m_faceId == -1)
                continue;

            // process vertices
            for (uint32_t i = 0; i < 3; ++i)
            {
                // vertex has at least the normal of it's face
                Vector3 vn = face->m_normal * face->area;

                // look at the faces shared by this vertex
                auto otherFacesAtPoint = m_connectivity.collectFacesAtPoint(face->points[i], face->m_faceId);
                for (;otherFacesAtPoint.valid(); ++otherFacesAtPoint)
                {
                    auto otherFaceId = otherFacesAtPoint.faceID();
                    if (otherFaceId != face->m_faceId)
                    {
                        auto otherFace = m_connectivityFaces[otherFaceId];
                        if (!respectSmoothGroups || (0 != (otherFace->m_smoothGroup & face->m_smoothGroup)))
                        {
                            if (Dot(otherFace->m_normal, face->m_normal) > maxNormalSmoothAngleCos)
                            {
                                vn += otherFace->m_normal * otherFace->area;
                            }
                        }
                    }
                }

                // normalize and overwrite the current vertex normal
                face->vertices[i].Normal = vn.normalized();
            }
        }
    }

    void MeshGeometryBuilder::generateTangents(Chunk& chunk) const
    {
        // process faces
        for (auto face  = chunk.m_faces.head; face != nullptr; face = face->next)
        {
            if (face->m_faceId == -1)
                continue;

            // process vertices
            for (uint32_t i = 0; i < 3; ++i)
            {
                // vertex has at least the tangents of the face
                Vector3 vn = face->vertices[i].Normal;
                Vector3 vt = face->m_tangent * face->area;
                Vector3 vb = face->m_bitangent * face->area;

                // look at the faces shared by this vertex
                auto thisPointId = face->points[i];
                auto otherFacesAtPoint = m_connectivity.collectFacesAtPoint(thisPointId, face->m_faceId);

                // get the UV coordinates at the current point
                // we can average the tangent space vectors only if the mapping is continuous
                auto pointUV = face->vertices[i].TexCoord[0];

                // average all tangent vectors at a given point if the texture coordinates are continuous
                // NOTE: if a mirroring is detected we do not average
                for (;otherFacesAtPoint.valid(); ++otherFacesAtPoint)
                {
                    auto otherFaceId = otherFacesAtPoint.faceID();
                    if (otherFaceId != face->m_faceId)
                    {
                        auto &otherFace = m_connectivityFaces[otherFaceId];

                        // detect mirror situations when face tangent space was flipped, in such case we cannot average the vectors
                        auto tDot = Dot(face->m_tangent, otherFace->m_tangent);
                        auto bDot = Dot(face->m_bitangent, otherFace->m_bitangent);
                        if (tDot < 0.0f || bDot < 0.0f)
                            continue;

                        for (uint32_t j = 0; j < 3; ++j)
                        {
                            // look for matching point
                            if (otherFace->points[j] == thisPointId)
                            {
                                // do we have the continuous UV ?
                                auto otherPointUV = otherFace->vertices[j].TexCoord[0];
                                if (otherPointUV.distance(pointUV) < 0.001f)
                                {
                                    vt += otherFace->m_tangent * otherFace->area;
                                    vb += otherFace->m_bitangent * otherFace->area;
                                    break;
                                }
                            }
                        }
                    }
                }

                // orthonormalize (Gram-Schmidt) the tangent vector
                vt = vt - vn*Dot(vt, vn);
                vt.normalize();

                // and the bitangent as well
                vb = vb - vn*Dot(vb, vn) - vt*Dot(vt, vb);
                vb.normalize();

                // overwrite the tangent space at vertex
                face->vertices[i].Tangent = vt;
                face->vertices[i].Binormal = vb;
            }
        }
    }
#endif

} // rendering
