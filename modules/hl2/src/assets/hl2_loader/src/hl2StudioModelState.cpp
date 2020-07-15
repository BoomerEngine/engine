/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"
#include "hl2StudioModelFile.h"
#include "hl2StudioModelState.h"

namespace hl2
{

    const base::Matrix ModelState::HL2Transform(0.0f, -2.2f / 64.0f, 0.0f, 0.0f,
                                          -2.2f / 64.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 2.2f / 64.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f, 1.0f);

    //---

    /*static void BuildMeshVertex(rendering::content::MeshBuilderVertex& outV, const mdl::mstudio_modelvertexdata_t& data, uint32_t index)
    {
        auto vertex  = data.GlobalVertex(index);

        outV.Position = ModelState::HL2Transform.transformPoint(vertex->m_vecPosition);
        outV.Normal = ModelState::HL2Transform.transformVector(vertex->m_vecNormal).normalized();
        outV.TexCoord[0] = vertex->m_vecTexCoord;
        outV.Color[0] = base::Color::WHITE;

        if (data.HasTangentData())
        {
            auto tangent  = data.GlobalTangentS(index);
            if (tangent->xyz().length() > 0.0f)
            {
                outV.Tangent = ModelState::HL2Transform.transformVector(tangent->xyz()).normalized();
                outV.Binormal = base::Cross(outV.Tangent, outV.Normal) * tangent->w;
            }
        }
    }*/

    base::Buffer ModelState::LoadModelData(base::res::IResourceCookerInterface& cooker, const base::StringBuf& pathOverride)
    {
        // we should cook a texture
        base::StringBuf sourceFilePath = pathOverride.empty() ? base::StringBuf(cooker.queryResourcePath().path()) : pathOverride;

        // load into buffer
        auto data = cooker.loadToBuffer(sourceFilePath);
        if (!data)
        {
            TRACE_ERROR("Unable to load data from source file '{}'", sourceFilePath);
            return nullptr;
        }

        // map the data
        auto modelData  = (const mdl::studiohdr_t*)data.data();
        if (!mdl::ValidateFile(*modelData))
        {
            TRACE_ERROR("Studio model files is not valid");
            return nullptr;
        }

        // print some stats
        TRACE_INFO("Model version {}", modelData->version);
        TRACE_INFO("Model name '{}'", modelData->pszName());
        TRACE_INFO("Num roots: {}", modelData->numAllowedRootLODs);
        TRACE_INFO("Num include models: {}", modelData->numincludemodels);
        TRACE_INFO("Num body parts: {}", modelData->numbodyparts);
        TRACE_INFO("Num bone controllers: {}", modelData->numbonecontrollers);
        TRACE_INFO("Num bones: {}", modelData->numbones);
        TRACE_INFO("Num flex controllers: {}", modelData->numflexcontrollers);
        TRACE_INFO("Num flex controllers UI: {}", modelData->numflexcontrollerui);
        TRACE_INFO("Num flex desc: {}", modelData->numflexdesc);
        TRACE_INFO("Num flex rules: {}", modelData->numflexrules);
        TRACE_INFO("Num hit boxes: {}", modelData->numhitboxsets);
        TRACE_INFO("Num anim blocks: {}", modelData->numanimblocks);
        TRACE_INFO("Num local anims: {}", modelData->numlocalanim);
        TRACE_INFO("Num local attachments: {}", modelData->numlocalattachments);
        TRACE_INFO("Num local nodes: {}", modelData->numlocalnodes);
        TRACE_INFO("Num IK chains: {}", modelData->numikchains);
        TRACE_INFO("Num IK auto play locks: {}", modelData->numlocalikautoplaylocks);
        TRACE_INFO("Num local pose parameters: {}", modelData->numlocalposeparameters);
        TRACE_INFO("Num local sequence: {}", modelData->numlocalseq);
        TRACE_INFO("Num mouths: {}", modelData->nummouths);
        TRACE_INFO("Num skin families: {}", modelData->numskinfamilies);
        TRACE_INFO("Num skin refs: {}", modelData->numskinref);
        TRACE_INFO("Num textures: {}", modelData->numtextures);
        TRACE_INFO("Num texture search paths: {}", modelData->numcdtextures);
        return data;
    }

    base::Buffer ModelState::LoadVertexData(base::res::IResourceCookerInterface& cooker, const base::StringBuf& modelName)
    {
        // get model path
        auto modelBasePath = cooker.queryResourcePath().path().beforeFirst("models/");
        base::StringBuf vertexFilePath = base::TempString("{}models/{}.vvd", modelBasePath, modelName.toLower().stringBeforeLast("."));
        vertexFilePath.replaceChar('\\', '/');
        TRACE_INFO("Trying to load vertex data from '{}'", vertexFilePath);

        // load data
        auto vertexFileData = cooker.loadToBuffer(vertexFilePath);
        if (!vertexFileData)
        {
            TRACE_ERROR("Unable to load data from source file '{}'", vertexFilePath);
            return nullptr;
        }

        // look at the header
        auto header  = (const mdl::vertexFileHeader_t*)vertexFileData.data();
        if (header->id != MODEL_VERTEX_FILE_ID)
        {
            TRACE_ERROR("Error Vertex File {} id {} should be {}\n", vertexFilePath, header->id, MODEL_VERTEX_FILE_ID);
            return nullptr;
        }
        if (header->version != MODEL_VERTEX_FILE_VERSION)
        {
            TRACE_ERROR("Error Vertex File {} version {} should be {}\n", vertexFilePath, header->version, MODEL_VERTEX_FILE_VERSION);
            return nullptr;
        }

        return vertexFileData;
    }

    base::Buffer ModelState::LoadOptimizedModelData(base::res::IResourceCookerInterface& cooker, const base::StringBuf& modelName)
    {
        // get model path
        auto modelBasePath = cooker.queryResourcePath().path().beforeFirst("models/");
        base::StringBuf vertexFilePath = base::TempString("{}models/{}.dx90.vtx", modelBasePath, modelName.toLower().stringBeforeLast("."));
        vertexFilePath.replaceChar('\\', '/');
        TRACE_INFO("Trying to load vertex data from '{}'", vertexFilePath);

        // load data
        auto vertexFileData = cooker.loadToBuffer(vertexFilePath);
        if (!vertexFileData)
        {
            // load from DX8 version
            vertexFilePath = base::TempString("{}models/{}.dx80.vtx", modelBasePath, modelName.toLower().stringBeforeLast("."));
            vertexFilePath.replaceChar('\\', '/');
            TRACE_INFO("Trying to load vertex data from '{}'", vertexFilePath);

            // load data
            vertexFileData = cooker.loadToBuffer(vertexFilePath);
            if (!vertexFileData)
            {
                // load from Software Rendering version
                vertexFilePath = base::TempString("{}models/{}.sw.vtx", modelBasePath, modelName.toLower().stringBeforeLast("."));
                vertexFilePath.replaceChar('\\', '/');
                TRACE_INFO("Trying to load vertex data from '{}'", vertexFilePath);

                // not loaded
                vertexFileData = cooker.loadToBuffer(vertexFilePath);
                if (!vertexFileData)
                {
                    TRACE_ERROR("Unable to load data from source file '{}'", vertexFilePath);
                    return nullptr;
                }
            }
        }

        // check header
        auto header  = (const mdl::OptimizedModel::FileHeader_t*)vertexFileData.data();
        if (header->version != OPTIMIZED_MODEL_FILE_VERSION)
        {
            TRACE_ERROR("Error Optimized Model File {} version {} should be {}\n", vertexFilePath, header->version, OPTIMIZED_MODEL_FILE_VERSION);
            return nullptr;
        }

        // print stats
        TRACE_INFO("Vertex cache size: {}", header->vertCacheSize);
        TRACE_INFO("Max bones per strip: {}", header->maxBonesPerStrip);
        TRACE_INFO("Max bones per tri: {}", header->maxBonesPerTri);
        TRACE_INFO("Max bones per vertex: {}", header->maxBonesPerVert);
        TRACE_INFO("Num LODs: {}", header->numLODs);
        TRACE_INFO("Num body parts: {}", header->numBodyParts);

        return vertexFileData;
    }

    base::RefPtr<ModelState> ModelState::Load(base::res::IResourceCookerInterface& cooker, const base::StringBuf& pathOverride/*= base::StringBuf::EMPTY()*/)
    {
        auto ret = base::CreateSharedPtr<ModelState>();

        // load the main model
        ret->m_modelDataBuffer = LoadModelData(cooker, pathOverride);
        if (cooker.checkCancelation() || !ret->m_modelDataBuffer)
            return nullptr;
        ret->m_modelData = (const mdl::studiohdr_t*)ret->m_modelDataBuffer.data();

        // load vertex data
        ret->m_vertexDataBuffer = LoadVertexData(cooker, ret->m_modelData->pszName());
        if (ret->m_vertexDataBuffer)
            ret->m_vertexData = (const mdl::vertexFileHeader_t*)ret->m_vertexDataBuffer.data();

        // load optimized model data
        ret->m_optimizedModelDataBuffer = LoadOptimizedModelData(cooker, ret->m_modelData->pszName());
        if (ret->m_optimizedModelDataBuffer)
            ret->m_optimizedModelData = (const mdl::OptimizedModel::FileHeader_t*)ret->m_optimizedModelDataBuffer.data();

        // load included models
        for (uint32_t i=0; i<ret->m_modelData->numincludemodels; ++i)
        {
            auto modelGroupInfo  = ret->m_modelData->pModelGroup(i);
            TRACE_INFO("Model group {}: {}, file {}", i, modelGroupInfo->pszLabel(), modelGroupInfo->pszName());
        }

        // fixup vertices
        if (ret->m_vertexData && ret->m_vertexData->numFixups > 0)
        {
            TRACE_INFO("Found {} vertex fixups", ret->m_vertexData->numFixups);

            ret->m_fixedVerticesForLOD.reserve(ret->m_vertexData->numLODs);
            for (uint32_t lodIndex=0; lodIndex<ret->m_vertexData->numLODs; ++lodIndex)
            {
                auto& vertexTable = ret->m_fixedVerticesForLOD.emplaceBack();
                for (uint32_t i=0; i<ret->m_vertexData->numFixups; ++i)
                {
                    auto& fixup = ret->m_vertexData->GetFixupData()[i];
                    if (fixup.lodIndex >= lodIndex)
                    {
                        for (uint32_t j = 0; j < fixup.vertexCount; ++j)
                            vertexTable.pushBack(fixup.vertexIndex + j);
                    }
                }
            }
        }

        return ret;
    }

    void ModelState::exportMaterialNames(base::res::IResourceCookerInterface& cooker, base::Array<rendering::MeshMaterial>& outMeshMaterials)
    {
        base::Task task(m_modelData->numcdtextures, "Processing materials");
        for (uint32_t i=0; i<m_modelData->numtextures; ++i)
        {
            auto texture = m_modelData->pTexture(i);

            // determine material name
            auto materialName = base::StringBuf(texture->pszName());
            if (materialName.empty())
                materialName = base::TempString("Material{}", i);

            // build a material path
            //base::res::ResourcePathBuilder pathBuilder(cooker.queryResourcePath());
            //pathBuilder.param("Material", materialName.c_str());

            // create a resource reference to base material
            /*rendering::MaterialInstanceRef baseMaterialRef;
            {
                if (auto material = base::LoadResource<rendering::content::MaterialInstance>(pathBuilder.buildPath()))
                    baseMaterialRef.reset(material);
            }

            // build a reference to material
            if (baseMaterialRef.empty())
                baseMaterialRef.resetToPathOnlyNoLoading(pathBuilder.buildPath(), rendering::content::MaterialInstance::GetStaticClass());

            // set base material only
            auto& outMaterial = outMeshMaterials.emplaceBack();
            outMaterial.name = base::StringID(materialName);
            outMaterial.m_material = baseMaterialRef;*/
        }
    }

    /*void ModelState::exportMaterials(rendering::content::MeshGeometryBuilder& geometryBuilder, rendering::content::MaterialBindingsBuilder& materialBuilder, base::res::IResourceCookerInterface& cooker, const base::Array<base::StringBuf>& discardedMaterials)
    {
        base::Array<base::StringBuf> searchPaths;
        for (uint32_t i=0; i<m_modelData->numcdtextures; ++i)
        {
            auto path  = m_modelData->pCdtexture(i);
            TRACE_INFO("MaterialSearchPath[{}]: '{}'", i, path);
            searchPaths.pushBack(path);

        }

        auto rootPathToDepot = cooker.queryResourcePath().path().stringBeforeFirst("/models/");

        m_materialMapping.clear();
        for (uint32_t i=0; i<m_modelData->numtextures; ++i)
        {
            auto texture = m_modelData->pTexture(i);
            TRACE_INFO("Material[{}]: {}, flags {}", i, texture->pszName(), Hex(texture->flags));

            // determine material name
            auto materialName = base::StringBuf(texture->pszName());
            if (materialName.empty())
                materialName = base::TempString("Material{}", i);

            // ignore material
            if (discardedMaterials.contains(materialName))
            {
                m_materialMapping.pushBack(INDEX_MAX);
                continue;
            }

            // create simple material
            auto localID = geometryBuilder.addMaterial(materialName);
            m_materialMapping.pushBack(localID);

            // assemble material paths and try to load
            for (auto& baseSearchPath : searchPaths)
            {
                auto fullPath = base::StringBuf(base::TempString("{}/materials/{}{}.vmt", rootPathToDepot, baseSearchPath, materialName)).toLower();
                fullPath.replaceChar('\\', '/');

                base::io::TimeStamp timestamp;
                if (cooker.queryFileTimestamp(fullPath, timestamp))
                {
                    TRACE_INFO("Found material at '{}'", fullPath);

                    // setup material ref
                    auto materialPath = base::res::ResourcePath(fullPath);
                    auto materialInstance = cooker.m_services.service<DepotService>()->loadResource<rendering::content::MaterialInstance>(materialPath);
                    materialBuilder.materialBase(base::StringID(materialName.c_str()), materialInstance);
                }
            }
        }
    }*/

    /*void ModelState::exportBodyPart(uint32_t bodyPartIndex, rendering::content::MeshGeometryBuilder& builder)
    {
        if (!m_vertexData || !m_optimizedModelData)
            return;

        auto bodyPart  = m_modelData->pBodypart(bodyPartIndex);
        auto optimizedBodyPart  = m_optimizedModelData->pBodyPart(bodyPartIndex);
        TRACE_INFO("Body part[{}]: {}, {} models", bodyPartIndex, bodyPart->pszName(), bodyPart->nummodels);

        // WTF
        if (optimizedBodyPart->numModels != bodyPart->nummodels)
        {
            TRACE_ERROR("Body part {} has different number of models {} than optimized models {}", bodyPart->pszName(), optimizedBodyPart->numModels, bodyPart->nummodels);
            return;
        }

        // in default mode export only model 0 of each body part
        uint32_t modelIndexToExport = 0;
        if (modelIndexToExport < bodyPart->nummodels)
        {
            // optimized mode must have the data as well
            auto model  = bodyPart->pModel(modelIndexToExport);
            auto optimizedModel  = optimizedBodyPart->pModel(modelIndexToExport);
            TRACE_INFO("  Model[{}]: {}, {} vertices, {} LODs, {} model meshes", modelIndexToExport, model->pszName(), model->numvertices, optimizedModel->numLODs, model->nummeshes);

            // bind vertex data
            model->vertexdata.pVertexData = m_vertexData->GetVertexData();
            model->vertexdata.pTangentData = m_vertexData->GetTangentData();

            // export each LOD
            auto maxLods = std::min<uint32_t>(1, optimizedModel->numLODs);
            for (uint32_t lodIndex=0; lodIndex<maxLods; ++lodIndex)
            {
                auto optimizedLOD  = optimizedModel->pLOD(lodIndex);
                TRACE_INFO("    LOD[{}]: {} meshes, distance {}", lodIndex, optimizedLOD->numMeshes, optimizedLOD->switchPoint);

                // emit the mesh for the given LOD
                for (uint32_t meshIndex=0; meshIndex<optimizedLOD->numMeshes; ++meshIndex)
                {
                    auto optimizedMesh  = optimizedLOD->pMesh(meshIndex);
                    auto mesh  = model->pMesh(meshIndex);
                    TRACE_INFO("      Mesh[{}]: {} strip groups, {} material, {} vertex offset", meshIndex, optimizedMesh->numStripGroups, mesh->material, mesh->vertexoffset);

                    // select material
                    if (optimizedMesh->numStripGroups > 0)
                    {
                        auto materialMappedID = m_materialMapping[mesh->material];
                        if (materialMappedID == INVALID_ID)
                        {
                            TRACE_INFO("        Material discarded");
                            continue;
                        }

                        builder.selectMaterial(materialMappedID);
                    }

                    // process strip
                    for (uint32_t stripGroupIndex=0; stripGroupIndex<optimizedMesh->numStripGroups; ++stripGroupIndex)
                    {
                        auto optimizedStripGroup  = optimizedMesh->pStripGroup(stripGroupIndex);
                        TRACE_INFO("        StripGroup[{}]: {} strips, {} vertices, {} indices, flags {}", stripGroupIndex, optimizedStripGroup->numStrips, optimizedStripGroup->numVerts, optimizedStripGroup->numIndices, optimizedStripGroup->flags);

                        if (optimizedStripGroup->numIndices == 0)
                            continue;

                        for (uint32_t stripIndex=0; stripIndex<optimizedStripGroup->numStrips; ++stripIndex)
                        {
                            auto optimizedStrip  = optimizedStripGroup->pStrip(stripIndex);
                            TRACE_INFO("          Strip[{}]: TriList, {} vertices (+{}), {} indices (+{}), bones {}, flags {}", stripIndex,
                                       optimizedStrip->numVerts, optimizedStrip->vertOffset,
                                       optimizedStrip->numIndices, optimizedStrip->indexOffset,
                                       optimizedStrip->numBones, optimizedStrip->flags);

                            if (optimizedStrip->flags & mdl::OptimizedModel::STRIP_IS_TRILIST)
                            {
                                for (uint32_t idx=0; idx<optimizedStrip->numIndices; idx += 3)
                                {
                                    auto index0 = *optimizedStripGroup->pIndex(idx + optimizedStrip->indexOffset + 0);
                                    auto index1 = *optimizedStripGroup->pIndex(idx + optimizedStrip->indexOffset + 1);
                                    auto index2 = *optimizedStripGroup->pIndex(idx + optimizedStrip->indexOffset + 2);

                                    auto vertex0 = optimizedStripGroup->pVertex(index0)->origMeshVertID;// + mesh->vertexoffset;// + model->vertexindex;
                                    auto vertex1 = optimizedStripGroup->pVertex(index1)->origMeshVertID;// + mesh->vertexoffset;// + model->vertexindex;
                                    auto vertex2 = optimizedStripGroup->pVertex(index2)->origMeshVertID;// + mesh->vertexoffset;// + model->vertexindex;

                                    // make global
                                    vertex0 = model->vertexdata.GetGlobalVertexIndex(vertex0 + mesh->vertexoffset);
                                    vertex1 = model->vertexdata.GetGlobalVertexIndex(vertex1 + mesh->vertexoffset);
                                    vertex2 = model->vertexdata.GetGlobalVertexIndex(vertex2 + mesh->vertexoffset);

                                    // apply fixup
                                    if (!m_fixedVerticesForLOD.empty())
                                    {
                                        auto& lodForVertices = m_fixedVerticesForLOD[lodIndex];
                                        vertex0 = lodForVertices[vertex0];
                                        vertex1 = lodForVertices[vertex1];
                                        vertex2 = lodForVertices[vertex2];
                                    }

                                    rendering::content::MeshBuilderVertex buildV[3];
                                    BuildMeshVertex(buildV[0], model->vertexdata, vertex0);
                                    BuildMeshVertex(buildV[1], model->vertexdata, vertex1);
                                    BuildMeshVertex(buildV[2], model->vertexdata, vertex2);
                                    builder.addTriangle(buildV[0], buildV[1], buildV[2]);
                                }
                            }
                            else if (optimizedStrip->flags & mdl::OptimizedModel::STRIP_IS_TRISTRIP)
                            {

                            }
                        }
                    }

                    //if (meshIndex)
                    //break;
                }
            }
        }
    }*/

    int rotSwap = 0;
    int rotPerm = 0;
    int posPerm = 0;
    int posSwap = 0;
    int rotOrder = 0;
    base::Vector3 rootRotation;

    static base::Vector3 Permute(base::Vector3 v, int perm)
    {
        switch (perm % 6)
        {
            case 4: return base::Vector3(v.x, v.z, v.y);
            case 1: return base::Vector3(v.y, v.x, v.z);
            case 3: return base::Vector3(v.y, v.z, v.x);
            case 2: return base::Vector3(v.z, v.y, v.x);
            case 5: return base::Vector3(v.z, v.x, v.y);
        }

        return v;
    }

    static base::Vector3 DecodeVector(base::Vector3 v, int perm, int swp)
    {
        v = Permute(v, perm);
        if (swp & 1) v.x = -v.x;
        if (swp & 2) v.y = -v.y;
        if (swp & 4) v.z = -v.z;
        return v;
    }

    /*void ModelState::exportBones(rendering::content::SkeletonDataBuilder& outSkeleton) const
    {
        // build model space transforms
        base::Array<base::Transform> modelSpaceTransforms;
        modelSpaceTransforms.resize(m_modelData->numbones);
        for (uint32_t i=0; i<m_modelData->numbones; ++i)
        {
            auto boneInfo = m_modelData->pBone(i);
            ASSERT(boneInfo->parent < (int)i);

            // convert bone transform
            auto boneRot = boneInfo->quat;
            auto bonePos = boneInfo->pos * (2.2f / 64.0f);
            if (boneInfo->parent == -1)
            {
                boneRot = base::Quat(base::Vector3::EZ(), 90.0f) * boneRot;
                auto tempPos = bonePos;
                bonePos.y = tempPos.x;
                bonePos.x = -tempPos.y;
            }

            // set transform
            modelSpaceTransforms[i].rotation(boneRot);
            modelSpaceTransforms[i].translation(bonePos);

            // move to model space
            if (boneInfo->parent != -1)
            {
                auto& parentTransfrom = modelSpaceTransforms[boneInfo->parent];
                modelSpaceTransforms[i] = modelSpaceTransforms[i].applyTo(parentTransfrom);
            }
        }

        // mirror
        for (auto& t : modelSpaceTransforms)
        {
            auto pos = t.translation();
            pos.y = -pos.y;
            t.translation(pos);

            auto rot = t.rotation();
            rot.x = -rot.x;
            rot.z = -rot.z;
            t.rotation(rot);
        }

        // export bones
        for (uint32_t i=0; i<m_modelData->numbones; ++i)
        {
            auto boneInfo  = m_modelData->pBone(i);

            // create bone info
            auto boneName = boneInfo->pszName();
            auto boneTransform = modelSpaceTransforms[i];
            outSkeleton.addBone(boneName, boneTransform);

            // create link
            if (boneInfo->parent != -1)
            {
                auto parentBoneInfo  = m_modelData->pBone(boneInfo->parent);
                outSkeleton.boneParent(boneName, parentBoneInfo->pszName());
            }
        }
    }*/

} // hl2
