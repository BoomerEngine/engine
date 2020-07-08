/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooker #]
***/

#pragma once
#include "base/resource/include/resource.h"

namespace rendering
{
    //--

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

    //--

    /// manifest with settings to pack rendering mesh
    class ASSETS_MESH_LOADER_API MeshPackingManifest : public base::res::IResourceManifest
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshPackingManifest, base::res::IResourceManifest);

    public:
        MeshPackingManifest();

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
    };

    //--

} // rendering