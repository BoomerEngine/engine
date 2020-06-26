/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"

#include "base/io/include/ioFileHandle.h"
#include "base/io/include/timestamp.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resourceCookingInterface.h"
#include "base/geometry/include/mesh.h"

#include "hl2StudioModelFile.h"
#include "hl2StudioModelState.h"

namespace hl2
{

    //---

    /// manifest for cooking MDL texture
    class MeshCookertMDL : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshCookertMDL, base::res::IResourceCooker);

    public:
        virtual base::res::ResourcePtr cook(base::res::IResourceCookerInterface& cooker) const override final
        {
            /*// load the model
            auto modelState = ModelState::Load(cooker);
            if (!modelState)
                return false;

            // export geometry
            rendering::content::MeshGeometryBuilder geometryBuilder;
            for (uint32_t i=0; i<modelState->m_modelData->numbodyparts; ++i)
                modelState->exportBodyPart(i, geometryBuilder);

            // extract geometry into chunk list
            // TODO: make a builder for each LOD
            base::Array<base::RefPtr<rendering::content::GeometryChunkGroup>> chunkGroups;
            if (auto chunk = geometryBuilder.extractData())
                chunkGroups.pushBack(chunk);

            base::Array<rendering::content::MeshMaterial> materials;
            modelState->exportMaterialNames(cooker, materials);

            // create new mesh object and initialize it with data
            auto createdMesh = base::rtti_cast<rendering::content::Mesh>(createdResource);
            //createdMesh->content(chunkGroups, materials);
            return true;*/
            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MeshCookertMDL);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<base::mesh::Mesh>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("mdl");
        RTTI_METADATA(base::res::ResourceCookerVersionMetadata).version(1);
    RTTI_END_TYPE();

} // hl2