/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import\config #]
***/

#include "build.h"
#include "renderingMeshImportConfig.h"
#include "rendering/material/include/renderingMaterial.h"

namespace rendering
{
    //--

    RTTI_BEGIN_TYPE_ENUM(MeshImportUnits);
        RTTI_ENUM_OPTION(Auto);
        RTTI_ENUM_OPTION(Meters);
        RTTI_ENUM_OPTION(Centimeters);
        RTTI_ENUM_OPTION(Milimeter);
        RTTI_ENUM_OPTION(Inches);
        RTTI_END_TYPE();

    //--

    RTTI_BEGIN_TYPE_ENUM(MeshImportSpace);
        RTTI_ENUM_OPTION(Auto);
        RTTI_ENUM_OPTION(RightHandZUp);
        RTTI_ENUM_OPTION(RightHandYUp);
        RTTI_ENUM_OPTION(LeftHandZUp);
        RTTI_ENUM_OPTION(LeftHandYUp);
    RTTI_END_TYPE();

    //--

    RTTI_BEGIN_TYPE_ENUM(MeshDataRecalculationMode);
    RTTI_ENUM_OPTION(Never);
    RTTI_ENUM_OPTION(WhenMissing);
    RTTI_ENUM_OPTION(Always);
    RTTI_ENUM_OPTION(Remove);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(MeshNormalComputationMode);
    RTTI_ENUM_OPTION(Flat);
    RTTI_ENUM_OPTION(FaceUniform);
    RTTI_ENUM_OPTION(FaceArea);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(MeshMaterialImportMode);
        RTTI_ENUM_OPTION_WITH_HINT(DontImport, "Do not import any material data, material entries are still created");
        RTTI_ENUM_OPTION_WITH_HINT(FindOnly, "Only attempt to find materials, do not import anything missing");
        RTTI_ENUM_OPTION_WITH_HINT(EmbedAll, "Import materials and embed them into mesh");
        RTTI_ENUM_OPTION_WITH_HINT(EmbedMissing, "Use materials that are already there but embed everything else");
        RTTI_ENUM_OPTION_WITH_HINT(ImportAll, "Always report material for importing, does not use the search path");
        RTTI_ENUM_OPTION_WITH_HINT(ImportMissing, "Import only the materials that were not found already in depot");
    RTTI_END_TYPE();

    //--

    RTTI_BEGIN_TYPE_CLASS(MeshImportConfig);
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
        RTTI_PROPERTY(m_materialImportMode).editable("Automatically import materials used by this mesh").overriddable();
        RTTI_PROPERTY(m_materialSearchPath).editable("ADDITONAL paths to explore when looking for materials (before we import one)").overriddable();
        RTTI_PROPERTY(m_materialImportPath).editable("Where are the new materials imported").overriddable();
        RTTI_CATEGORY("Texture import");
        RTTI_PROPERTY(m_textureImportMode).editable("Automatically import textures used by this mesh").overriddable();
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
        RTTI_CATEGORY("Base material");
        RTTI_PROPERTY(m_templateDefault).editable().overriddable();
    RTTI_END_TYPE();

    static base::res::StaticResource<rendering::IMaterial> resDefaultMaterialBase("/engine/materials/std_pbr.v4mg");

    MeshImportConfig::MeshImportConfig()
    {
        m_materialImportPath = StringBuf("../materials/");
        m_materialSearchPath = StringBuf("../materials/");

        m_textureImportPath = StringBuf("../textures/");
        m_textureSearchPath = StringBuf("../textures/");

        m_textureImportMode = MaterialTextureImportMode::ImportMissing;
        m_materialImportMode = MeshMaterialImportMode::EmbedMissing;

        m_templateDefault = resDefaultMaterialBase.asyncRef();
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

        crc << (char)m_materialImportMode;
        crc << m_materialSearchPath.view();
        crc << m_materialImportPath.view();

        // texture import
        crc << (char)m_textureImportMode;
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

        crc << m_templateDefault.key().view();
    }

    //--

} // base
