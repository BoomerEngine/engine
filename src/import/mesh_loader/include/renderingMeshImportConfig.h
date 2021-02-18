/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import\config #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/resource/include/resourceMetadata.h"
#include "renderingMaterialImportConfig.h"
#include "base/resource_compiler/include/importInterface.h"

namespace rendering
{

    //---

    // units of the import
    enum class MeshImportUnits : uint8_t
    {
        Auto, // automatic - determination based on the type of the asset
        Meters, // the 1.0 in the imported geometry represents a meter
        Inches, // the 1.0 in the imported geometry represents an imperial inch
        Centimeters, // the 1.0 in the imported geometry represents a centimeter in real world
        Milimeter, // the 1.0 in the imported geometry represents a mm
    };

    // space of the import
    enum class MeshImportSpace : uint8_t
    {
        Auto, // automatic - determination based on the type of the asset
        RightHandZUp, // assume asset is in a Z-up RHS
        RightHandYUp, // assume asset is in a Y-up RHS
        LeftHandZUp, // assume asset is in a Z-up LHS
        LeftHandYUp, // assume asset is in a Y-up LHS
    };

    //---

    // get scale factor for given units
    extern IMPORT_MESH_LOADER_API float ScaleFactorForUnits(MeshImportUnits units);

    // get the "content to engine" space conversion matrix - NOTE, no space unit conversion
    extern IMPORT_MESH_LOADER_API base::Matrix CalcContentToEngineMatrix(MeshImportSpace space);

    // calculate "content to engine" space conversion matrix that includes the units 
    extern IMPORT_MESH_LOADER_API base::Matrix CalcContentToEngineMatrix(MeshImportSpace space, MeshImportUnits units);

    //---
          
    // tangent space recalculation mode
    enum class MeshDataRecalculationMode : uint8_t
    {
        Never, // never recalculate, leave vectors as-is or empty (0,0,0)
        WhenMissing, // when tangent space data stream is missing recalculate it
        Always, // always recalculate the tangent space vectors, even if there were provided in data
        Remove, // remove any existing data
    };

    //--

    // normal vector computation mode
    enum class MeshNormalComputationMode : uint8_t
    {
        Flat, // compute normals from directly from faces, not recommended
        FaceUniform, // incident faces have uniform influence on the vertex normal
        FaceArea, // incident face normals are averaged by area
    };

    //---

    /// common manifest for assets importable into mesh formats
    /// contains coordinate system conversion and other setup for geometry importing from outside sources
    class IMPORT_MESH_LOADER_API MeshImportConfig : public base::res::ResourceConfiguration
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshImportConfig, base::res::ResourceConfiguration);

    public:
        MeshImportConfig();

        //--

        // units of the imported data
        MeshImportUnits units = MeshImportUnits::Auto;

        // space of the imported data
        MeshImportSpace space = MeshImportSpace::Auto;

        //--

        // global translation to apply to mesh when importing
        // NOTE: applied AFTER the normal space transform
        base::Vector3 globalTranslation = base::Vector3(0,0,0);

        // global rotation to apply to mesh when importing
        // NOTE: applied AFTER the normal space transform
        base::Angles globalRotation = base::Angles(0,0,0);

        // global scale to apply to mesh when importing
        // NOTE: applied AFTER the normal space transform
        // NOTE: uniform scale only
        float globalScale = 1.0f;

        //--

        // flip the face winding globally
        bool flipFaces = false;

        //--

        // material import
        bool m_importMaterials = true; // import materials
        base::StringBuf m_materialSearchPath;
        base::StringBuf m_materialImportPath;

        // texture import
        bool m_importTextures = true; // import textures
        base::StringBuf m_textureSearchPath;
        base::StringBuf m_textureImportPath;

        //--

        // search depth when looking file up in depot
        int m_depotSearchDepth = 6;

        // search depth when looking file up in source asset repository
        int m_sourceAssetsSearchDepth = 6;


        //--

        MeshDataRecalculationMode m_normalRecalculation = MeshDataRecalculationMode::WhenMissing;
        MeshNormalComputationMode m_normalComputationMode = MeshNormalComputationMode::FaceArea;
        float m_normalAngularThreshold = 45.0f;
        bool m_useFaceSmoothGroups = true;
        bool m_flipNormals = false;

        //--

        MeshDataRecalculationMode m_tangentsRecalculation = MeshDataRecalculationMode::WhenMissing;
        float m_tangentsAngularThreshold = 45.0f;
        bool m_flipTangent = false;
        bool m_flipBitangent = false;

        //--

        // calculate the space conversion matrix for given content type
        // NOTE: includes custom transformation specified in the manifest itself
        base::Matrix calcAssetToEngineConversionMatrix(MeshImportUnits defaultAssetUnits, MeshImportSpace defaultAssetSpace) const;

        //--

        INLINE MeshImportUnits resolveUnits(MeshImportUnits defaultAssetUnits) const
        {
            return (units == MeshImportUnits::Auto) ? defaultAssetUnits : units;
        }

        INLINE MeshImportSpace resolveSpace(MeshImportSpace defaultAssetSpace) const
        {
            return (space == MeshImportSpace::Auto) ? defaultAssetSpace : space;
        }

        //--

        virtual void computeConfigurationKey(base::CRC64& crc) const override;
    };

    //---

    /// general mesh importer, contains common functions
    class IMPORT_MESH_LOADER_API IGeneralMeshImporter : public base::res::IResourceImporter
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IGeneralMeshImporter, base::res::IResourceImporter);

    public:
        virtual ~IGeneralMeshImporter();

    protected:
        static void EmitDepotPath(const base::Array<base::StringView>& pathParts, base::IFormatStream& f);
        static void GlueDepotPath(base::StringView path, bool isFileName, base::Array<base::StringView>& outPathParts);

        static base::StringBuf BuildMaterialFileName(base::StringView name, uint32_t materialIndex);
        static base::StringBuf BuildAssetDepotPath(base::StringView referenceDepotPath, base::StringView materialImportPath, base::StringView materialFileName);

        virtual base::RefPtr<MaterialImportConfig> createMaterialImportConfig(const MeshImportConfig& cfg, base::StringView name) const = 0;

        virtual MaterialRef buildSingleMaterialRef(base::res::IResourceImporterInterface& importer, const MeshImportConfig& cfg, base::StringView name, base::StringView materialLibraryName, uint32_t materialIndex) const;

        virtual MaterialInstancePtr buildSingleMaterial(base::res::IResourceImporterInterface& importer, const MeshImportConfig& cfg, base::StringView name, base::StringView materialLibraryName, uint32_t materialIndex, const Mesh* existingMesh) const;
    };

    //---

} // rendering
