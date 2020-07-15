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
    extern ASSETS_MESH_LOADER_API float ScaleFactorForUnits(MeshImportUnits units);

    // get the "content to engine" space conversion matrix - NOTE, no space unit conversion
    extern ASSETS_MESH_LOADER_API Matrix CalcContentToEngineMatrix(MeshImportSpace space);

    // calculate "content to engine" space conversion matrix that includes the units 
    extern ASSETS_MESH_LOADER_API Matrix CalcContentToEngineMatrix(MeshImportSpace space, MeshImportUnits units);

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
    class ASSETS_MESH_LOADER_API MeshImportConfig : public res::ResourceConfiguration
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshImportConfig, res::ResourceConfiguration);

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
        Vector3 globalTranslation = Vector3(0,0,0);

        // global rotation to apply to mesh when importing
        // NOTE: applied AFTER the normal space transform
        Angles globalRotation = Angles(0,0,0);

        // global scale to apply to mesh when importing
        // NOTE: applied AFTER the normal space transform
        // NOTE: uniform scale only
        float globalScale = 1.0f;

        //--

        // flip the face winding globally
        bool flipFaces = false;

        //--

        // material import
        bool m_autoImportMaterials = true;
        base::Array<base::StringBuf> m_materialSearchPath;
        base::StringBuf m_materialsImportPath;

        // texture import
        bool m_autoImportTextures = true;
        base::Array<base::StringBuf> m_textureSearchPath;
        base::StringBuf m_textureImportPath;

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
        Matrix calcAssetToEngineConversionMatrix(MeshImportUnits defaultAssetUnits, MeshImportSpace defaultAssetSpace) const;

        //--

        INLINE MeshImportUnits resolveUnits(MeshImportUnits defaultAssetUnits) const
        {
            return (units == MeshImportUnits::Auto) ? defaultAssetUnits : units;
        }

        INLINE MeshImportSpace resolveSpace(MeshImportSpace defaultAssetSpace) const
        {
            return (space == MeshImportSpace::Auto) ? defaultAssetSpace : space;
        }
    };

    //---

} // base
