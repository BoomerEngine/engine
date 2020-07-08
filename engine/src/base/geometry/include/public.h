/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "base_geometry_glue.inl"

#include "base/resource/include/resourceReference.h"

namespace base
{
    namespace mesh
    {
        //--

        /// type of data in the mesh stream
        enum class MeshStreamType : uint8_t
        {
            // float3 POSITION
            Position_3F = 0,

            // tangent space
            Normal_3F = 1,
            Tangent_3F = 2,
            Binormal_3F = 3,

            // UVs
            TexCoord0_2F = 4,
            TexCoord1_2F = 5,
            TexCoord2_2F = 6,
            TexCoord3_2F = 7,

            // colors
            Color0_4U8 = 8,
            Color1_4U8 = 9,
            Color2_4U8 = 10,
            Color3_4U8 = 11,

            // 4 bone skinning
            SkinningIndices_4U8 = 16,
            SkinningWeights_4F = 17,

            // 8 bone skinning (extra data)
            SkinningIndicesEx_4U8 = 18,
            SkinningWeightsEx_4F = 19,

            // trees/foliage
            TreeLodPosition_3F = 40,
            TreeWindBranchData_4F = 41,
            TreeBranchData_7F = 42,
            TreeFrondData_4F = 43,
            TreeLeafAnchors_3F = 44,
            TreeLeafWindData_4F = 45,
            TreeLeafCardCorner_3F = 46,
            TreeLeafLod_1F = 47,

            // extra/general purpose streams
            General0_F4 = 50,
            General1_F4 = 51,
            General2_F4 = 52,
            General3_F4 = 53,
            General4_F4 = 54,
            General5_F4 = 55,
            General6_F4 = 56,
            General7_F4 = 57,
        };

        /// mask of streams
        typedef uint64_t MeshStreamMask;

        /// get stream mask from stream type
        INLINE constexpr MeshStreamMask MeshStreamMaskFromType(MeshStreamType type)
        {
            return 1ULL << (uint64_t)type;
        }

        /// get stride for mesh data stream
        extern BASE_GEOMETRY_API uint32_t GetMeshStreamStride(MeshStreamType type);

        /// create empty stream buffer for given vertex stream
        extern BASE_GEOMETRY_API Buffer CreateMeshStreamBuffer(MeshStreamType type, uint32_t vertexCount);

        /// chunk rendering mask, defines when given chunk is rendered
        enum class MeshChunkRenderingMaskBit : uint32_t
        {
            Scene = FLAG(0), // chunk is used then rendering visible geometry
            ObjectShadows = FLAG(1), // chunk is used when rendering hi-res shadows for the object
            LocalShadows = FLAG(2), // chunk is used when rendering local shadows (points lights, spot lights, etc)
            Cascade0 = FLAG(3), // chunk is used for rendering shadows in cascade 0
            Cascade1 = FLAG(4), // chunk is used for rendering shadows in cascade 1
            Cascade2 = FLAG(5), // chunk is used for rendering shadows in cascade 2
            Cascade3 = FLAG(6), // chunk is used for rendering shadows in cascade 3
            LodMerge = FLAG(7), // use this chunk when generating merged lod
            ShadowMesh = FLAG(8), // this chunk is part of global shadow mesh
            LocalReflection = FLAG(9), // render this chunk in local reflection probes
            GlobalReflection = FLAG(10), // render this chunk in global reflection probes
            Lighting = FLAG(11), // include this chunk in lighting computations
            StaticOccluder = FLAG(12), // include this chunk in static occlusion calculations
            DynamicOccluder = FLAG(13), // include this chunk in dynamic occlusion calculations
            ConvexCollision = FLAG(14), // convex collision data (can be shared with rendering as well)
            ExactCollision = FLAG(15), // exact collision data
            Cloth = FLAG(16), // cloth mesh

            DEFAULT = Scene | ObjectShadows | LocalShadows | Cascade0 | Cascade1 | Cascade2 | Cascade3 | ShadowMesh | LodMerge | LocalReflection | GlobalReflection | Lighting | StaticOccluder,
            RENDERABLE = Scene | ObjectShadows | LocalShadows | Cascade0 | Cascade1 | Cascade2 | Cascade3 | ShadowMesh | LocalReflection | GlobalReflection | Lighting,
            ALL = 0xFFFF,
        };

        typedef base::DirectFlags<MeshChunkRenderingMaskBit> MeshChunkRenderingMask;

        //--

        // general material flag
        enum class MeshMaterialFlagBit : uint16_t
        {
            Unlit = FLAG(0), // material does not want to receive lighting
            Simple = FLAG(1), // material does not implement the full PBR pipeline
            Transparent = FLAG(2), // material is transparent
            Masked = FLAG(3), // material is masked
            Undefined = FLAG(4), // we don't have data for this material
        };

        typedef base::DirectFlags<MeshMaterialFlagBit> MeshMaterialFlags;

        //--

        /// mesh chunk topology type
        enum class MeshTopologyType : uint8_t
        {
            Triangles,
            Quads,
        };

        //--

        class Mesh;
        typedef RefPtr<Mesh> MeshPtr;
        typedef res::Ref<Mesh> MeshRef;

        struct MeshMaterial;
        struct MeshModelChunkData;
        struct MeshModelChunk;
        struct MeshModel;
        struct MeshDetailRange;

        //--

    } // mesh

    namespace shape
    {
        class AABB;
        class Capsule;
        class ConvexHull;
        class TriMesh;
        class Cylinder;
        class OBB;
        class Sphere;

        class IShape;
        typedef RefPtr<IShape> ShapePtr;

        ///---

        /// interface to access source mesh data without copying
        class BASE_GEOMETRY_API ISourceMeshInterface : public base::NoCopy
        {
        public:
            virtual ~ISourceMeshInterface();

            /// get number of triangles
            virtual uint32_t numTriangles() const = 0;

            /// process all triangles in the mesh
            typedef std::function<void(const Vector3 * vertices, uint32_t chunkIndex)> TProcessFunc;
            virtual void processTriangles(const TProcessFunc& func) const = 0;
        };

        ///---

    } // shape

} // base