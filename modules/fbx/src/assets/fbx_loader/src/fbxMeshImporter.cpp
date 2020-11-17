/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fbx #]
***/

#include "build.h"

#include "fbxMeshImporter.h"
#include "fbxFileLoaderService.h"
#include "fbxFileData.h"

#include "base/io/include/ioFileHandle.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/containers/include/inplaceArray.h"
#include "base/mesh/include/mesh.h"
#include "rendering/mesh/include/renderingMeshChunkPayload.h"

namespace fbx
{

    //--

    RTTI_BEGIN_TYPE_CLASS(FBXMeshImportConfig);
        //RTTI_METADATA(base::res::ResourceManifestExtensionMetadata).extension("mesh.meta");
        //RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("FBX Import SetupMetadata");
        RTTI_CATEGORY("FBX Tuning");
        RTTI_PROPERTY(m_alignToPivot).editable("Align imported geometry to the pivot of the root exported node");
        RTTI_PROPERTY(m_forceNodeSkin).editable("Skin all meshes to parent nodes");
        RTTI_PROPERTY(m_flipUV).editable("Flip V channel of the UVs");
        RTTI_PROPERTY(m_createNodeMaterials).editable("Create a new material for each node");
        RTTI_CATEGORY("FBX Materials");
        RTTI_PROPERTY(m_baseMaterialTemplate).editable("Base material tempalte to use when importing");
    RTTI_END_TYPE();

    FBXMeshImportConfig::FBXMeshImportConfig()
        : m_alignToPivot(true)
    {
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(MeshImporter);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<base::mesh::Mesh>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("fbx").addSourceExtension("FBX");
        RTTI_METADATA(base::res::ResourceCookerVersionMetadata).version(0);
    RTTI_END_TYPE();

    //--

    base::ConfigProperty<base::StringBuf> cvLitMaterialTemplate("Assets.FBX", "DefaultLitMaterialTemplate", "modules/fbx/materials/default_lit.v4mi");
    base::ConfigProperty<base::StringBuf> cvUnlitMaterialTemplate("Assets.FBX", "DefaultUnlitMaterialTemplate", "modules/fbx/materials/default_unlit.v4mi");

    //--

    // Property 'ShadingModel', type KString
    // Property 'MultiLayer', type bool
    // Property 'EmissiveColor', type Color
    // Property 'EmissiveFactor', type Number
    // Property 'AmbientColor', type Color
    // Property 'AmbientFactor', type Number
    // Property 'DiffuseColor', type Color
    // Property 'DiffuseFactor', type Number
    // Property 'Bump', type Vector
    // Property 'NormalMap', type Vector
    // Property 'BumpFactor', type Number
    // Property 'TransparentColor', type Color
    // Property 'TransparencyFactor', type Number
    // Property 'DisplacementColor', type Color
    // Property 'DisplacementFactor', type Number
    // Property 'VectorDisplacementColor', type Color
    // Property 'VectorDisplacementFactor', type Number
    // Property 'SpecularColor', type Color
    // Property 'SpecularFactor', type Number
    // Property 'ShininessExponent', type Number
    // Property 'ReflectionColor', type Color
    // Property 'ReflectionFactor', type Number
    // Property 'Emissive', type Vector
    // Property 'Ambient', type Vector
    // Property 'Diffuse', type Vector
    // Property 'Specular', type Vector
    // Property 'Shininess', type Number
    // Property 'Opacity', type Number
    // Property 'Reflectivity', type Number

    static bool TryReadTexturePath(const FbxSurfaceMaterial& material, const char* propertyName, base::StringBuf& outPath)
    {
        auto prop = material.FindProperty(propertyName, false);
        if (!prop.IsValid())
            return false;

        auto numTextures = prop.GetSrcObjectCount<FbxTexture>();
        if (0 == numTextures)
            return false;

        auto lTex = prop.GetSrcObject<FbxFileTexture>(0);
        if (!lTex)
            return false;

        auto uOffset = lTex->GetTranslationU();
        auto vOffset = lTex->GetTranslationV();
        auto uScale = lTex->GetScaleU();
        auto vScale = lTex->GetScaleV();
        TRACE_INFO("UV setup for '{}': Offset: [{},{}], Scale: [{},{}]", lTex->GetFileName(), uOffset, vOffset, uScale, vScale);

        outPath = lTex->GetFileName();
        return true;
    }

    static bool IsValidMaterialNameChar(char ch)
    {
        if (ch >= 'A' && ch <= 'Z') return true;
        if (ch >= 'a' && ch <= 'z') return true;
        if (ch >= '0' && ch <= '9') return true;
        if (ch == '.' || ch == '_' || ch == '(' || ch == ')') return true;
        return false;
    }
    
    static base::StringID ConformMaterialName(base::StringView name)
    {
        base::StringBuilder txt;

        for (auto ch : name)
        {
            if (!IsValidMaterialNameChar(ch))
                ch = '_';
            txt.appendch(ch);
        }

        return base::StringID(txt.c_str());

    }

    /*static void ExportMaterial(base::StringView name, const FbxSurfaceMaterial* material, base::mesh::MeshMaterial& outMaterial)
    {
        // we are always FBX material
        outMaterial.name = ConformMaterialName(name);
        outMaterial.materialClass = "FBXMaterial"_id;

        // suck source data
        if (material)
        {
            // get shading model
            auto shadingModel = material->ShadingModel.Get();
            TRACE_INFO("Material '{}' uses shading model '{}'", outMaterial.name, shadingModel.Buffer());
            if (shadingModel != "Unlit" || shadingModel != "unlit" || shadingModel != "flat")
                outMaterial.flags |= base::mesh::MeshMaterialFlagBit::Unlit;
            else
                outMaterial.flags -= base::mesh::MeshMaterialFlagBit::Unlit;

            // todo: detect transparencies

            // load diffuse map
            base::StringBuf diffusePath;
            //if (!TryBindTexture(cooker, *materialToImport, "DiffuseColor", "BaseColorMap;DiffuseMap", *materialParams, &diffusePath))
                //  TryBindTexture(cooker, *materialToImport, "Diffuse", "BaseColorMap;DiffuseMap", *materialParams, &diffusePath);

            // load normal map
            //if (!TryBindTexture(cooker, *materialToImport, "NormalMap", "NormalMap", *materialParams))
                //  TryBindTexture(cooker, *materialToImport, "Bump", "NormalMap", *materialParams);

            // process all properties
            for (auto prop = material->GetFirstProperty(); prop.IsValid(); prop = material->GetNextProperty(prop))
            {
                base::StringBuf path;
                if (TryReadTexturePath(*material, prop.GetName(), path))
                {
                    TRACE_INFO("Property '{}', type {} = '{}'", prop.GetNameAsCStr(), prop.GetPropertyDataType().GetName(), path);
                }
                else
                {
                    TRACE_INFO("Property '{}', type {}", prop.GetNameAsCStr(), prop.GetPropertyDataType().GetName());
                }

                if (!path.empty())
                {
                    const auto propName = base::StringID(prop.GetNameAsCStr());
                    outMaterial.parameters.setValue(propName, path);
                }
            }
        }
    }

    static void BuildMaterialData(const LoadedFile& sourceGeometry, const MaterialMapper& materials, base::Array<base::mesh::MeshMaterial>& outMaterials)
    {
        for (auto material : materials.materials)
        {
            auto& outMaterial = outMaterials.emplaceBack();
            ExportMaterial(material.name, material.sourceData, outMaterial);
        }
    }*/

    //--

    static bool BuildModels(base::IProgressTracker& progress, const LoadedFile& sourceGeometry, base::Array<DataNodeMesh>& outModels, SkeletonBuilder& outSkeleton, MaterialMapper& outMaterials, const base::Matrix& assetToWorld, bool forceSkinToNode, bool flipFaces, bool flipUV)
    {
        const auto& nodes = sourceGeometry.nodes();

        for (uint32_t i=0; i<nodes.size(); ++i)
        {
            const auto& node = nodes[i];

            if (node->m_type == DataNodeType::VisualMesh)
            {
                // update status, also support cancellation
                progress.reportProgress(i, nodes.size(), base::TempString("Processing node '{}'", node->m_name));
                if (progress.checkCancelation())
                    return false;

                // export node
                auto& outModel = outModels.emplaceBack();
                // TODO: parse tags
                outModel.name = node->m_name;
                node->exportToMeshModel(progress, sourceGeometry, assetToWorld, outModel, outSkeleton, outMaterials, forceSkinToNode, flipUV, flipFaces);
            }
        }

        return true;
    }

    //--

    MeshImporter::MeshImporter()
    {}

    base::res::ResourcePtr MeshImporter::importResource(base::res::IResourceImporterInterface& importer) const
    {
        // load the FBX data
        auto importedScene = importer.loadSourceAsset<fbx::LoadedFile>(importer.queryImportPath());
        if (!importedScene)
        {
            TRACE_ERROR("Failed to load scene from import file");
            return false;
        }

        // get the configuration
        auto importConfig = importer.queryConfigration<FBXMeshImportConfig>();

        // create output mesh
        auto existingMesh = importer.existingData<base::mesh::Mesh>();
        auto ret = base::CreateSharedPtr<base::mesh::Mesh>(existingMesh);

        //---

        // extract geometry
        bool extractGeometry = true;
        bool extractSkeleton = true;
        if (extractGeometry || extractSkeleton)
        {
            // build the scaling matrix
            auto fileToWorld = importConfig->calcAssetToEngineConversionMatrix(base::mesh::MeshImportUnits::Meters, base::mesh::MeshImportSpace::RightHandZUp);

            // extract the unpacked meshes from the FBX
            SkeletonBuilder skeleton;
            MaterialMapper materials;
            base::Array<DataNodeMesh> meshes;
            BuildModels(importer, *importedScene, meshes, skeleton, materials, fileToWorld, importConfig->m_forceNodeSkin, importConfig->flipFaces, importConfig->m_flipUV);

            /

        //--

        // create default LODs
        // TODO: import FbxLODGroup
        // TODO: use the config to fill in the LODs is already done
        base::Array<rendering::MeshDetailLevel> detailLevel;
        {
            auto& defaultRange = detailLevel.emplaceBack();
            defaultRange.rangeMin = 0.0f;
            defaultRange.rangeMax = 100.0f;
        }

        //--

        // extract custom collision shapes
        //base::Array<base::mesh::MeshCollisionShape> meshCollision;

        //--

        // calculate bounds of the data
        base::Box meshLocalBounds;
        for (const auto& model : meshes)
            for (const auto& chunk : model.chunks)
                meshLocalBounds.merge(chunk.bounds);

        //---


        // attach payloads
        ret->attachPayload()
    }

} // fbx

