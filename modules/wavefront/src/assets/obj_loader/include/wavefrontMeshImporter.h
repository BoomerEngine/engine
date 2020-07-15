/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: mesh #]
***/

#pragma once

#include "base/resource_compiler/include/importInterface.h"
#include "assets/mesh_loader/include/renderingMeshImportConfig.h"

namespace wavefront
{
    //--

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
    class ASSETS_OBJ_LOADER_API MeshImportConfig : public rendering::MeshImportConfig
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshImportConfig, rendering::MeshImportConfig);

    public:
        MeshImportConfig();

        MeshAttributeMode emitNormals = MeshAttributeMode::IfPresentAnywhere;
        MeshAttributeMode emitUVs = MeshAttributeMode::IfPresentAnywhere;
        MeshAttributeMode emitColors = MeshAttributeMode::IfPresentAnywhere;

        bool forceTriangles = false;
        bool allowThreads = true;

        bool flipUV = true;
    };

    //--

    /// mesh cooker for OBJ files
    class ASSETS_OBJ_LOADER_API MeshImporter : public base::res::IResourceImporter
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshImporter, base::res::IResourceImporter);

    public:
        MeshImporter();

        virtual base::res::ResourcePtr importResource(base::res::IResourceImporterInterface& importer) const override final;
    };

    //--

} // wavefront
