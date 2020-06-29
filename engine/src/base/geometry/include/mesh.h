/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

#include "base/resources/include/resource.h"
#include "base/reflection/include/variantTable.h"

namespace base
{
    namespace mesh
    {
        //--

        /// binding for single material in mesh
        struct BASE_GEOMETRY_API MeshMaterial
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(MeshMaterial);

        public:
            MeshMaterial();

            StringID name; // display name of the material
            MeshMaterialFlags flags; // generalized material flags (if present in the source data)
            StringID materialClass; // type of the material, something like "WavefrontIlum0", "FbxPhongMaterial" etc that describes "shader" as understood by the importer
            StringBuf materialBasePath; // path to material instance/shader to use as a base (if known)
            VariantTable parameters; // imported parameters, NOTE: references to textures are saved as plain strings
        };

        //--

        // data stream in a mesh
        struct BASE_GEOMETRY_API MeshModelChunkData
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(MeshModelChunkData);

        public:
            MeshModelChunkData();

            MeshStreamType type; // stream type
            Buffer data; // data buffer
        };

        //--

        // chunk in a model
        struct BASE_GEOMETRY_API MeshModelChunk
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(MeshModelChunk);

        public:
            MeshModelChunk();

            MeshStreamMask streamMask; // streams in the chunk
            MeshChunkRenderingMask renderMask; // render mask for this chunk
            uint32_t detailMask; // detail mask (LOD levels this chunk is in)
            uint32_t materialIndex; // index of the material in the material table

            MeshTopologyType topology; // rendering topology
            uint32_t numVertices; // number of vertices in the chunk
            uint32_t numFaces; // number of faces in the chunk, this groups N vertices into a face 

            Array<MeshModelChunkData> streams; // stream of per-vertex data

            base::Box bounds; // position bounds in model space
        };

        //--

        /// model in a mesh
        struct BASE_GEOMETRY_API MeshModel
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(MeshModel);

        public:
            MeshModel();

            StringID name; // name of the model as given
            Array<StringID> tags; // additional tags
            Array<MeshModelChunk> chunks; // chunks with data for this model (usually one chunk per material but can be split more)
            Box bounds; // bounds of this model (NOTE: models are usually centered around 0)
        };

        //--

        /// detail range for model (LOD settings)
        struct BASE_GEOMETRY_API MeshDetailRange
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(MeshDetailRange);

        public:
            MeshDetailRange();

            float showDistance; // where to show given detail
            float hideDistance; // where to hide given bit
        };

        //--

        /// bone in the mesh
        struct BASE_GEOMETRY_API MeshBone
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(MeshBone);

        public:
            MeshBone();

            base::StringID name;
            short parentIndex = -1; // parent bone (note: may be NOT known)

            base::Vector3 posePlacement;
            base::Quat poseRotation;
            base::Matrix poseToLocal;
            base::Matrix localToPose;
        };

        //--

        /// collision shape
        struct BASE_GEOMETRY_API MeshCollisionShape
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(MeshCollisionShape);

        public:
            MeshCollisionShape();

            short boneIndex = -1; // only if shape is attached to particular bone (rare case but think of wall of bricks with bricks done using skinning)
            shape::ShapePtr shape; // collision shape, NOTE: only predefined collision shapes are exported (ie. triangle mesh copy of the whole mesh is NOT exported)
        };

        //--

        /// mesh is a grouping of chunks that should be rendered with the same relative position
        /// chunk is a geometry that should be rendered with a predefined material
        /// geometry is collection of data streams and shader to interpret them
        class BASE_GEOMETRY_API Mesh : public base::res::IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Mesh, base::res::IResource);

        public:
            Mesh();
            Mesh(Array<MeshMaterial>&& materials, Array<MeshModel>&& models, Array<MeshDetailRange>&& details, Array<MeshBone>&& bones, Array<MeshCollisionShape>&& collision);

            //---

            /// get all materials
            INLINE const Array<MeshMaterial>& materials() const { return m_materials; }

            /// get all models
            INLINE const Array<MeshModel>& models() const { return m_models; }

            /// get all detail levels
            INLINE const Array<MeshDetailRange>& detailRanges() const { return m_detailRanges; }

            /// get all mesh bones
            INLINE const Array<MeshBone>& bones() const { return m_bones; }
            
            /// get all collision shapes
            INLINE const Array<MeshCollisionShape>& collision() const { return m_collision; }

            /// get mesh bounds that cover all the mesh vertices
            INLINE const base::Box& bounds() const { return m_bounds; }

            ///---

            /// find material by name
            const MeshMaterial* findMaterial(base::StringID name) const;

            /// find model by name
            const MeshModel* findModel(base::StringID name) const;

            ///---

            /// debug save this mesh to OBJ file, helps a lot with debugging
            void debugSave(const io::AbsolutePath& path) const;

        private:
            //---

            Array<MeshMaterial> m_materials;
            Array<MeshModel> m_models;
            Array<MeshDetailRange> m_detailRanges;
            Array<MeshBone> m_bones;
            Array<MeshCollisionShape> m_collision; 
            Box m_bounds;

            //---

            HashMap<StringIDIndex, const MeshMaterial*> m_materialsMap;
            HashMap<StringIDIndex, const MeshModel*> m_modelsMap;

            //--

            virtual void onPostLoad() override;

            void rebuildMaps();
            void recomputeBounds();
        };

    } // mesh
} // base