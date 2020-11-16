/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#pragma once

#include "base/resource/include/resourceMountPoint.h"
#include "base/resource/include/resource.h"

namespace base
{
    namespace res
    {
        //--

        /// seed file is basically a list of resource references
        typedef Array<AsyncRef<IResource>> TSeedFileList;

        /// cooking seed file - adds a roots to the cooking dependency tree (basically tells cooker - those files and all files needed by them MUST be cooked)
        class BASE_RESOURCE_COMPILER_API SeedFile : public IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SeedFile, IResource);

        public:
            SeedFile();
            SeedFile(TSeedFileList&& files);

            INLINE const TSeedFileList& files() const { return m_files; }

        private:
            TSeedFileList m_files;
        };

        //--

    } // res
} // base