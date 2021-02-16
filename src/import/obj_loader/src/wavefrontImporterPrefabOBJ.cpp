/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import #]
***/

#include "build.h"

#include "wavefrontFormatOBJ.h"
#include "wavefrontImporterOBJ.h"
#include "wavefrontImporterPrefabOBJ.h"

#include "base/world/include/worldPrefab.h"
#include "base/resource/include/resourceTags.h"
#include "base/containers/include/bitSet.h"
#include "rendering/mesh/include/renderingMesh.h"
#include "import/mesh_loader/include/renderingMeshImportConfig.h"
#include "import/mesh_loader/include/renderingMaterialImportConfig.h"

#include "icp/icp.h"
#include "base/resource/include/objectIndirectTemplate.h"

#pragma optimize("", off)

namespace wavefront
{
    //--

    RTTI_BEGIN_TYPE_CLASS(OBJPrefabImportConfig);
        RTTI_CATEGORY("Topology");
        RTTI_PROPERTY(forceTriangles).editable("Force triangles at output, even if we had quads").overriddable();
        RTTI_CATEGORY("Texture mapping");
        RTTI_PROPERTY(flipUV).editable("Flip V channel of the UVs").overriddable();
        RTTI_CATEGORY("Mesh extraction");
        RTTI_PROPERTY(reuseIdenticalMeshes).editable("Try to detect similar meshes and reuse them").overriddable();
        RTTI_PROPERTY(unrotateMeshes).editable("Axis align extracted mesh (yaw only)");
        RTTI_PROPERTY(positionAlignX).editable("Align pivot position - x coordinate");
        RTTI_PROPERTY(positionAlignY).editable("Align pivot position - y coordinate");
        RTTI_PROPERTY(positionAlignZ).editable("Align pivot position - z coordinate");
    RTTI_END_TYPE();

    OBJPrefabImportConfig::OBJPrefabImportConfig()
    {
        m_meshImportPath = StringBuf("../meshes/");
        m_materialImportMode = rendering::MeshMaterialImportMode::ImportAll;
        m_textureImportMode = rendering::MaterialTextureImportMode::ImportAll;
    }

    void OBJPrefabImportConfig::computeConfigurationKey(CRC64& crc) const
    {
        TBaseClass::computeConfigurationKey(crc);

        crc << forceTriangles;
        crc << flipUV;

        crc << reuseIdenticalMeshes;
        crc << unrotateMeshes;

        crc << positionAlignX;
        crc << positionAlignY;
        crc << positionAlignZ;
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(OBJPrefabImporter);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<base::world::Prefab>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("obj");
        RTTI_METADATA(base::res::ResourceCookerVersionMetadata).version(1);
        RTTI_METADATA(base::res::ResourceImporterConfigurationClassMetadata).configurationClass<OBJPrefabImportConfig>();
    RTTI_END_TYPE();

    OBJPrefabImporter::OBJPrefabImporter()
    {
    }

    //--

    struct SourceMeshCharacteristic
    {
        uint32_t numFaces = 0;
        uint32_t numVertices = 0;
        uint32_t size = 0;
        base::Vector3 center;
        base::Box box;

        INLINE bool operator==(const SourceMeshCharacteristic& other) const
        {
            return (numVertices == other.numVertices) && (numFaces == other.numFaces) && (size == other.size);
        }

        INLINE bool operator!=(const SourceMeshCharacteristic& other) const
        {
            return !operator==(other);
        }

        INLINE static uint32_t CalcHash(const SourceMeshCharacteristic& key)
        {
            base::CRC32 crc;
            crc << key.numFaces;
            crc << key.numVertices;
            crc << key.size;
            return crc;
        }
    };

    struct SourceMeshTriangle
    {
        base::Vector3 p[3];
        mutable base::Vector3 pt[3];
        uint32_t v[3] = { 0,0,0 };
        uint32_t m = 0;
        base::Vector3 n;
        float a = 0.0f;

        INLINE bool operator<(const SourceMeshTriangle& other) const
        {
            return a > other.a; // put the larger stuff first
        }

        float distanceTo(const base::Vector3& p0, const base::Vector3& p1, const base::Vector3& p2) const
        {
            const auto f0 = p[0].squareDistance(p0);
            const auto f1 = p[1].squareDistance(p1);
            const auto f2 = p[2].squareDistance(p2);
            return std::max(std::max(f0, f1), f2);
        }

        float distanceTo(const SourceMeshTriangle& otherTri, const base::Matrix& otherToLocal) const
        {
            auto p0 = otherToLocal.transformPoint(otherTri.p[0]);
            auto p1 = otherToLocal.transformPoint(otherTri.p[1]);
            auto p2 = otherToLocal.transformPoint(otherTri.p[2]);

            const auto f0 = distanceTo(p0, p1, p2);
            const auto f1 = distanceTo(p1, p2, p0);
            const auto f2 = distanceTo(p2, p0, p1);
            return std::min(std::min(f0, f1), f2);
        }

        void calcPlaneSpace(int edge, base::Matrix& outTriangleToWorld) const
        {
            const auto posA = p[(0 + edge) % 3];
            const auto posB = p[(1 + edge) % 3];
            const auto posC = p[(2 + edge) % 3];

            const auto tn = base::TriangleNormal(posA, posB, posC);
            const auto tu = (posB - posA).normalized();
            const auto tv = base::Cross(tn, tu).normalized();

            outTriangleToWorld = base::Matrix33(tu, tv, tn);
            outTriangleToWorld.translation(posA);
        }
    };

    static void CalcMeshCharacteristic(const Array<SourceMeshTriangle>& triangles, const Array<base::Vector3>& vertices, SourceMeshCharacteristic& outInfo)
    {
        outInfo.numVertices = vertices.size();
        outInfo.numFaces = triangles.size();

        Box box;
        double cx = 0, cy = 0, cz = 0;
        for (const auto& localPos : vertices)
        {
            cx += localPos.x;
            cy += localPos.y;
            cz += localPos.z;
            box.merge(localPos);
        }

        if (outInfo.numVertices)
        {
            cx /= (double)outInfo.numVertices;
            cy /= (double)outInfo.numVertices;
            cz /= (double)outInfo.numVertices;
        }
        else
        {
            cx = 0;
            cy = 0;
            cz = 0;
        }

        outInfo.center.x = cx;
        outInfo.center.y = cy;
        outInfo.center.z = cz;
        outInfo.box = box;

        auto extents = box.extents();
        extents.x = 1.0f / std::max<float>(0.001f, extents.x);
        extents.y = 1.0f / std::max<float>(0.001f, extents.y);
        extents.z = 1.0f / std::max<float>(0.001f, extents.z);

        double averageDistanceDistribution = 0.0f;
        for (const auto& localPos : vertices)
        {
            float dx = (localPos.x - cx) * extents.x;
            float dy = (localPos.y - cy) * extents.y;
            float dz = (localPos.z - cz) * extents.z;

            float dist = dx * dx + dy * dy + dz * dz;
            averageDistanceDistribution += dist;
        }

        averageDistanceDistribution /= (double)outInfo.numVertices;
        outInfo.size = (uint32_t)(averageDistanceDistribution * 100);
    }

    //--

    static void FindAligningRotation(const base::Array<SourceMeshTriangle>& triangles, float& outYaw)
    {
        const float yawMaxStep = 5.0f;

        float yaw = 0.0f;
        float yawStep = 5.0f;
        double prevAlignedArea = 0.0;
        double prevAlignStep = 0.0;

        const auto maxSteps = 100;
        for (int i=0; i<maxSteps; ++i)
        {
            const auto angles = base::Angles(0.0f, -yaw, 0.0f);
            const auto dir = angles.forward();

            const auto AlignMargin = 0.1f;

            double alignedArea = 0.0;
            double totalArea = 0.0;
            for (const auto& tri : triangles)
            {
                auto flatN = base::Vector3(tri.n.x, tri.n.y, 0.0f);
                auto dot = base::Dot(flatN, dir);
                if (dot > (1.0f - AlignMargin))
                {
                    auto alignFactor = std::clamp<float>((1.0f - dot) / AlignMargin, 0.0f, 1.0f);
                    alignedArea += tri.a * alignFactor;
                }

                totalArea += tri.a;
            }

            alignedArea /= totalArea;
            if (alignedArea > 0.99999)
                break;

            TRACE_ERROR("AlignStep[{}]: Yaw {}, Step {}, Aligned {}, PrevAligned {}", i, yaw, yawStep, alignedArea, prevAlignedArea);

            if (i >= 1)
            {
                if (alignedArea > prevAlignedArea)
                {
                    auto step = alignedArea - prevAlignedArea;
                    if (step < prevAlignStep)
                        yawStep = yawStep * 0.5f;
                    else
                        yawStep = yawStep * 1.1f; // go a little bit faster

                    prevAlignStep = step;
                }
                else if (alignedArea < prevAlignedArea)
                {
                    yawStep = -yawStep * 0.5f;
                    prevAlignStep = 0.0;
                }
            }

            if (yawStep > yawMaxStep)
                yawStep = yawMaxStep;
            else if (yawStep < -yawMaxStep)
                yawStep = -yawMaxStep;

            yaw += yawStep;
            prevAlignedArea = alignedArea;
        }

        outYaw = yaw;
    }

    //--

    struct SourcePrefabMesh
    {
        base::StringBuf assetName;

        Group sourceGroup;
        SourceMeshCharacteristic key;

        base::Vector3 exportTranslation; // offset that brings the mesh vertices to origin
        base::Angles exportRotation; // rotation to un-rotate mesh from the model
        base::Matrix meshToWorld;

        rendering::MeshAsyncRef exportPath;
        uint32_t numObjects = 0;;

        base::Array<base::Vector3> vertices;
        base::Array<SourceMeshTriangle> triangles;

        SourcePrefabMesh(const FormatOBJ& data, const Group& g, const base::Matrix& assetToEngine)
            : sourceGroup(g)
        {
            triangles.reserve(key.numFaces*2); // worst case

            base::HashSet<uint32_t> uniquePositions;
            for (uint32_t i = 0; i < g.numChunks; ++i)
            {
                const auto& chunk = data.chunks()[g.firstChunk + i];

                const auto* fi = data.faceIndices() + chunk.firstFaceIndex;
                const auto* f = data.faces() + chunk.firstFace;
                for (uint32_t j = 0; j < chunk.numFaces; ++j)
                {
                    for (uint32_t k = 2; k < f->numVertices; ++k)
                    {
                        auto& tri = triangles.emplaceBack();

                        tri.m = chunk.material;

                        tri.v[0] = fi[0];
                        tri.v[1] = fi[(k-1) * chunk.numAttributes];
                        tri.v[2] = fi[k * chunk.numAttributes];

                        tri.p[0] = assetToEngine.transformPoint(data.positions()[tri.v[0]]);
                        tri.p[1] = assetToEngine.transformPoint(data.positions()[tri.v[1]]);
                        tri.p[2] = assetToEngine.transformPoint(data.positions()[tri.v[2]]);

                        const auto ba = (tri.p[1] - tri.p[0]);
                        const auto ca = (tri.p[2] - tri.p[0]);
                        tri.n = base::Cross(ba, ca);
                        tri.a = tri.n.length();
                        if (tri.a > 0.00001)
                            tri.n /= tri.a;

                        uniquePositions.insert(tri.v[0]);
                        uniquePositions.insert(tri.v[1]);
                        uniquePositions.insert(tri.v[2]);
                    }

                    fi += chunk.numAttributes * f->numVertices;
                    f += 1;
                }
            }

            vertices.reserve(uniquePositions.size());
            for (const auto id : uniquePositions)
                vertices.pushBack(assetToEngine.transformPoint(data.positions()[id]));

            //            std::stable_sort(outTriangles.getTypedData(), outTriangles.getTypedData() + outTriangles.size());

            CalcMeshCharacteristic(triangles, vertices, key);
        }

        float distanceTo(const SourcePrefabMesh& other, const Matrix& otherToLocal, float distanceCutOff) const
        {
            const auto numTris = other.triangles.size();

            base::BitSet<> matchedSourceTris;
            matchedSourceTris.resizeWithZeros(numTris);

            float worstTriangleDistance = 0.0f;

            const auto maxTrisToMatch = std::min<uint32_t>(numTris, 100U);
            for (uint32_t i=0; i < maxTrisToMatch; ++i)
            {
                const auto& targetTri = other.triangles[i];

                // try
                float bestMatchDistance = FLT_MAX;
                float matchScale = FLT_MAX;
                int bestMatchIndex = -1;
                for (uint32_t j = 0; j < maxTrisToMatch; ++j)
                {
                    if (!matchedSourceTris[j])
                    {
                        // at some points triangles become very small, stop matching them
                        const auto& sourceTri = triangles[j];
                        if (matchScale == FLT_MAX)
                            matchScale = sourceTri.a * 0.98f;
                        else if (sourceTri.a < matchScale)
                            break;

                        // material must match
                        if (targetTri.m == sourceTri.m)
                        {
                            const auto dist = sourceTri.distanceTo(targetTri, otherToLocal);
                            if (dist < bestMatchDistance)
                            {
                                bestMatchDistance = dist;
                                bestMatchIndex = j;
                            }
                        }
                    }
                }

                // no triangle was matched (invalid materials)
                if (bestMatchIndex == -1)
                {
                    TRACE_WARNING("Match: No triangle match for target tri %u", i);
                    return distanceCutOff;
                }

                // mark the triangle as matched
                matchedSourceTris.set(bestMatchIndex);
                worstTriangleDistance = std::max(worstTriangleDistance, bestMatchDistance);
                TRACE_WARNING("Match: Matched target triangle %u to source triangle %u, distance %f", i, bestMatchIndex, bestMatchDistance);

                // best distance if worse than cutoff there's no point continuing
                if (worstTriangleDistance > distanceCutOff)
                    return distanceCutOff;
            }

            // return the worst distance between triangles
            return worstTriangleDistance;
        }
        
        bool alignToICP(const SourcePrefabMesh& targetMesh, base::Matrix& outBestSourceToTargetTransform) const
        {
            base::Matrix ret;

            const auto dist = thirdparty::icp::CalcPoint2PointRigidTransform(
                (const float*)vertices.typedData(), vertices.size(),
                (const float*)targetMesh.vertices.typedData(), targetMesh.vertices.size(),
                (float*)&ret);

            if (dist > 0.1f)
                return false;

            outBestSourceToTargetTransform = ret;
            return true;
        }

        bool alignTo(const SourcePrefabMesh& targetMesh, base::Matrix& outBestSourceToTargetTransform) const
        {
            base::Matrix bestTransform;
            float bestTransformDistance = 0.2f;
            bool hasValidBestTransformDistance = false;

            // calculate orientation of the target triangle
            // the target triangle is in the target mesh space
            base::Matrix targetTriangleToWorld, worldToTargetTriangle;
            targetMesh.triangles[0].calcPlaneSpace(0, targetTriangleToWorld);
            worldToTargetTriangle = targetTriangleToWorld.inverted();

            // test all triangles that are large enough to be considered a match for the target triangle
            const auto sourceTriangleSizeCutoff = triangles[0].a * 0.99f;
            for (uint32_t sourceTriangleIndex = 0; sourceTriangleIndex < triangles.size(); ++sourceTriangleIndex)
            {
                // if the triangle is to small relative to the target we can't match it
                TRACE_INFO("Match: Testing source triangle %u (%f>%f)", sourceTriangleIndex, triangles[sourceTriangleIndex].a, sourceTriangleSizeCutoff);
                if (triangles[sourceTriangleIndex].a < sourceTriangleSizeCutoff)
                    break;

                // test 3 orientations of the source triangle
                for (uint32_t sourceOrientation = 0; sourceOrientation < 3; ++sourceOrientation)
                {
                    base::Matrix sourceTriangleToWorld, worldToSourceTriangle;
                    triangles[sourceTriangleIndex].calcPlaneSpace(sourceOrientation, sourceTriangleToWorld);

                    // calculate transform that (hopefully) aligns source triangles with target triangles
                    const auto sourceToTarget = sourceTriangleToWorld * worldToTargetTriangle;

                    // calculate the relative transform between the object and the mesh
                    const auto meshDistance = targetMesh.distanceTo(*this, sourceToTarget, bestTransformDistance);
                    if (meshDistance < bestTransformDistance)
                    {
                        TRACE_WARNING("Match: Best distance %f for transform from source %u, orientation %u",
                            meshDistance, sourceTriangleIndex, sourceOrientation);

                        hasValidBestTransformDistance = true;
                        bestTransformDistance = meshDistance;
                        bestTransform = sourceToTarget;

                        // short circuit if we got really close
                        if (bestTransformDistance < 0.01f)
                        {
                            outBestSourceToTargetTransform = bestTransform;
                            return true;
                        }
                    }
                }
            }

            if (hasValidBestTransformDistance)
                outBestSourceToTargetTransform = bestTransform;

            return hasValidBestTransformDistance;
        }
    };

    //--

    struct SourcePrefabObject
    {
        base::StringBuf fullName;

        Group group;

        const SourcePrefabMesh* mesh = nullptr;
        uint32_t meshInstanceIndex = 0;

        base::Matrix objectToMesh;
    };

    //--

    static base::StringBuf CreateAssetName(const base::StringBuf& groupName, base::HashSet<base::StringBuf>& usedNames)
    {
        auto name = groupName.toLower();
        //auto name = groupName;//.stringBeforeFirst("_").toLower();
        name.replaceChar(' ', '_');

        auto testName = name;
        uint32_t index = 0;
        for (;;)
        {
            if (usedNames.insert(testName))
                return testName;

            index += 1;
            testName = base::StringBuf(base::TempString("{}_ver{}", name, index));
        }
    }

    //--

    static base::StringBuf BuildeMeshFileName(base::StringView name, uint32_t meshIndex)
    {
        static const auto ext = base::res::IResource::GetResourceExtensionForClass(rendering::Mesh::GetStaticClass());

        base::StringBuf fileName;
        if (!base::MakeSafeFileName(name, fileName))
            fileName = base::TempString("mesh{}", meshIndex);

        ASSERT(ValidateFileName(fileName));

        return TempString("{}.{}", fileName, ext);
    }

    extern void EmitDepotPath(const base::Array<base::StringView>& pathParts, base::IFormatStream& f);
    extern void GlueDepotPath(base::StringView path, bool isFileName, base::Array<base::StringView>& outPathParts);
    extern base::StringBuf BuildAssetDepotPath(base::StringView referenceDepotPath, base::StringView meshImportPath, base::StringView meshFileName);

    //--
    
    static float CalcAlignedPos(const base::Box& box, const base::Vector3& center, int axis, int align)
    {
        if (align < 0)
            return box.min[axis];
        else if (align > 0)
            return box.max[axis];
        else
            return center[axis];
    }

    base::res::ResourcePtr OBJPrefabImporter::importResource(base::res::IResourceImporterInterface& importer) const
    {
        // load source data from OBJ format
        auto sourceFilePath = importer.queryImportPath();
        auto sourceGeometry = base::rtti_cast<wavefront::FormatOBJ>(importer.loadSourceAsset(importer.queryImportPath()));
        if (!sourceGeometry)
            return nullptr;

        // get the configuration for mesh import
        auto prefabManifest = importer.queryConfigration<OBJPrefabImportConfig>();

        // calculate the transform to apply to source data
        // TODO: move to config file!
        auto defaultSpace = rendering::MeshImportSpace::LeftHandYUp;
        if (importer.queryResourcePath().view().beginsWith("/engine/"))
            defaultSpace = rendering::MeshImportSpace::RightHandZUp;

        // calculate asset transformation to engine space
        auto assetToEngineTransform = prefabManifest->calcAssetToEngineConversionMatrix(rendering::MeshImportUnits::Meters, defaultSpace);

        ///--

        // extract mesh information
        // TODO: threads
        TRACE_INFO("Analyzing {} objects, {} groups", sourceGeometry->objects().size(), sourceGeometry->groups().size());
        base::Array<SourcePrefabObject> prefabSourceObjects;
        for (const auto& g : sourceGeometry->groups())
        {
            /*if ((g.m_fullName.findStrNoCase("object__lod0_italian_cypress") != -1) ||
                (g.m_fullName.findStrNoCase("object_ivy_ivy") != -1) ||
                (g.m_fullName.findStrNoCase("object_bux_hedge_box_long") != -1) ||
                (g.m_fullName.findStrNoCase("object_bux_hedge_ball_small") != -1))*/
            {
                auto& obj = prefabSourceObjects.emplaceBack();
                obj.fullName = g.name;
                obj.group = g;
            }
        }

        ///--

        // find unique meshes
        base::Array<SourcePrefabMesh*> finalMeshes;
        base::HashMap<SourceMeshCharacteristic, base::Array<SourcePrefabMesh*>> meshes;
        base::HashSet<base::StringBuf> usedAssetNames;
        for (auto& o : prefabSourceObjects)
        {
            // create mesh from source's object data
            // NOTE: we don't know yet if mesh is unique or not
            auto* mesh = new SourcePrefabMesh(*sourceGeometry, o.group, assetToEngineTransform);

            // try to match with existing asset and use it
            auto& potentialMeshes = meshes[mesh->key];
            for (auto* potentialMesh : potentialMeshes)
            {
                base::Matrix objectToMesh;
                //if (mesh->alignTo(*potentialMesh, objectToMesh))
                if (mesh->alignToICP(*potentialMesh, objectToMesh))
                {
                    TRACE_INFO("Object '{}' registered with mesh '{}' ({} uses)", o.fullName, potentialMesh->assetName, potentialMesh->numObjects + 1);

                    o.meshInstanceIndex = potentialMesh->numObjects;
                    o.objectToMesh = objectToMesh;
                    o.mesh = potentialMesh;

                    potentialMesh->numObjects += 1;
                    break;
                }
            }

            // not aligned ? add new mesh
            if (!o.mesh)
            {
                mesh->assetName = CreateAssetName(o.fullName, usedAssetNames);
                mesh->numObjects = 1;

                o.meshInstanceIndex = 0;
                o.mesh = mesh;
                o.objectToMesh = base::Matrix::IDENTITY();

                finalMeshes.pushBack(mesh);
                potentialMeshes.pushBack(mesh);
            }
            else
            {
                delete mesh;
            }
        }

        // finalize mesh setup
        TRACE_INFO("Found {} unique meshes", finalMeshes.size());
        uint32_t meshIndex = 0;
        for (auto* mesh : finalMeshes)
        {
            // compute mesh placement
            mesh->exportTranslation.x = CalcAlignedPos(mesh->key.box, mesh->key.center, 0, prefabManifest->positionAlignX);
            mesh->exportTranslation.y = CalcAlignedPos(mesh->key.box, mesh->key.center, 1, prefabManifest->positionAlignY);
            mesh->exportTranslation.z = CalcAlignedPos(mesh->key.box, mesh->key.center, 2, prefabManifest->positionAlignZ);

            // find best un-rotation fo mesh
            /*{
                float unrotateYaw = 0.0f;
                FindAligningRotation(mesh->triangles, unrotateYaw);
                mesh->exportRotation.yaw = unrotateYaw;
            }*/

            mesh->exportTranslation -= mesh->exportRotation.toMatrix().transformInvVector(mesh->exportTranslation);

            // calculate placement to add to any instance of this mesh to take into account that asset will be moved
            mesh->meshToWorld = mesh->exportRotation.toMatrix();
            mesh->meshToWorld.translation(mesh->exportTranslation);

            // build export path
            {
                const auto meshFileName = BuildeMeshFileName(mesh->assetName, meshIndex++);
                const auto exportDepotPath = BuildAssetDepotPath(importer.queryResourcePath().view(), prefabManifest->m_meshImportPath, meshFileName);
                mesh->exportPath = base::res::ResourcePath(exportDepotPath);
            }

            // import mesh
            {
                const auto meshImportConfig = base::RefNew<OBJMeshImportConfig>();
                meshImportConfig->groupFilter = mesh->sourceGroup.name;
                meshImportConfig->markPropertyOverride("groupFilter"_id);

                if (prefabManifest->forceTriangles != meshImportConfig->forceTriangles)
                {
                    meshImportConfig->forceTriangles = prefabManifest->forceTriangles;
                    meshImportConfig->markPropertyOverride("forceTriangles"_id);
                }

                //--

                if (meshImportConfig->units != prefabManifest->units)
                {
                    meshImportConfig->units = prefabManifest->units;
                    meshImportConfig->markPropertyOverride("units"_id);
                }

                if (meshImportConfig->space != prefabManifest->space)
                {
                    meshImportConfig->space = prefabManifest->space;
                    meshImportConfig->markPropertyOverride("space"_id);
                }

                meshImportConfig->globalTranslation = -mesh->exportTranslation;
                meshImportConfig->globalRotation = -mesh->exportRotation;
                meshImportConfig->markPropertyOverride("globalTranslation"_id);
                meshImportConfig->markPropertyOverride("globalRotation"_id);

                if (meshImportConfig->globalScale != prefabManifest->globalScale)
                {
                    meshImportConfig->globalScale = prefabManifest->globalScale;
                    meshImportConfig->markPropertyOverride("globalScale"_id);
                }

                if (prefabManifest->flipFaces != meshImportConfig->flipFaces)
                {
                    meshImportConfig->flipFaces = prefabManifest->flipFaces;
                    meshImportConfig->markPropertyOverride("flipFaces"_id);
                }

                if (meshImportConfig->m_materialImportMode != prefabManifest->m_materialImportMode)
                {
                    meshImportConfig->m_materialImportMode = prefabManifest->m_materialImportMode;
                    meshImportConfig->markPropertyOverride("materialImportMode"_id);
                }

                bool pathValid = base::ApplyRelativePath(importer.queryResourcePath(), prefabManifest->m_materialSearchPath, meshImportConfig->m_materialSearchPath);
                pathValid &= base::ApplyRelativePath(importer.queryResourcePath(), prefabManifest->m_materialImportPath, meshImportConfig->m_materialImportPath);
                meshImportConfig->markPropertyOverride("materialImportPath"_id);
                meshImportConfig->markPropertyOverride("materialSearchPath"_id);

                if (meshImportConfig->m_textureImportMode != prefabManifest->m_textureImportMode)
                {
                    meshImportConfig->m_textureImportMode = prefabManifest->m_textureImportMode;
                    meshImportConfig->markPropertyOverride("textureImportMode"_id);
                }

                pathValid &= base::ApplyRelativePath(importer.queryResourcePath(), prefabManifest->m_textureSearchPath, meshImportConfig->m_textureSearchPath);
                pathValid &= base::ApplyRelativePath(importer.queryResourcePath(), prefabManifest->m_textureImportPath, meshImportConfig->m_textureImportPath);
                meshImportConfig->markPropertyOverride("textureImportPath"_id);
                meshImportConfig->markPropertyOverride("textureSearchPath"_id);

                if (meshImportConfig->m_depotSearchDepth != prefabManifest->m_depotSearchDepth)
                {
                    meshImportConfig->m_depotSearchDepth = prefabManifest->m_depotSearchDepth;
                    meshImportConfig->markPropertyOverride("depotSearchDepth"_id);

                }

                if (meshImportConfig->m_sourceAssetsSearchDepth != prefabManifest->m_sourceAssetsSearchDepth)
                {
                    meshImportConfig->m_sourceAssetsSearchDepth = prefabManifest->m_sourceAssetsSearchDepth;
                    meshImportConfig->markPropertyOverride("sourceAssetsSearchDepth"_id);
                }

                // add follow-up
                importer.followupImport(importer.queryImportPath(), mesh->exportPath.path().view(), meshImportConfig);
            }
        }

        //--

        static const auto meshEntityClass = RTTI::GetInstance().findClass("rendering::StaticMeshEntity"_id);

        // build prefab nodes
        auto root = RefNew<base::world::NodeTemplate>();
        root->m_name = "default"_id;
        for (const auto& obj : prefabSourceObjects)
        {
            auto node = base::RefNew<base::world::NodeTemplate>();
            node->m_name = base::StringID(obj.fullName.view());

            {
                node->m_entityTemplate = base::RefNew<base::ObjectIndirectTemplate>();
                node->m_entityTemplate->templateClass(meshEntityClass);
                node->m_entityTemplate->writeProperty("mesh"_id, obj.mesh->exportPath);
                node->m_entityTemplate->parent(node);

                auto objectToWorld = obj.objectToMesh * obj.mesh->meshToWorld;
                auto objectToWorldTransform = objectToWorld.toTransform();
                node->m_entityTemplate->placement(objectToWorldTransform.toEulerTransform());
            }

            root->m_children.pushBack(node);
            node->parent(root);
        }

        ///--

        // build final prefab
        auto ret = base::RefNew<base::world::Prefab>();
        ret->setup(root);
        return ret;
    }
    
    //--

} // mesh


