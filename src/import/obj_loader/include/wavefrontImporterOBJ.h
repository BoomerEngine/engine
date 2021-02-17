/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: import #]
***/

#pragma once

#include "base/resource_compiler/include/importInterface.h"
#include "import/mesh_loader/include/renderingMeshImportConfig.h"

namespace wavefront
{
    //--

    /// attribute filter - should we output given attribute as mesh data stream or not
    enum class OBJMeshAttributeMode : uint8_t
    {
        Always, // attribute will be always emitted, even if it contains no data (does not make sense but zeros compress well, so..)
        IfPresentAnywhere, // attribute will be emitted to a mesh if at least one input group from .obj contains it
        IfPresentEverywhere, // attribute will be emitted to a mesh ONLY if ALL groups in a build group contain it (to avoid uninitialized data)
        Never, // attribute will never be emitted
    };

    //--

    /// manifest specific for importing Wavefront meshes
    class IMPORT_OBJ_LOADER_API OBJMeshImportConfig : public rendering::MeshImportConfig
    {
        RTTI_DECLARE_VIRTUAL_CLASS(OBJMeshImportConfig, rendering::MeshImportConfig);

    public:
        OBJMeshImportConfig();

        base::StringBuf objectFilter;
        base::StringBuf groupFilter;

        OBJMeshAttributeMode emitNormals = OBJMeshAttributeMode::IfPresentAnywhere;
        OBJMeshAttributeMode emitUVs = OBJMeshAttributeMode::IfPresentAnywhere;
        OBJMeshAttributeMode emitColors = OBJMeshAttributeMode::IfPresentAnywhere;

        bool forceTriangles = false;
        bool allowThreads = true;

        bool flipUV = true;

        virtual void computeConfigurationKey(base::CRC64& crc) const override;
    };

    //--

    /// mesh cooker for OBJ files
    class IMPORT_OBJ_LOADER_API OBJMeshImporter : public base::res::IResourceImporter
    {
        RTTI_DECLARE_VIRTUAL_CLASS(OBJMeshImporter, base::res::IResourceImporter);

    public:
        OBJMeshImporter();

        virtual base::res::ResourcePtr importResource(base::res::IResourceImporterInterface& importer) const override final;
    };

    //--

} // wavefront
