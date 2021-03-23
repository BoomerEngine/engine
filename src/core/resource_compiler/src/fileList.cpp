/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#include "build.h"
#include "fileList.h"

#include "core/resource/include/tags.h"
#include "core/resource/include/metadata.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(ImportFileEntry);
    RTTI_PROPERTY(depotPath);
    RTTI_PROPERTY(assetPath);
    RTTI_PROPERTY(userConfiguration);
    RTTI_PROPERTY(id);
    RTTI_PROPERTY(resourceClass);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(ImportList);
    RTTI_METADATA(ResourceDescriptionMetadata).description("Import List");
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

ImportList::ImportList(const ImportFileEntry& entry)
{
    m_files.pushBack(entry);

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

END_BOOMER_NAMESPACE()