/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#include "build.h"
#include "meshManifest.h"

namespace base
{
    namespace mesh
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

        RTTI_BEGIN_TYPE_CLASS(MeshManifest);
            RTTI_METADATA(base::res::ResourceExtensionMetadata); // disable direct saving/loading
            RTTI_METADATA(base::res::ResourceManifestExtensionMetadata).extension("mesh.meta");
            RTTI_CATEGORY("Source tool adjustment");
            RTTI_PROPERTY(units).editable();
            RTTI_PROPERTY(space).editable();
            RTTI_CATEGORY("Engine space adjustment");
            RTTI_PROPERTY(globalTranslation).editable();
            RTTI_PROPERTY(globalRotation).editable();
            RTTI_PROPERTY(globalScale).editable();
            RTTI_CATEGORY("Faces");
            RTTI_PROPERTY(flipFaces).editable();
        RTTI_END_TYPE();

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

        base::Matrix GetOrientationMatrixForSpace(MeshImportSpace space)
        {
            switch (space)
            {
                case MeshImportSpace::RightHandZUp:
                    return base::Matrix::IDENTITY();

                case MeshImportSpace::RightHandYUp:
                    return base::Matrix(1, 0, 0, 0, 0, -1, 0, 1, 0); // -1 to keep the stuff right handed

                case MeshImportSpace::LeftHandZUp:
                    return base::Matrix(1, 0, 0, 0, -1, 0, 0, 0, 1);

                case MeshImportSpace::LeftHandYUp:
                    return base::Matrix(1, 0, 0, 0, 0, 1, 0, 1, 0); // swapping Y and Z causes the space to flip
            }

            // default space
            return base::Matrix::IDENTITY();
        }

        base::Matrix CalcContentToEngineMatrix(MeshImportSpace space, MeshImportUnits units)
        {
            auto orientationMatrix = GetOrientationMatrixForSpace(space);
            orientationMatrix.scaleInner(GetScaleFactorForUnits(units));
            return orientationMatrix;
        }

        base::Matrix MeshManifest::calcAssetToEngineConversionMatrix(MeshImportUnits defaultAssetUnits, MeshImportSpace defaultAssetSpace) const
        {
            // select setup
            auto importUnits = resolveUnits(defaultAssetUnits);
            auto importSpace = resolveSpace(defaultAssetSpace);

            // calculate the orientation matrix
            auto orientationMatrix = CalcContentToEngineMatrix(importSpace, importUnits);

            // calculate the additional setup matrix
            auto additionalTransformMatrix = base::Matrix::BuildTRS(globalTranslation, globalRotation, globalScale);
            return orientationMatrix * additionalTransformMatrix;
        }        

        //--

    }// mesh
} // base
