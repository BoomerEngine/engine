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

    /// manifest specific for importing Wavefront meshes
    class IMPORT_OBJ_LOADER_API OBJPrefabImportConfig : public rendering::MeshImportConfig
    {
        RTTI_DECLARE_VIRTUAL_CLASS(OBJPrefabImportConfig, rendering::MeshImportConfig);

    public:
        OBJPrefabImportConfig();

        base::StringBuf m_meshImportPath;

        bool forceTriangles = false;
        bool flipUV = true;

        bool reuseIdenticalMeshes = true;
        bool unrotateMeshes = true;

        char positionAlignX = 0; // center X
        char positionAlignY = 0; // center Y
        char positionAlignZ = -1; // bottom

        virtual void computeConfigurationKey(base::CRC64& crc) const override;
    };

    //--

    /// prefab cooker for OBJ files
    class IMPORT_OBJ_LOADER_API OBJPrefabImporter : public base::res::IResourceImporter
    {
        RTTI_DECLARE_VIRTUAL_CLASS(OBJPrefabImporter, base::res::IResourceImporter);

    public:
        OBJPrefabImporter();

        virtual base::res::ResourcePtr importResource(base::res::IResourceImporterInterface& importer) const override final;
    };

    //--

} // wavefront
