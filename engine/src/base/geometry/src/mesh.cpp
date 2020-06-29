/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#include "build.h"
#include "mesh.h"
#include "shape.h"
#include "base/containers/include/pagedBuffer.h"
#include "base/io/include/utils.h"

namespace base
{
    namespace mesh
    {
        //---

         //--

        RTTI_BEGIN_TYPE_ENUM(MeshStreamType);
            RTTI_OLD_NAME("MeshStreamType");
            RTTI_ENUM_OPTION(Position_3F);
            RTTI_ENUM_OPTION(Normal_3F);
            RTTI_ENUM_OPTION(Tangent_3F);
            RTTI_ENUM_OPTION(Binormal_3F);
            RTTI_ENUM_OPTION(TexCoord0_2F);
            RTTI_ENUM_OPTION(TexCoord1_2F);
            RTTI_ENUM_OPTION(TexCoord2_2F);
            RTTI_ENUM_OPTION(TexCoord3_2F);
            RTTI_ENUM_OPTION(Color0_4U8);
            RTTI_ENUM_OPTION(Color1_4U8);
            RTTI_ENUM_OPTION(Color2_4U8);
            RTTI_ENUM_OPTION(Color3_4U8);
            RTTI_ENUM_OPTION(SkinningIndices_4U8);
            RTTI_ENUM_OPTION(SkinningWeights_4F);
            RTTI_ENUM_OPTION(SkinningIndicesEx_4U8);
            RTTI_ENUM_OPTION(SkinningWeightsEx_4F);
            RTTI_ENUM_OPTION(TreeLodPosition_3F);
            RTTI_ENUM_OPTION(TreeWindBranchData_4F);
            RTTI_ENUM_OPTION(TreeBranchData_7F);
            RTTI_ENUM_OPTION(TreeFrondData_4F);
            RTTI_ENUM_OPTION(TreeLeafAnchors_3F);
            RTTI_ENUM_OPTION(TreeLeafWindData_4F);
            RTTI_ENUM_OPTION(TreeLeafCardCorner_3F);
            RTTI_ENUM_OPTION(TreeLeafLod_1F);
            RTTI_ENUM_OPTION(General0_F4);
            RTTI_ENUM_OPTION(General1_F4);
            RTTI_ENUM_OPTION(General2_F4);
            RTTI_ENUM_OPTION(General3_F4);
            RTTI_ENUM_OPTION(General4_F4);
            RTTI_ENUM_OPTION(General5_F4);
            RTTI_ENUM_OPTION(General6_F4);
            RTTI_ENUM_OPTION(General7_F4);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_ENUM(MeshChunkRenderingMaskBit);
            RTTI_ENUM_OPTION(Scene);
            RTTI_ENUM_OPTION(ObjectShadows);
            RTTI_ENUM_OPTION(LocalShadows);
            RTTI_ENUM_OPTION(Cascade0);
            RTTI_ENUM_OPTION(Cascade1);
            RTTI_ENUM_OPTION(Cascade2);
            RTTI_ENUM_OPTION(Cascade3);
            RTTI_ENUM_OPTION(LodMerge);
            RTTI_ENUM_OPTION(ShadowMesh);
            RTTI_ENUM_OPTION(LocalReflection);
            RTTI_ENUM_OPTION(GlobalReflection);
            RTTI_ENUM_OPTION(Lighting);
            RTTI_ENUM_OPTION(StaticOccluder);
            RTTI_ENUM_OPTION(DynamicOccluder);
            RTTI_ENUM_OPTION(ConvexCollision);
            RTTI_ENUM_OPTION(ExactCollision);
            RTTI_ENUM_OPTION(Cloth);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_ENUM(MeshTopologyType);
            RTTI_ENUM_OPTION(Triangles);
            RTTI_ENUM_OPTION(Quads);
        RTTI_END_TYPE();

        //--

        uint32_t GetMeshStreamStride(MeshStreamType type)
        {
            switch (type)
            {
            case MeshStreamType::TexCoord0_2F:
            case MeshStreamType::TexCoord1_2F:
            case MeshStreamType::TexCoord2_2F:
            case MeshStreamType::TexCoord3_2F:
                return 8;

            case MeshStreamType::TreeLeafLod_1F:
            case MeshStreamType::Color0_4U8:
            case MeshStreamType::Color1_4U8:
            case MeshStreamType::Color2_4U8:
            case MeshStreamType::Color3_4U8:
            case MeshStreamType::SkinningIndices_4U8:
            case MeshStreamType::SkinningIndicesEx_4U8:
                return 4;

            case MeshStreamType::Position_3F:
            case MeshStreamType::Normal_3F:
            case MeshStreamType::Tangent_3F:
            case MeshStreamType::Binormal_3F:
            case MeshStreamType::TreeLodPosition_3F:
            case MeshStreamType::TreeLeafAnchors_3F:
            case MeshStreamType::TreeLeafCardCorner_3F:
                return 12;

            case MeshStreamType::SkinningWeights_4F:
            case MeshStreamType::SkinningWeightsEx_4F:
            case MeshStreamType::TreeWindBranchData_4F:
            case MeshStreamType::TreeFrondData_4F:
            case MeshStreamType::TreeLeafWindData_4F:
                return 16;

            case MeshStreamType::TreeBranchData_7F:
                return 28;
            }

            FATAL_ERROR("Invalid mesh stream type");
            return 0;
        }

        Buffer CreateMeshStreamBuffer(MeshStreamType type, uint32_t vertexCount)
        {
            Buffer ret;

            if (vertexCount)
            {
                const auto dataSize = vertexCount * GetMeshStreamStride(type);
                ret.initWithZeros(POOL_TEMP, dataSize, 16);
            }

            return ret;
        }

        //---

        RTTI_BEGIN_TYPE_STRUCT(MeshModelChunkData);
            RTTI_PROPERTY(type);
            RTTI_PROPERTY(data);
        RTTI_END_TYPE();

        MeshModelChunkData::MeshModelChunkData()
            : type(MeshStreamType::Position_3F)
        {}

        //---

        RTTI_BEGIN_TYPE_STRUCT(MeshModelChunk);
            RTTI_PROPERTY_FORCE_TYPE(streamMask, uint64_t);
            RTTI_PROPERTY_FORCE_TYPE(renderMask, uint32_t);
            RTTI_PROPERTY(detailMask);
            RTTI_PROPERTY(materialIndex);
            RTTI_PROPERTY(topology);
            RTTI_PROPERTY(numVertices);
            RTTI_PROPERTY(numFaces);
            RTTI_PROPERTY(streams);
            //RTTI_PROPERTY(faces);
        RTTI_END_TYPE();

        MeshModelChunk::MeshModelChunk()
            : streamMask(0)
            , renderMask(MeshChunkRenderingMaskBit::DEFAULT)
            , detailMask(1)
            , materialIndex(0)
            , topology(MeshTopologyType::Triangles)
            , numFaces(0)
            , numVertices(0)
        {}

        //---

        RTTI_BEGIN_TYPE_STRUCT(MeshModel);
            RTTI_PROPERTY(name);
            RTTI_PROPERTY(tags);
            RTTI_PROPERTY(chunks);
            RTTI_PROPERTY(bounds);
        RTTI_END_TYPE();

        MeshModel::MeshModel()
        {}

        //---

        RTTI_BEGIN_TYPE_STRUCT(MeshDetailRange);
            RTTI_PROPERTY(showDistance);
            RTTI_PROPERTY(hideDistance);
        RTTI_END_TYPE();

        MeshDetailRange::MeshDetailRange()
            : showDistance(0.0f)
            , hideDistance(0.0f)
        {}

        //---

        RTTI_BEGIN_TYPE_STRUCT(MeshMaterial);
            RTTI_PROPERTY(name);
            RTTI_PROPERTY(materialClass);
            RTTI_PROPERTY(materialBasePath);
            RTTI_PROPERTY(parameters);
            //RTTI_PROPERTY(data);
        RTTI_END_TYPE();

        MeshMaterial::MeshMaterial()
        {}

        //---

        RTTI_BEGIN_TYPE_STRUCT(MeshBone);
            RTTI_PROPERTY(name);
            RTTI_PROPERTY(parentIndex);
            RTTI_PROPERTY(posePlacement);
            RTTI_PROPERTY(poseRotation);
            RTTI_PROPERTY(poseToLocal);
            RTTI_PROPERTY(localToPose);
        RTTI_END_TYPE();

        MeshBone::MeshBone()
        {}

        //---

        RTTI_BEGIN_TYPE_STRUCT(MeshCollisionShape);
            RTTI_PROPERTY(boneIndex);
            RTTI_PROPERTY(shape);
        RTTI_END_TYPE();

        MeshCollisionShape::MeshCollisionShape()
        {}

        //---

        RTTI_BEGIN_TYPE_CLASS(Mesh);
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4geometry");
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Mesh geometry");
            RTTI_METADATA(base::res::ResourceDataVersionMetadata).version(7);
            RTTI_PROPERTY(m_materials);
            RTTI_PROPERTY(m_models);
            RTTI_PROPERTY(m_detailRanges);
            RTTI_PROPERTY(m_bones);
            RTTI_PROPERTY(m_collision);
            RTTI_PROPERTY(m_bounds);
        RTTI_END_TYPE();

        Mesh::Mesh()
        {
            recomputeBounds();
        }

        Mesh::Mesh(Array<MeshMaterial>&& materials, Array<MeshModel>&& models, Array<MeshDetailRange>&& details, Array<MeshBone>&& bones, Array<MeshCollisionShape>&& collision)
        {
            m_materials = std::move(materials);
            m_models = std::move(models);
            m_detailRanges = std::move(details);
            m_bones = std::move(bones);
            m_collision = std::move(collision);

            recomputeBounds();
            rebuildMaps();
        }

        const MeshMaterial* Mesh::findMaterial(base::StringID name) const
        {
            const MeshMaterial* ret = nullptr;
            m_materialsMap.find(name.index(), ret);
            return ret;
        }

        const MeshModel* Mesh::findModel(base::StringID name) const
        {
            const MeshModel* ret = nullptr;
            m_modelsMap.find(name.index(), ret);
            return ret;
        }

        void Mesh::onPostLoad()
        {
            TBaseClass::onPostLoad();
            rebuildMaps();
        }
        
        void Mesh::rebuildMaps()
        {
            m_materialsMap.clear();
            m_materialsMap.reserve(m_materials.size());

            m_modelsMap.clear();
            m_modelsMap.reserve(m_models.size());

            for (auto& ptr : m_materials)
                if (ptr.name)
                    m_materialsMap[ptr.name.index()] = &ptr;

            for (auto& ptr : m_models)
                if (ptr.name)
                    m_modelsMap[ptr.name.index()] = &ptr;
        }

        void Mesh::recomputeBounds()
        {
            m_bounds.clear();

            for (auto& model : m_models)
                m_bounds.merge(model.bounds);

            if (m_bounds.empty())
                m_bounds = base::Box(base::Vector3::ZERO(), 0.1f); // just something
        }

        //---

        struct PagedBufferPrinter : public base::IFormatStream
        {
            PagedBufferPrinter()
                : data(POOL_TEMP)
            {}

            virtual IFormatStream& append(const char* str, uint32_t len /*= INDEX_MAX*/) override
            {
                if (len == INDEX_MAX)
                    len = strlen(str);

                const auto* strEnd = str + len;
                while (str < strEnd)
                {
                    const auto toAlloc = strEnd - str;
                    uint32_t allocatedSize = 0;
                    auto* writePtr = data.allocateBatch(toAlloc, allocatedSize);
                    memcpy(writePtr, str, allocatedSize);
                    str += allocatedSize;
                }

                return *this; 
            }

            void save(const io::AbsolutePath& path) const
            {
                if (auto buffer = data.toBuffer())
                    base::io::SaveFileFromBuffer(path, buffer);
            }

            //--

            PagedBuffer<char> data;
        };

        template< typename T >
        struct VertexCacheEntry
        {
            T data;

            INLINE VertexCacheEntry()
            {}

            INLINE VertexCacheEntry(const T& x)
                : data(x)
            {}

            INLINE bool operator==(const T& x) const
            {
                return data == x;
            }

            INLINE bool operator==(const VertexCacheEntry<T>& x) const
            {
                return data == x.data;
            }

            INLINE static uint32_t CalcHash(const VertexCacheEntry& entry)
            {
                CRC32 crc;
                crc.append(&entry.data, sizeof(entry.data));
                return crc;
            }

            INLINE static uint32_t CalcHash(const T& data)
            {
                CRC32 crc;
                crc.append(&data, sizeof(data));
                return crc;
            }
        };

        template< typename T >
        struct VertexCache
        {
            HashMap<VertexCacheEntry<T>, uint32_t> entries;

            VertexCache()
            {
                entries.reserve(65536);
            }

            uint32_t map(const T& data)
            {
                if (const auto* index = entries.find(data))
                    return *index;

                auto index = entries.size() + 1;
                entries[data] = index;
                return index;
            }

            void reset()
            {
                entries.reset();
            }
        };

        static const MeshModelChunkData* FindMeshChunkDataForStream(const MeshModelChunk& chunk, MeshStreamType type)
        {
            for (const auto& data : chunk.streams)
                if (data.type == type)
                    return &data;
            return nullptr;
        }

        template< typename T >
        static void ExportStreamToCache(const MeshModelChunkData* data, VertexCache<T>& outCache, Array<uint32_t>& outIndexTable)
        {
            outIndexTable.reset();

            if (data)
            {
                const auto* readPtr = (T*)data->data.data();
                const auto* readEndPtr = readPtr + (data->data.size() / sizeof(T));

                while (readPtr < readEndPtr)
                    outIndexTable.emplaceBack(outCache.map(*readPtr++));
            }
        }

        static void PrintElement(IFormatStream& f, const VertexCacheEntry<base::Vector2>& data)
        {
            f.appendf(" {} {}\n", data.data.x, data.data.y);
        }

        static void PrintElement(IFormatStream& f, const VertexCacheEntry<base::Vector3>& data)
        {
            f.appendf(" {} {} {}\n", data.data.x, data.data.y, data.data.z);
        }

        template< typename T >
        static void PrintStreamData(IFormatStream& f, const char* prefix, const VertexCache<T>& cache, uint32_t& printCount)
        {
            const auto* printPtr = cache.entries.keys().typedData() + printCount;
            const auto* printEndPtr = cache.entries.keys().typedData() + cache.entries.keys().size();
            while (printPtr < printEndPtr)
            {
                f << prefix;
                PrintElement(f, *printPtr++);
            }
            printCount = cache.entries.size();
        }

        static const MeshStreamMask MASK_V_VT_VN = MeshStreamMaskFromType(MeshStreamType::Position_3F) | MeshStreamMaskFromType(MeshStreamType::TexCoord0_2F) | MeshStreamMaskFromType(MeshStreamType::Normal_3F);
        static const MeshStreamMask MASK_V_VT = MeshStreamMaskFromType(MeshStreamType::Position_3F) | MeshStreamMaskFromType(MeshStreamType::TexCoord0_2F);
        static const MeshStreamMask MASK_V_VN = MeshStreamMaskFromType(MeshStreamType::Position_3F) | MeshStreamMaskFromType(MeshStreamType::Normal_3F);
        static const MeshStreamMask MASK_V = MeshStreamMaskFromType(MeshStreamType::Position_3F);

        void Mesh::debugSave(const io::AbsolutePath& path) const
        {
            PagedBufferPrinter printer;

            printer.appendf("o BoomerModel\n");

            VertexCache<base::Vector3> cachePositions;
            VertexCache<base::Vector3> cacheNormals;
            VertexCache<base::Vector2> cacheUVs;

            Array<uint32_t> indicesPosition;
            Array<uint32_t> indicesNormal;
            Array<uint32_t> indicesUV;

            indicesPosition.reserve(65536);
            indicesNormal.reserve(65536);
            indicesUV.reserve(65536);

            uint32_t modelCount = 0;
            uint32_t prevPositionPrintCount = 0;
            uint32_t prevNormalPrintCount = 0;
            uint32_t prevUvPrintCount = 0;
            for (const auto& model : m_models)
            {   
                if (model.name.empty())
                    printer.appendf("g Model{}\n", modelCount++);
                else
                    printer.appendf("g {}\n", model.name);

                for (const auto& chunk : model.chunks)
                {
                    printer.appendf("usemtl {}\n", m_materials[chunk.materialIndex].name);

                    // map streams
                    ExportStreamToCache(FindMeshChunkDataForStream(chunk, MeshStreamType::Position_3F), cachePositions, indicesPosition);
                    ExportStreamToCache(FindMeshChunkDataForStream(chunk, MeshStreamType::Normal_3F), cacheNormals, indicesNormal);
                    ExportStreamToCache(FindMeshChunkDataForStream(chunk, MeshStreamType::TexCoord0_2F), cacheUVs, indicesUV);
                    
                    // print new data
                    PrintStreamData(printer, "v ", cachePositions, prevPositionPrintCount);
                    PrintStreamData(printer, "vn ", cacheNormals, prevNormalPrintCount);
                    PrintStreamData(printer, "vt ", cacheUVs, prevUvPrintCount);

                    // face table
                    const auto faceSize = (chunk.topology == MeshTopologyType::Triangles) ? 3 : 4;
                    TRACE_INFO("Chunk {}.{}: {} faces, {} vertices", model.name, m_materials[chunk.materialIndex].name, chunk.numFaces, chunk.numVertices);

                    // export faces
                    uint32_t indicesPtr = 0;
                    uint32_t indicesEndPtr = chunk.numFaces * faceSize;
                    const auto dataMask = chunk.streamMask & MASK_V_VT_VN;
                    if (MASK_V_VT_VN == dataMask)
                    {                        
                        while (indicesPtr < indicesEndPtr)
                        {
                            printer << "f";
                            for (uint32_t i = 0; i < faceSize; ++i)
                                printer.appendf(" {}/{}/{}", indicesPosition[indicesPtr + i], indicesUV[indicesPtr + i], indicesNormal[indicesPtr + i]);
                            indicesPtr += faceSize;
                            printer << "\n";
                        }
                    }
                    else if (MASK_V_VT == dataMask)
                    {
                        while (indicesPtr < indicesEndPtr)
                        {
                            printer << "f";
                            for (uint32_t i = 0; i < faceSize; ++i)
                                printer.appendf(" {}/{}", indicesPosition[indicesPtr + i], indicesUV[indicesPtr + i]);
                            indicesPtr += faceSize;
                            printer << "\n";
                        }
                    }
                    else if (MASK_V_VT_VN == dataMask)
                    {
                        while (indicesPtr < indicesEndPtr)
                        {
                            printer << "f";
                            for (uint32_t i = 0; i < faceSize; ++i)
                                printer.appendf(" {}//{}", indicesPosition[indicesPtr + i], indicesNormal[indicesPtr + i]);
                            indicesPtr += faceSize;
                            printer << "\n";
                        }
                    }
                    else if (MASK_V == dataMask)
                    {
                        while (indicesPtr < indicesEndPtr)
                        {
                            printer << "f";
                            for (uint32_t i = 0; i < faceSize; ++i)
                                printer.appendf(" {}", indicesPosition[indicesPtr + i]);
                            indicesPtr += faceSize;
                            printer << "\n";
                        }
                    }
                }
            }
            
            printer.save(path);
        }

        //---

    } // mesh
} // base
