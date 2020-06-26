/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: loader #]
***/

#include "build.h"
#include "fbxFileDAta.h"

#include "base/io/include/utils.h"
#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/containers/include/inplaceArray.h"
#include "base/resources/include/resource.h"
#include "base/resources/include/resourceCookingInterface.h"

namespace fbx
{
    //---

    MaterialMapper::MaterialMapper()
    {
        m_materials.reserve(256);
        m_materialsMapping.reserve(256);
    }

    uint32_t MaterialMapper::addMaterial(const fbxsdk::FbxSurfaceMaterial* material)
    {
        if (!material)
            return 0;

        uint32_t ret = 0;
        if (m_materialsMapping.find(material, ret))
            return ret;

        ret = m_materials.size();
        m_materials.pushBack(material);
        m_materialsMapping[material] = ret;
        return ret;
    }

    //---

    SkeletonBuilder::SkeletonBuilder()
    {
        m_skeletonBones.reserve(256);
        m_skeletonBonesMapping.reserve(256);
        m_skeletonRawBonesMapping.reserve(256);
    }

    uint32_t SkeletonBuilder::addBone(const LoadedFile& owner, const DataNode* node)
    {
        // invalid node, map to root
        if (!node)
            return 0;

        // find mapping
        uint32_t index = 0;
        if (m_skeletonBonesMapping.find(node, index))
            return index;

        // map parent
        SkeletonBone info;
        if (node->m_parent != nullptr)
            info.m_parentIndex = addBone(owner, node->m_parent);
        else
            info.m_parentIndex = -1;
        info.m_data = node;

        // store
        index = m_skeletonBones.size();
        m_skeletonBones.pushBack(info);

        // map
        m_skeletonBonesMapping[node] = index;
        m_skeletonRawBonesMapping[node->m_node] = index;
        return index;
    }

    uint32_t SkeletonBuilder::addBone(const LoadedFile& owner, const fbxsdk::FbxNode* node)
    {
        // invalid node, map to root
        if (!node)
            return 0;

        // find mapping
        uint32_t index = 0;
        if (m_skeletonRawBonesMapping.find(node, index))
            return index;

        // find the actual node
        auto actualNode  = owner.findDataNode(node);
        return addBone(owner, actualNode);
    }

    //---

    struct MeshVertexInfluence
    {
        uint32_t m_indices[4];
        float m_weights[4];
        uint8_t m_numBones;

        void add(uint32_t index, float weight)
        {
            if (m_numBones < ARRAY_COUNT(m_indices))
            {
                m_indices[m_numBones] = index;
                m_weights[m_numBones] = weight;
                m_numBones += 1;
            }
            else
            {
                uint32_t smallest = 0;
                for (uint32_t i=1; i<ARRAY_COUNT(m_indices); ++i)
                {
                    if (m_weights[i] < m_weights[smallest])
                    {
                        smallest = i;
                    }
                }

                if (weight > m_weights[smallest])
                {
                    m_indices[smallest] = index;
                    m_weights[smallest] = weight;
                }
            }
        }

        void normalize()
        {
            float weightSum = 0.0f;
            for (uint32_t i=0; i<ARRAY_COUNT(m_indices); ++i)
            {
                weightSum += m_weights[i];
            }

            if (weightSum > 0)
            {
                for (uint32_t i=0; i<ARRAY_COUNT(m_indices); ++i)
                {
                    m_weights[i] /= weightSum;
                }
            }
        }
    };

#if 0
    void DataNode::exportToMeshBuilder(const LoadedFile& owner, const base::Matrix& fileToWorld, rendering::content::MeshGeometryBuilder& outGeonetry, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials, bool forceSkinToNode) const
    {
        // no mesh data
        if (!m_mesh)
            return;

        // get number of vertices
        uint32_t numVertices = m_mesh->GetControlPointsCount();
        const FbxVector4* pControlPoints = m_mesh->GetControlPoints();
        TRACE_INFO("FBX: Found {} control points (vertices) in {}", numVertices, name);

        // get number of polygons
        uint32_t numPolygons = m_mesh->GetPolygonCount();
        TRACE_INFO("FBX: Found {} polygons in {}", numPolygons, name);

        // get mesh flags
        auto hasNormal = (m_mesh->GetElementNormalCount() >= 1);
        auto hasTangents = (m_mesh->GetElementTangentCount() >= 1);
        auto hasBitangents = (m_mesh->GetElementBinormalCount() >= 1);
        auto hasUV0 = (m_mesh->GetElementUVCount() >= 1);
        auto hasUV1 = (m_mesh->GetElementUVCount() >= 2);
        auto hasColor = (m_mesh->GetElementVertexColorCount() >= 1);
        TRACE_INFO("FBX: HasNormals: {}", hasNormal);
        TRACE_INFO("FBX: HasTangents: {}", hasTangents);
        TRACE_INFO("FBX: HasBitangents: {}", hasBitangents);
        TRACE_INFO("FBX: HasUV0: {}", hasUV0);
        TRACE_INFO("FBX: HasUV1: {}", hasUV1);
        TRACE_INFO("FBX: HasColor: {}", hasColor);

        // generate some automatic stuff if missing from mesh
        outGeonetry.toggleNormalsComputation(!hasNormal);
        outGeonetry.toggleTangentSpaceComputation(!hasNormal || !hasTangents || !hasBitangents);
        outGeonetry.toggleUVGeneration(rendering::content::MeshStreamType::TexCoord0_2F, !hasUV0);

        // prepare skin tables
        base::Array<MeshVertexInfluence> skinInfluences;

        // flip faces ?
        auto fullMatrix = localToWorld * fileToWorld;
        auto flipFaces = fullMatrix.det3() < 0.0f;

        // extarct skinning
        if (forceSkinToNode)
        {
            TRACE_INFO("FBX: Binding all vertices to node");

            // prepare influence table
            skinInfluences.resize(numVertices);
            memzero(skinInfluences.data(), skinInfluences.dataSize());

            // map the single bone (node)
            auto boneIndex = outSkeleton.addBone(owner, this);
            for (uint32_t i=0; i<numVertices; ++i)
                skinInfluences[i].add(boneIndex, 1.0f);
        }
        else if (m_mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
        {
            const FbxSkin* pSkin = pSkin = (const FbxSkin*) m_mesh->GetDeformer(0, FbxDeformer::eSkin);
            TRACE_INFO("FBX: Found skinning ({} clusters)", pSkin->GetClusterCount());

            // prepare influence table
            skinInfluences.resize(numVertices);
            memzero(skinInfluences.data(), skinInfluences.dataSize());

            // load influences
            uint32_t numTotalInfluences = 0;
            uint32_t numClusters =  pSkin->GetClusterCount();
            for (uint32_t i=0; i<numClusters; ++i)
            {
                const FbxCluster* pCluster = pSkin->GetCluster(i);

                const FbxNode* pLinkNode = pCluster->GetLink();
                if (!pLinkNode)
                    continue;

                FbxAMatrix linkMatrix;
                pCluster->GetTransformLinkMatrix(linkMatrix);

                uint32_t globalBoneIndex = outSkeleton.addBone(owner, pLinkNode);//, linkMatrix);
                //LOG(TXT("Cluster %d link to '%s' mapped as %d"), i, ANSI_TO_UNICODE(pLinkNode->GetName()), globalBoneIndex);

                uint32_t numInfluences = pCluster->GetControlPointIndicesCount();
                const int* pIndices = pCluster->GetControlPointIndices();
                const double* pWeights = pCluster->GetControlPointWeights();
                for (uint32_t j=0; j<numInfluences; ++j)
                {
                    uint32_t pointIndex = pIndices[j];
                    float weight = (float)pWeights[j];
                    skinInfluences[pointIndex].add(globalBoneIndex, weight);
                    numTotalInfluences += 1;
                }
            }

            TRACE_INFO( "FBX: Loaded {} total skin influences", numTotalInfluences);
        }

        // check if all materials on mesh are the same (faster loading)
        bool bIsAllSame = true;
        if (m_mesh->GetElementMaterialCount() > 0)
        {
            const FbxGeometryElementMaterial* pMaterialElement = m_mesh->GetElementMaterial(0);
            if (pMaterialElement->GetMappingMode() == FbxGeometryElement::eByPolygon)
            {
                TRACE_INFO( "FBX: Mesh {} has per-polygon material mapping", name);
                bIsAllSame = false;
            }
        }

        // check if the same material is used for the whole mesh
        uint32_t meshMaterialID = 0;
        if (bIsAllSame)
        {
            const FbxGeometryElementMaterial* lMaterialElement = m_mesh->GetElementMaterial(0);
            if (lMaterialElement && lMaterialElement->GetMappingMode() == FbxGeometryElement::eAllSame)
            {
                FbxSurfaceMaterial* lMaterial = m_mesh->GetNode()->GetMaterial(lMaterialElement->GetIndexArray().GetAt(0));
                meshMaterialID = lMaterialElement->GetIndexArray().GetAt(0);
                TRACE_INFO("FBX: Mesh uses material {} for all it's content", meshMaterialID);
            }
        }

        // get input data streams
        auto pNormals  = hasNormal ? m_mesh->GetElementNormal(0) : NULL;
        auto pTangents  = hasTangents ? m_mesh->GetElementTangent(0) : NULL;
        auto pBitangents  = hasBitangents ? m_mesh->GetElementBinormal(0) : NULL;
        auto pColors  = hasColor ? m_mesh->GetElementVertexColor(0) : NULL;
        auto pUV0  = hasUV0 ? m_mesh->GetElementUV(0) : NULL;
        auto pUV1  = hasUV1 ? m_mesh->GetElementUV(1) : NULL;

        // Temp vertices storage
        base::InplaceArray<rendering::content::MeshBuilderVertex, 10> tempVertices;

        // prepare material mapping
        base::Array<int> materialMapping;
        materialMapping.resizeWith(m_mesh->GetNode()->GetMaterialCount()+1, INDEX_NONE);

        // extract polygons (and create triangles)
        uint32_t vertexIndex = 0;
        uint32_t numTriangles = 0;
        int activeMaterial = -1;
        for (uint32_t i=0; i<numPolygons; i++)
        {
            // face vertex count
            uint32_t polygonSize = m_mesh->GetPolygonSize(i);
            if (polygonSize > tempVertices.size())
                tempVertices.resize(polygonSize);

            // get material used on the face
            uint32_t polygonMaterialID = meshMaterialID;
            if (!bIsAllSame)
            {
                const FbxGeometryElementMaterial* pMaterialElement = m_mesh->GetElementMaterial(0);
                polygonMaterialID = pMaterialElement->GetIndexArray().GetAt(i);
                if (polygonMaterialID < 0 || polygonMaterialID >= materialMapping.lastValidIndex())
                    polygonMaterialID = 0;
            }

            // map polygon material
            auto realPolygonMaterialId = materialMapping[polygonMaterialID];
            if (realPolygonMaterialId == -1)
            {
                auto lMaterial  = m_mesh->GetNode()->GetMaterial(polygonMaterialID);
                auto globalMaterialIndex = outMaterials.addMaterial(lMaterial);

                realPolygonMaterialId = outGeonetry.addMaterial(lMaterial->GetName());
                materialMapping[polygonMaterialID] = realPolygonMaterialId;
            }

            // switch material
            if (activeMaterial != realPolygonMaterialId)
            {
                activeMaterial = realPolygonMaterialId;
                outGeonetry.selectMaterial((rendering::content::LocalID)activeMaterial);
            }

            // Extract polygon vertices
            for (uint32_t j=0; j<polygonSize; j++, vertexIndex++)
            {
                auto controlPointIndex = m_mesh->GetPolygonVertex(i, j);

                // extract position
                auto& v = tempVertices[j];
                v.Position = fullMatrix.transformPoint(ToVector(pControlPoints[controlPointIndex]));

                // skinning
                if (!skinInfluences.empty())
                {
                    for (uint32_t k=0; k<4; ++k)
                    {
                        v.Skinning[k].indexValue = skinInfluences[controlPointIndex].indices[k];
                        v.Skinning[k].m_weight = skinInfluences[controlPointIndex].m_weights[k];
                    }
                }

                // extract first normal
                if (hasNormal)
                {
                    FbxVector4 normalData(0,0,0,0);

                    if (pNormals->GetMappingMode() == FbxGeometryElement::eByControlPoint)
                    {
                        switch (pNormals->GetReferenceMode())
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                normalData = pNormals->GetDirectArray().GetAt(controlPointIndex);
                                break;
                            }

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                uint32_t id = pNormals->GetIndexArray().GetAt(controlPointIndex);
                                normalData = pNormals->GetDirectArray().GetAt(id);
                                break;
                            }
                        }
                    }
                    else if (pNormals->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
                    {
                        switch (pNormals->GetReferenceMode())
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                normalData = pNormals->GetDirectArray().GetAt(vertexIndex);
                                break;
                            }

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                uint32_t id = pNormals->GetIndexArray().GetAt(vertexIndex);
                                normalData = pNormals->GetDirectArray().GetAt(id);
                                break;
                            }
                        }
                    }

                    v.Normal = fullMatrix.transformVector(ToVector(normalData)).normalized();
                }

                // extract tangents
                if (hasTangents && hasBitangents)
                {
                    FbxVector4 tangentData(0,0,0,0);

                    if (pTangents->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
                    {
                        switch (pTangents->GetReferenceMode())
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                tangentData = pTangents->GetDirectArray().GetAt(vertexIndex);
                                break;
                            }

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                uint32_t id = pTangents->GetIndexArray().GetAt(vertexIndex);
                                tangentData = pTangents->GetDirectArray().GetAt(id);
                                break;
                            }
                        }
                    }
                    else if (pTangents->GetMappingMode() == FbxGeometryElement::eByControlPoint)
                    {
                        switch (pTangents->GetReferenceMode())
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                tangentData = pTangents->GetDirectArray().GetAt(controlPointIndex);
                                break;
                            }

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                uint32_t id = pTangents->GetIndexArray().GetAt(controlPointIndex);
                                tangentData = pTangents->GetDirectArray().GetAt(id);
                                break;
                            }
                        }
                    }

                    FbxVector4 bitangentData(0,0,0,0);
                    if (pBitangents->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
                    {
                        switch (pBitangents->GetReferenceMode())
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                bitangentData = pBitangents->GetDirectArray().GetAt(vertexIndex);
                                break;
                            }

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                uint32_t id = pBitangents->GetIndexArray().GetAt(vertexIndex);
                                bitangentData = pBitangents->GetDirectArray().GetAt(id);
                                break;
                            }
                        }
                    }
                    else if (pBitangents->GetMappingMode() == FbxGeometryElement::eByControlPoint)
                    {
                        switch (pBitangents->GetReferenceMode())
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                bitangentData = pBitangents->GetDirectArray().GetAt(controlPointIndex);
                                break;
                            }

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                uint32_t id = pBitangents->GetIndexArray().GetAt(controlPointIndex);
                                bitangentData = pBitangents->GetDirectArray().GetAt(id);
                                break;
                            }
                        }
                    }

                    // Store
                    v.Tangent = fullMatrix.transformVector(ToVector(tangentData)).normalized();
                    v.Binormal = fullMatrix.transformVector(ToVector(bitangentData)).normalized();
                }

                // extarct UV0
                if (hasUV0)
                {
                    FbxVector2 uvData(0,0);

                    if (pUV0->GetMappingMode() == FbxGeometryElement::eByControlPoint)
                    {
                        switch (pUV0->GetReferenceMode())
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                uvData = pUV0->GetDirectArray().GetAt(controlPointIndex);
                                break;
                            }

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                uint32_t id = pUV0->GetIndexArray().GetAt(controlPointIndex);
                                uvData = pUV0->GetDirectArray().GetAt(id);
                                break;
                            }
                        }
                    }
                    else if (pUV0->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
                    {
                        uint32_t textureUVIndex = ((FbxMesh*)m_mesh)->GetTextureUVIndex(i, j);
                        if (pUV0->GetReferenceMode() == FbxGeometryElement::eDirect ||
                            pUV0->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                        {
                            uvData = pUV0->GetDirectArray().GetAt(textureUVIndex);
                        }
                    }

                    v.TexCoord[0].x = (float)uvData[0];
                    v.TexCoord[0].y = 1.0f - (float)uvData[1];
                }

                // extract UV1
                if (hasUV1)
                {
                    FbxVector2 uvData(0,0);

                    if (pUV1->GetMappingMode() == FbxGeometryElement::eByControlPoint)
                    {
                        switch (pUV1->GetReferenceMode())
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                uvData = pUV1->GetDirectArray().GetAt(controlPointIndex);
                                break;
                            }

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                uint32_t id = pUV1->GetIndexArray().GetAt(controlPointIndex);
                                uvData = pUV1->GetDirectArray().GetAt(id);
                                break;
                            }
                        }
                    }
                    else if (pUV1->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
                    {
                        uint32_t textureUVIndex = ((FbxMesh*)m_mesh)->GetTextureUVIndex(i, j);
                        if (pUV1->GetReferenceMode() == FbxGeometryElement::eDirect ||
                            pUV1->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
                        {
                            uvData = pUV1->GetDirectArray().GetAt(textureUVIndex);
                        }
                    }

                    v.TexCoord[1].x = (float)uvData[0];
                    v.TexCoord[1].y = 1.0f - (float)uvData[1];
                }

                // extract vertex color
                if (hasColor)
                {
                    FbxColor colorData(1,1,1,1);

                    if (pColors->GetMappingMode() == FbxGeometryElement::eByControlPoint)
                    {
                        switch (pColors->GetReferenceMode())
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                colorData = pColors->GetDirectArray().GetAt(controlPointIndex);
                                break;
                            }

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                uint32_t id = pColors->GetIndexArray().GetAt(controlPointIndex);
                                colorData = pColors->GetDirectArray().GetAt(id);
                                break;
                            }
                        }
                    }
                    else if (pColors->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
                    {
                        switch (pColors->GetReferenceMode())
                        {
                            case FbxGeometryElement::eDirect:
                            {
                                colorData = pColors->GetDirectArray().GetAt(vertexIndex);
                                break;
                            }

                            case FbxGeometryElement::eIndexToDirect:
                            {
                                uint32_t id = pColors->GetIndexArray().GetAt(vertexIndex);
                                colorData = pColors->GetDirectArray().GetAt(id);
                                break;
                            }
                        }
                    }

                    v.Color[0].r = (uint8_t)std::clamp<float>(colorData.mRed * 255.0f, 0.0f, 255.0f);
                    v.Color[0].g = (uint8_t)std::clamp<float>(colorData.mGreen * 255.0f, 0.0f, 255.0f);
                    v.Color[0].b = (uint8_t)std::clamp<float>(colorData.mBlue * 255.0f, 0.0f, 255.0f);
                    v.Color[0].a = (uint8_t)std::clamp<float>(colorData.mAlpha * 255.0f, 0.0f, 255.0f);
                }
            }

            // Triangulate
            for (uint32_t j=2; j<polygonSize; j++)
            {
                if (flipFaces)
                    outGeonetry.addTriangle(tempVertices[j], tempVertices[j - 1], tempVertices[0]);
                else
                    outGeonetry.addTriangle(tempVertices[0], tempVertices[j - 1], tempVertices[j]);
            }
        }

        // Done
        TRACE_INFO("FBX: Extracted {} vertices and {} triangles from {}", vertexIndex, numTriangles, name);
    }
#endif
    //---

    LoadedFile::LoadedFile(fbxsdk::FbxScene* fbxScene/* = nullptr*/)
        : m_fbxScene(fbxScene)
    {
        m_nodes.reserve(256);
        m_nodeMap.reserve(256);
    }

	bool GFBXSceneClosed = false;

    LoadedFile::~LoadedFile()
    {
        if (m_fbxScene)
        {
			if (!GFBXSceneClosed)
				m_fbxScene->Destroy();
            m_fbxScene = nullptr;
        }
    }

    bool LoadedFile::captureNodes(const base::Matrix& spaceConversionMatrix)
    {
        if (!m_nodes.empty())
            return true;

        if (!m_fbxScene)
            return false;

        auto rootNode = m_fbxScene->GetRootNode();
        return walkStructure(base::Matrix::IDENTITY(), spaceConversionMatrix, rootNode, nullptr);
    }

    const DataNode* LoadedFile::findDataNode(const fbxsdk::FbxNode* fbxNode) const
    {
        const DataNode* ret = nullptr;
        m_nodeMap.find(fbxNode, ret);
        return ret;
    }

    bool LoadedFile::walkStructure(const base::Matrix& worldToParent, const base::Matrix& spaceConversionMatrix, const fbxsdk::FbxNode* node, DataNode* parentDataNode)
    {
        // get the local to parent matrix from FBX node
        // NOTE: this code is still shit...
        FbxAMatrix matrix = ((FbxNode*)node)->EvaluateGlobalTransform(FBXSDK_TIME_INFINITE, FbxNode::eSourcePivot, false, true);
        auto localToWorld = ToMatrix(matrix) * spaceConversionMatrix;

        // create the local node
        auto localNode  = MemNew(DataNode);
        localNode->m_localToWorld = localToWorld;
        localNode->m_localToParent = localToWorld * worldToParent;
        localNode->m_name = base::StringView<char>(node->GetName());
        localNode->m_parent = parentDataNode;
        if (parentDataNode)
            parentDataNode->m_children.pushBack(localNode);
        m_nodes.pushBack(localNode);

        // map
        localNode->m_node = node;
        m_nodeMap[node] = localNode;

        // extract mesh at this node
        if (node->GetNodeAttribute() != NULL)
        {
            if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
            {
                const FbxMesh *mesh = (const FbxMesh *) node->GetNodeAttribute();
                localNode->m_mesh = mesh;

                // remove pivot bullshit
                ((FbxMesh *) mesh)->ApplyPivot();

                // skip
                if (localNode->m_name.view().endsWithNoCase("_tri"))
                {
                    TRACE_INFO("Found triangle collision mesh data on node '{}'", localNode->m_name);
                    localNode->m_type = DataNodeType::TriangleCollision;
                }
                else if (localNode->m_name.view().endsWithNoCase("_convex") || localNode->m_name.view().endsWithNoCase("_col"))
                {
                    TRACE_INFO("Found convex collision mesh data on node '{}'", localNode->m_name);
                    localNode->m_type = DataNodeType::ConvexCollision;
                }
                else
                {
                    localNode->m_type = DataNodeType::VisualMesh;

                    if (localNode->m_name.view().endsWithNoCase("_lod0"))
                        localNode->m_lodIndex = 0;
                    else if (localNode->m_name.view().endsWithNoCase("_lod1"))
                        localNode->m_lodIndex = 1;
                    else if (localNode->m_name.view().endsWithNoCase("_lod2"))
                        localNode->m_lodIndex = 2;
                    else if (localNode->m_name.view().endsWithNoCase("_lod3"))
                        localNode->m_lodIndex = 3;
                    else if (localNode->m_name.view().endsWithNoCase("_lod4"))
                        localNode->m_lodIndex = 4;
                    else if (localNode->m_name.view().endsWithNoCase("_lod5"))
                        localNode->m_lodIndex = 5;
                    else if (localNode->m_name.view().endsWithNoCase("_lod6"))
                        localNode->m_lodIndex = 6;
                    else if (localNode->m_name.view().endsWithNoCase("_lod7"))
                        localNode->m_lodIndex = 7;


                    TRACE_INFO("Found visual mesh (LOD{}) on node '{}'", localNode->m_lodIndex, localNode->m_name);
                }
            }
        }

        // visit children
        uint32_t numChildren = node->GetChildCount();
        if (numChildren)
        {
            auto worldToParent = localNode->m_localToWorld.inverted();
            for (uint32_t i = 0; i < numChildren; ++i)
            {
                if (const FbxNode *childNode = node->GetChild(i))
                    if (!walkStructure(worldToParent, spaceConversionMatrix, childNode, localNode))
                        return false;
            }
        }

        // nothing broken found
        return true;
    }    

    //---

} // fbx
