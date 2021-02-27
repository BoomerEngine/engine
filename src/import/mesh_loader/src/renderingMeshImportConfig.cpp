/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import\config #]
***/

#include "build.h"
#include "renderingMeshImportConfig.h"
#include "engine/mesh/include/mesh.h"
#include "engine/material/include/material.h"
#include "engine/material/include/materialInstance.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

RTTI_BEGIN_TYPE_ENUM(MeshImportUnits);
    RTTI_OLD_NAME("rendering::MeshImportUnits");
    RTTI_ENUM_OPTION(Auto);
    RTTI_ENUM_OPTION(Meters);
    RTTI_ENUM_OPTION(Centimeters);
    RTTI_ENUM_OPTION(Milimeter);
    RTTI_ENUM_OPTION(Inches);
    RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ENUM(MeshImportSpace);
    RTTI_OLD_NAME("rendering::MeshImportSpace");
    RTTI_ENUM_OPTION(Auto);
    RTTI_ENUM_OPTION(RightHandZUp);
    RTTI_ENUM_OPTION(RightHandYUp);
    RTTI_ENUM_OPTION(LeftHandZUp);
    RTTI_ENUM_OPTION(LeftHandYUp);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ENUM(MeshDataRecalculationMode);
    RTTI_OLD_NAME("rendering::MeshDataRecalculationMode");
    RTTI_ENUM_OPTION(Never);
    RTTI_ENUM_OPTION(WhenMissing);
    RTTI_ENUM_OPTION(Always);
    RTTI_ENUM_OPTION(Remove);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_ENUM(MeshNormalComputationMode);
    RTTI_OLD_NAME("rendering::MeshNormalComputationMode");
    RTTI_ENUM_OPTION(Flat);
    RTTI_ENUM_OPTION(FaceUniform);
    RTTI_ENUM_OPTION(FaceArea);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(MeshImportConfig);
    RTTI_OLD_NAME("rendering::MeshImportConfig");
    RTTI_CATEGORY("Import space");
    RTTI_PROPERTY(units).editable().overriddable();
    RTTI_PROPERTY(space).editable().overriddable();
    RTTI_PROPERTY(flipFaces).editable().overriddable();
    RTTI_CATEGORY("Transform");
    RTTI_PROPERTY(globalTranslation).editable().overriddable();
    RTTI_PROPERTY(globalRotation).editable().overriddable();
    RTTI_PROPERTY(globalScale).editable().overriddable();
    RTTI_CATEGORY("Asset search");
    RTTI_PROPERTY(m_depotSearchDepth).editable().overriddable();
    RTTI_PROPERTY(m_sourceAssetsSearchDepth).editable().overriddable();
    RTTI_CATEGORY("Material import");
    RTTI_PROPERTY(m_importMaterials).editable("Automatically import materials used by this mesh").overriddable();
    RTTI_PROPERTY(m_materialSearchPath).editable("ADDITONAL paths to explore when looking for materials (before we import one)").overriddable();
    RTTI_PROPERTY(m_materialImportPath).editable("Where are the new materials imported").overriddable();
    RTTI_CATEGORY("Texture import");
    RTTI_PROPERTY(m_importTextures).editable("Automatically import textures used by this mesh").overriddable();
    RTTI_PROPERTY(m_textureSearchPath).editable("ADDITONAL paths to explore when looking for textures (before we import one)").overriddable();
    RTTI_PROPERTY(m_textureImportPath).editable("Where to place imported textures").overriddable();
    RTTI_CATEGORY("Vertex normals");
    RTTI_PROPERTY(m_normalRecalculation).editable("Controls when/if vertex normals are calculated").overriddable();
    RTTI_PROPERTY(m_normalComputationMode).editable("Controls how vertex normals are calculated, specifically how are the normals of incident faces averaged together").overriddable();
    RTTI_PROPERTY(m_normalAngularThreshold).editable("Controls maximum angle between incident faces that still allow for vertex normal weighing").overriddable();
    RTTI_PROPERTY(m_useFaceSmoothGroups).editable("Allow use of the per-face smooth group information if it's available").overriddable();
    RTTI_PROPERTY(m_flipNormals).editable("Flip normal vector, regardless if calculated or not").overriddable();
    RTTI_CATEGORY("Tangent space");
    RTTI_PROPERTY(m_tangentsRecalculation).editable("Tangent space generation mode, controls if tangent space should be generated").overriddable();
    RTTI_PROPERTY(m_tangentsAngularThreshold).editable("Angle threshold for welding tangent space vectors together").overriddable();
    RTTI_PROPERTY(m_flipTangent).editable("Flip tangent vector, regardless if calculated or not").overriddable();
    RTTI_PROPERTY(m_flipBitangent).editable("Flip bitangent vector, regardless if calculated or not").overriddable();
RTTI_END_TYPE();

MeshImportConfig::MeshImportConfig()
{
    m_materialImportPath = StringBuf("../materials/");
    m_materialSearchPath = StringBuf("../materials/");

    m_textureImportPath = StringBuf("../textures/");
    m_textureSearchPath = StringBuf("../textures/");
}

float GetScaleFactorForUnits(MeshImportUnits units)
{
    switch (units)
    {
        case MeshImportUnits::Meters: return 1.0f;
        case MeshImportUnits::Inches: return 0.0254f;
        case MeshImportUnits::Centimeters: return 0.01f;
        case MeshImportUnits::Milimeter: return 0.001f;
    }

    // default scale factor
    return 1.0f;
}

Matrix GetOrientationMatrixForSpace(MeshImportSpace space)
{
    switch (space)
    {
        case MeshImportSpace::RightHandZUp:
            return Matrix::IDENTITY();

        case MeshImportSpace::RightHandYUp:
            return Matrix(1, 0, 0, 0, 0, -1, 0, 1, 0); // -1 to keep the stuff right handed

        case MeshImportSpace::LeftHandZUp:
            return Matrix(1, 0, 0, 0, -1, 0, 0, 0, 1);

        case MeshImportSpace::LeftHandYUp:
            return Matrix(1, 0, 0, 0, 0, 1, 0, 1, 0); // swapping Y and Z causes the space to flip
    }

    // default space
    return Matrix::IDENTITY();
}

Matrix CalcContentToEngineMatrix(MeshImportSpace space, MeshImportUnits units)
{
    auto orientationMatrix = GetOrientationMatrixForSpace(space);
    orientationMatrix.scaleInner(GetScaleFactorForUnits(units));
    return orientationMatrix;
}

Matrix MeshImportConfig::calcAssetToEngineConversionMatrix(MeshImportUnits defaultAssetUnits, MeshImportSpace defaultAssetSpace) const
{
    // select setup
    auto importUnits = resolveUnits(defaultAssetUnits);
    auto importSpace = resolveSpace(defaultAssetSpace);

    // calculate the orientation matrix
    auto orientationMatrix = CalcContentToEngineMatrix(importSpace, importUnits);

    // calculate the additional setup matrix
    auto additionalTransformMatrix = Matrix::BuildTRS(globalTranslation, globalRotation, globalScale);
    return orientationMatrix * additionalTransformMatrix;
}

//--

void MeshImportConfig::computeConfigurationKey(CRC64& crc) const
{
    TBaseClass::computeConfigurationKey(crc);

    crc << (char)units;
    crc << (char)space;
    crc << globalTranslation.x;
    crc << globalTranslation.y;
    crc << globalTranslation.z;
    crc << globalRotation.pitch;
    crc << globalRotation.yaw;
    crc << globalRotation.roll;
    crc << globalScale;
    crc << flipFaces;

    //--

    crc << m_importMaterials;
    crc << m_materialSearchPath.view();
    crc << m_materialImportPath.view();

    // texture import
    crc << m_importTextures;
    crc << m_textureSearchPath.view();
    crc << m_textureImportPath.view();

    crc << m_depotSearchDepth;
    crc << m_sourceAssetsSearchDepth;

    //--

    crc << (char)m_normalRecalculation;
    crc << (char)m_normalComputationMode;
    crc << m_normalAngularThreshold;
    crc << m_useFaceSmoothGroups;
    crc << m_flipNormals;

    //--

    crc << (char)m_tangentsRecalculation;
    crc << m_tangentsAngularThreshold;
    crc << m_flipTangent;
    crc << m_flipBitangent;
}

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGeneralMeshImporter);
RTTI_END_TYPE();

IGeneralMeshImporter::~IGeneralMeshImporter()
{}


StringBuf IGeneralMeshImporter::BuildMaterialFileName(StringView name, uint32_t materialIndex)
{
    static const auto ext = res::IResource::GetResourceExtensionForClass(MaterialInstance::GetStaticClass());

    StringBuf fileName;
    if (!MakeSafeFileName(name, fileName))
        fileName = TempString("Material{}", materialIndex);

    ASSERT(ValidateFileName(fileName));

    return TempString("{}.{}", fileName, ext);
}

void IGeneralMeshImporter::EmitDepotPath(const Array<StringView>& pathParts, IFormatStream& f)
{
    f << "/";

    for (const auto part : pathParts)
    {
        f << part;
        f << "/";
    }
}

void IGeneralMeshImporter::GlueDepotPath(StringView path, bool isFileName, Array<StringView>& outPathParts)
{
    InplaceArray<StringView, 10> pathParts;
    path.slice("/\\", false, pathParts);

    // skip the file name itself
    const auto dirPath = path.endsWith("/") || path.endsWith("\\");
    if (isFileName && (!dirPath || !pathParts.empty()))
        pathParts.popBack();

    // if we are absolute path than clear existing stuff
    const auto absolutePath = path.beginsWith("/") || path.beginsWith("\\");
    if (absolutePath)
        outPathParts.clear();

    // append path parts, respecting the ".." and "."
    for (const auto part : pathParts)
    {
        if (part == ".")
            continue;

        if (part == "..")
        {
            if (!outPathParts.empty())
                outPathParts.popBack();
            continue;
        }

        outPathParts.pushBack(part);
    }
}

StringBuf IGeneralMeshImporter::BuildAssetDepotPath(StringView referenceDepotPath, StringView materialImportPath, StringView materialFileName)
{
    InplaceArray<StringView, 20> pathParts;
    GlueDepotPath(referenceDepotPath, true, pathParts);
    GlueDepotPath(materialImportPath, false, pathParts);

    StringBuilder txt;
    EmitDepotPath(pathParts, txt);

    txt << materialFileName;

    return txt.toString();
}

//--

static StringBuf TransformRelativePath(res::IResourceImporterInterface& importer, StringView secondaryImportPath, StringView searchPath)
{
    if (!searchPath)
        return StringBuf::EMPTY();

    if (!secondaryImportPath)
        secondaryImportPath = importer.queryImportPath();

    secondaryImportPath = secondaryImportPath.baseDirectory();

    StringBuf mergedAbsolutePath;
    if (!ApplyRelativePath(importer.queryResourcePath(), searchPath, mergedAbsolutePath))
        return StringBuf(searchPath); // try our luck with search path directly

    StringBuf relativePath;
    if (!BuildRelativePath(secondaryImportPath, mergedAbsolutePath, relativePath))
        return StringBuf(searchPath); // try our luck with search path directly

    TRACE_INFO("Search path '{}' relative to '{}' translated to '{}' relative to '{}'", searchPath, importer.queryImportPath(), relativePath, secondaryImportPath);
    return relativePath;        
}

MaterialRef IGeneralMeshImporter::buildSingleMaterialRef(res::IResourceImporterInterface& importer, const MeshImportConfig& cfg, StringView name, StringView materialLibraryName, uint32_t materialIndex) const
{
    // don't import
    if (!cfg.m_importMaterials)
        return MaterialRef();

    // find the material file
    const auto materialFileName = BuildMaterialFileName(name, materialIndex);
    if (cfg.m_materialSearchPath)
    {
        TRACE_INFO("Looking for material file '{}'...", materialFileName);

        StringBuf materialDepotPath;
        if (importer.findDepotFile(importer.queryResourcePath(), cfg.m_materialSearchPath, materialFileName, materialDepotPath, cfg.m_depotSearchDepth))
        {
            TRACE_INFO("Existing material file found at '{}'", materialDepotPath);
            return MaterialRef(res::ResourcePath(materialDepotPath));
        }
    }

    // import
    if (cfg.m_materialImportPath)
    {
        // check if the imported material already exists
        const auto depotPath = BuildAssetDepotPath(importer.queryResourcePath().view(), cfg.m_materialImportPath, materialFileName);

        const auto materialImportConfig = createMaterialImportConfig(cfg, name);// RefNew<MTLMaterialImportConfig>();
//                materialImportConfig->m_materialName = StringBuf(name);
        materialImportConfig->m_importTextures = cfg.m_importTextures;
        materialImportConfig->m_depotSearchDepth = cfg.m_depotSearchDepth;
        materialImportConfig->m_sourceAssetsSearchDepth = cfg.m_sourceAssetsSearchDepth;

        // make sure properties are propagated
        materialImportConfig->markPropertyOverride("textureImportMode"_id);
        materialImportConfig->markPropertyOverride("depotSearchDepth"_id);
        materialImportConfig->markPropertyOverride("sourceAssetsSearchDepth"_id);
        materialImportConfig->markPropertyOverride("templateDefault"_id);
        materialImportConfig->markPropertyOverride("templateEmissive"_id);
        materialImportConfig->markPropertyOverride("templateMasked"_id);
        materialImportConfig->markPropertyOverride("templateUnlit"_id);

        if (materialLibraryName)
        {
            StringBuf resolvedMaterialLibraryPath;
            if (importer.findSourceFile(importer.queryImportPath(), materialLibraryName, resolvedMaterialLibraryPath))
            {
                // build depot path for the imported texture
                TRACE_INFO("Material '{}' found in library '{}' will be improted as '{}'", name, resolvedMaterialLibraryPath, depotPath);

                // translate paths
                if (cfg.m_textureImportPath)
                {
                    materialImportConfig->m_textureImportPath = TransformRelativePath(importer, resolvedMaterialLibraryPath, cfg.m_textureImportPath);
                    materialImportConfig->markPropertyOverride("textureImportPath"_id);
                }
                if (cfg.m_textureSearchPath)
                {
                    materialImportConfig->m_textureSearchPath = TransformRelativePath(importer, resolvedMaterialLibraryPath, cfg.m_textureSearchPath);
                    materialImportConfig->markPropertyOverride("textureSearchPath"_id);
                }

                // emit the follow-up import, no extra config at the moment
                importer.followupImport(resolvedMaterialLibraryPath, depotPath, materialImportConfig);

                // build a unloaded material reference (so it can be saved)
                return MaterialRef(res::ResourcePath(depotPath));
            }
        }
        else
        {
            // copy search paths
            if (cfg.m_textureImportPath)
            {
                materialImportConfig->m_textureImportPath = cfg.m_textureImportPath;
                materialImportConfig->markPropertyOverride("textureImportPath"_id);
            }
            if (cfg.m_textureSearchPath)
            {
                materialImportConfig->m_textureSearchPath = cfg.m_textureSearchPath;
                materialImportConfig->markPropertyOverride("textureSearchPath"_id);
            }

            // emit the follow-up import, no extra config at the moment
            importer.followupImport(importer.queryImportPath(), depotPath, materialImportConfig);

            // build a unloaded material reference (so it can be saved)
            return MaterialRef(res::ResourcePath(depotPath));
        }
    }

    // no material imported
    return MaterialRef();
}

MaterialInstancePtr IGeneralMeshImporter::buildSingleMaterial(res::IResourceImporterInterface& importer, const MeshImportConfig& cfg, StringView name, StringView materialLibraryName, uint32_t materialIndex, const Mesh* existingMesh) const
{
    Array<MaterialInstanceParam> existingParameters;

    if (existingMesh)
    {
        for (const auto& ptr : existingMesh->materials())
        {
            if (ptr.name == name)
            {
                existingParameters = ptr.material->parameters();
                break;
            }
        }
    }

    const auto baseMaterial = buildSingleMaterialRef(importer, cfg, name, materialLibraryName, materialIndex);
    return RefNew<MaterialInstance>(baseMaterial, std::move(existingParameters));
}

END_BOOMER_NAMESPACE_EX(assets)
