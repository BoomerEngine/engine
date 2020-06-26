/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

#include "base/resources/include/resource.h"

namespace base
{
    namespace mesh
    {

        class MeshMaterialBuilder;

        //---

        // units of the import
        enum class MeshImportUnits : uint8_t
        {
            // auto matic  determination based on the type of the asset
            Auto,

            // the 1.0 in the imported geometry represents a meter
            Meters,

            // the 1.0 in the imported geometry represents an imperial inch
            Inches,

            // centimeters
            Centimeters, 

            // the 1.0 in the imported geometry represents a mm
            Milimeter,
        };

        // space of the import
        enum class MeshImportSpace : uint8_t
        {
            // auto matic  determination based on the type of the asset
            Auto,

            // right handed with Z up (Max, Unreal, Unity, every other sane product)
            RightHandZUp,

            // right handed with Y up (Maya, most of the free models)
            RightHandYUp,

            // left handed with Z up
            LeftHandZUp,

            // left handed with Y up
            LeftHandYUp
        };

        //---

        // get scale factor for given units
        extern BASE_GEOMETRY_API float ScaleFactorForUnits(MeshImportUnits units);

        // get the "content to engine" space conversion matrix - NOTE, no space unit conversion
        extern BASE_GEOMETRY_API base::Matrix CalcContentToEngineMatrix(MeshImportSpace space);

        // calculate "content to engine" space conversion matrix that includes the units 
        extern BASE_GEOMETRY_API base::Matrix CalcContentToEngineMatrix(MeshImportSpace space, MeshImportUnits units);

        //---

        /// common manifest for assets importable into mesh formats
        /// contains coordinate system conversion and other setup for geometry importing from outside sources
        class BASE_GEOMETRY_API MeshManifest : public base::res::IResourceManifest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(MeshManifest, base::res::IResourceManifest);

        public:
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
        };

        //---

    } // mesh
} // base
