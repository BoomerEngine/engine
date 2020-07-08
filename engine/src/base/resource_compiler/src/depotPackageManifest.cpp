/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot\meta #]
***/

#include "build.h"
#include "depotFileSystem.h"
#include "depotPackageManifest.h"

namespace base
{
    namespace depot
    {

        //---

        RTTI_BEGIN_TYPE_STRUCT(PackageManifestScriptsReference);
            RTTI_PROPERTY(name).editable();
            RTTI_PROPERTY(path).editable();
            RTTI_PROPERTY(priority).editable();
        RTTI_END_TYPE();

        //---

        RTTI_BEGIN_TYPE_STRUCT(PackageManifestDependency);
            RTTI_PROPERTY(prefixPath).editable();
            RTTI_PROPERTY(provider).editable();
        RTTI_END_TYPE();

        //---

        RTTI_BEGIN_TYPE_CLASS(PackageManifest);
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("manifest");
            RTTI_PROPERTY(m_globalID).editable();
            RTTI_PROPERTY(m_version).editable();
            RTTI_PROPERTY(m_description).editable();
            RTTI_PROPERTY(m_dependencies).editable();
            RTTI_PROPERTY(m_scriptPackages).editable();
        RTTI_END_TYPE();

        PackageManifest::PackageManifest()
        {}

        void PackageManifest::translatePaths(const res::ResourceMountPoint& fs)
        {
            for (auto& entry : m_scriptPackages)
                fs.expandPathFromRelative(entry.path, entry.fullPath);
        }

        //---

    } // depot
} // service