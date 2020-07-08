/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: cooking #]
***/

#include "build.h"
#include "cookerSeedFile.h"

namespace base
{
    namespace res
    {
        
        ///--

        RTTI_BEGIN_TYPE_CLASS(SeedFileEntry);
            RTTI_PROPERTY(relativePath);
            RTTI_PROPERTY(resourceClass);
        RTTI_END_TYPE();

        ///--

        RTTI_BEGIN_TYPE_CLASS(SeedFile);
            RTTI_METADATA(ResourceExtensionMetadata).extension("v4seed");
            RTTI_METADATA(ResourceDescriptionMetadata).description("Seed File");
            RTTI_METADATA(ResourceTagColorMetadata).color(0x20, 0xb0, 0x50);
            RTTI_PROPERTY(m_files);
        RTTI_END_TYPE();

        SeedFile::SeedFile()
        {}

        void SeedFile::files(const Array<SeedFileEntry>& files)
        {
            m_files = files;
            markModified();
        }

        ///--

    } // res
} // base


