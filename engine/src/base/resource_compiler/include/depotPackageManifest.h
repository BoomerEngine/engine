/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot\meta #]
***/

#pragma once

#include "base/resource/include/resource.h"

namespace base
{
    namespace depot
    {
        //---

        // script package reference
        struct BASE_RESOURCE_COMPILER_API PackageManifestScriptsReference
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(PackageManifestScriptsReference);

        public:
            StringBuf name;
            StringBuf path;
            StringBuf fullPath;
            int priority = 0;
        };

        //---

        // dependency of the file system
        struct BASE_RESOURCE_COMPILER_API PackageManifestDependency
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(PackageManifestDependency);

        public:
            StringBuf prefixPath;
            RefPtr<IFileSystemProvider> provider;
        };


        //---

        // manifest for the depot's package
        class BASE_RESOURCE_COMPILER_API PackageManifest : public res::ITextResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(PackageManifest, res::ITextResource);

        public:
            PackageManifest();

            INLINE const StringBuf& globalID() const { return m_globalID; }

            INLINE const StringBuf& version() const { return m_version; }

            INLINE const StringBuf& description() const { return m_description; }

            // get dependencies
            INLINE const Array<PackageManifestDependency>& dependencies() const { return m_dependencies; }

            // get list of script packages
            // TODO: remove
            INLINE const Array<PackageManifestScriptsReference>& scriptPackages() const { return m_scriptPackages; }

            //--

            // translate paths to the FS mount point
            void translatePaths(const res::ResourceMountPoint& fs);

        private:
            StringBuf m_globalID;
            StringBuf m_version;
            StringBuf m_description;

            Array<PackageManifestDependency> m_dependencies;
            Array<PackageManifestScriptsReference> m_scriptPackages;
        };

        //---

    } // depot
} // base
