/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/resources/include/resourceMountPoint.h"
#include "base/resources/include/resource.h"

namespace base
{
    namespace cooker
    {
        //--

        /// entry in the seed files - note: we use relative paths
        struct BASE_COOKING_API SeedFileEntry
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(SeedFileEntry);

            StringBuf relativePath;
            SpecificClassType<res::IResource> resourceClass;
        };

        //--

        /// cooking seed file - adds a roots to the cooking dependency tree (basically tells cooker - those files and all files needed by them MUST be cooked)
        class BASE_COOKING_API SeedFile : public res::ITextResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SeedFile, res::ITextResource);

        public:
            SeedFile();

            //--

            // list of files to cook
            INLINE const Array<SeedFileEntry>& files() const { return m_files; }

            //--

            // set files
            void files(const Array<SeedFileEntry>& files);

            //--

        private:
            Array<SeedFileEntry> m_files;
        };

        //--

    } // cooker
} // base