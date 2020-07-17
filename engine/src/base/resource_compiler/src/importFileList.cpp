/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "importFileList.h"
#include "base/resource/include/resourceTags.h"
#include "base/resource/include/resourceMetadata.h"

namespace base
{
    namespace res
    {

        //--

        RTTI_BEGIN_TYPE_CLASS(ImportFileEntry);
            RTTI_PROPERTY(depotPath);
            RTTI_PROPERTY(assetPath);
            RTTI_PROPERTY(userConfiguration);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(ImportList);
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4imports");
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Import List");
            RTTI_METADATA(base::res::ResourceTagColorMetadata).color(0xe2, 0xd6, 0xa8);
            RTTI_PROPERTY(m_files);
        RTTI_END_TYPE();

        ImportList::ImportList()
        {}

        ImportList::ImportList(Array<ImportFileEntry>&& files)
            : m_files(std::move(files))
        {
            for (auto& entry : m_files)
            {
                if (entry.userConfiguration)
                {
                    DEBUG_CHECK_EX(!entry.userConfiguration->parent(), "Object should not be parented");
                    entry.userConfiguration->parent(this);
                }
            }
        }

        //--

        //--

    } // res
} // base
