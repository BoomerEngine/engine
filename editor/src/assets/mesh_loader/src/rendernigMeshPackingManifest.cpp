/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooker #]
***/

#include "build.h"
#include "rendernigMeshPackingManifest.h"

namespace rendering
{
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

    RTTI_BEGIN_TYPE_CLASS(MeshPackingManifest);
        RTTI_METADATA(base::res::ResourceManifestExtensionMetadata).extension("packing.meta");
        RTTI_CATEGORY("Vertex normals");
        RTTI_PROPERTY(m_normalRecalculation).editable("Controls when/if vertex normals are calculated");
        RTTI_PROPERTY(m_normalComputationMode).editable("Controls how vertex normals are calculated, specifically how are the normals of incident faces averaged together");
        RTTI_PROPERTY(m_normalAngularThreshold).editable("Controls maximum angle between incident faces that still allow for vertex normal weighing");
        RTTI_PROPERTY(m_useFaceSmoothGroups).editable("Allow use of the per-face smooth group information if it's available");
        RTTI_PROPERTY(m_flipNormals).editable("Flip normal vector, regardless if calculated or not");
        RTTI_CATEGORY("Tangent space");
        RTTI_PROPERTY(m_tangentsRecalculation).editable("Tangent space generation mode, controls if tangent space should be generated");
        RTTI_PROPERTY(m_tangentsAngularThreshold).editable("Angle threshold for welding tangent space vectors together");
        RTTI_PROPERTY(m_flipTangent).editable("Flip tangent vector, regardless if calculated or not");
        RTTI_PROPERTY(m_flipBitangent).editable("Flip bitangent vector, regardless if calculated or not");
    RTTI_END_TYPE();

    MeshPackingManifest::MeshPackingManifest()
    {}

    //--
    
} // rendering
