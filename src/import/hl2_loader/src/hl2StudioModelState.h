/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"

#include "base/containers/include/stringBuilder.h"
#include "base/containers/include/stringParser.h"
#include "base/io/include/ioFileHandle.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "base/parser/include/textParser.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "rendering/mesh/include/renderingMesh.h"

namespace hl2
{
    class ModelState : public base::IReferencable
    {
    public:
        static const base::Matrix HL2Transform;

        static base::Buffer LoadModelData(base::res::IResourceCookerInterface& cooker, const base::StringBuf& pathOverride);

        static base::Buffer LoadVertexData(base::res::IResourceCookerInterface& cooker, const base::StringBuf& modelName);

        static base::Buffer LoadOptimizedModelData(base::res::IResourceCookerInterface& cooker, const base::StringBuf& modelName);

        static base::RefPtr<ModelState> Load(base::res::IResourceCookerInterface& cooker, const base::StringBuf& pathOverride = base::StringBuf::EMPTY());

        void exportMaterialNames(base::res::IResourceCookerInterface& cooker, base::Array<rendering::MeshMaterial>& outMeshMaterials);

        //void exportMaterials(rendering::content::MeshGeometryBuilder& geometryBuilder, rendering::content::MaterialBindingsBuilder& materialBuilder, base::res::IResourceCookerInterface& cooker, const base::Array<base::StringBuf>& discardedMaterials);

        //void exportBodyPart(uint32_t bodyPartIndex, rendering::content::MeshGeometryBuilder& builder);

        //--

        base::Buffer m_modelDataBuffer;
        base::Buffer m_vertexDataBuffer;
        base::Buffer m_optimizedModelDataBuffer;

        const mdl::studiohdr_t* m_modelData = nullptr;
        const mdl::vertexFileHeader_t* m_vertexData = nullptr;
        const mdl::OptimizedModel::FileHeader_t* m_optimizedModelData = nullptr;

        base::Array<uint32_t> m_materialMapping;
        base::Array<base::Array<uint32_t>> m_fixedVerticesForLOD;

        base::HashMap<base::StringBuf, base::RefPtr<ModelState>> m_includedModels;
    };

} // hl2