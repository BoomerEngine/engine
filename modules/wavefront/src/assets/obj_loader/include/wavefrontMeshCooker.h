/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

#include "base/geometry/include/meshManifest.h"

namespace wavefront
{
    //--

    /// how to convert group/objects into Mesh object/instances
    enum class MeshGrouppingMethod : uint8_t
    {
        Model, // the whole model is considered "one mesh", fastest for processing looses all context data
        Object, // separate Wavefront "o Object01" entries are considered to be meshes
        Group, // separate Wavefront "g Part01" entries are considered to be meshes, creates the most amount of separate meshes
    };

    /// attribute filter - should we output given attribute as mesh data stream or not
    enum class MeshAttributeMode : uint8_t
    {
        Always, // attribute will be always emitted, even if it contains no data (does not make sense but zeros compress well, so..)
        IfPresentAnywhere, // attribute will be emitted to a mesh if at least one input group from .obj contains it
        IfPresentEverywhere, // attribute will be emitted to a mesh ONLY if ALL groups in a build group contain it (to avoid uninitialized data)
        Never, // attribute will never be emitted
    };

    //--

    /// manifest specific for building
    class ASSETS_OBJ_LOADER_API MeshManifest : public base::mesh::MeshManifest
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshManifest, base::mesh::MeshManifest);

    public:
        MeshManifest();

        MeshGrouppingMethod groupingMethod = MeshGrouppingMethod::Group;

        MeshAttributeMode emitNormals = MeshAttributeMode::IfPresentAnywhere;
        MeshAttributeMode emitUVs = MeshAttributeMode::IfPresentAnywhere;
        MeshAttributeMode emitColors = MeshAttributeMode::IfPresentAnywhere;

        bool forceTriangles = false;
        bool allowThreads = true;

        bool flipUV = true;
    };

    //--

    /// mesh cooker for OBJ files
    class ASSETS_OBJ_LOADER_API MeshCooker : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshCooker, base::res::IResourceCooker);

    public:
        MeshCooker();

        virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override final;
        virtual void reportManifestClasses(base::Array<base::SpecificClassType<base::res::IResourceManifest>>& outManifestClasses) const override;
    };

    //--

} // wavefront
