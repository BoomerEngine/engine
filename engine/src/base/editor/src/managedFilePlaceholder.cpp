/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#include "build.h"
#include "managedDepot.h"
#include "managedDirectory.h"
#include "managedFileFormat.h"
#include "managedFilePlaceholder.h"
#include "managedThumbnails.h"
#include "base/resource_compiler/include/importFileService.h"

namespace ed
{

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedFilePlaceholder);
    RTTI_END_TYPE();

    static StringBuf FormatFileName(StringView<char> initialName, const ManagedFileFormat* format)
    {
        if (initialName.empty())
            initialName = "file";

        return TempString("{}.{}", initialName, format->extension());
    }

    ManagedFilePlaceholder::ManagedFilePlaceholder(ManagedDepot* depot, ManagedDirectory* parentDir, StringView<char> initialName, const ManagedFileFormat* format)
        : ManagedItem(depot, parentDir, FormatFileName(initialName, format))
        , m_fileFormat(format)
    {
        m_eventKey = MakeUniqueEventKey();
    }

    ManagedFilePlaceholder::~ManagedFilePlaceholder()
    {
    }

    const image::ImageRef& ManagedFilePlaceholder::typeThumbnail() const
    {
        return m_fileFormat->thumbnail();
    }

    void ManagedFilePlaceholder::rename(StringView<char> name)
    {
        if (ValidateFileName(name))
        {
            if (m_shortName != name)
            {
                m_shortName = StringBuf(name);
                m_name = FormatFileName(m_shortName, m_fileFormat);
            }
        }
    }

    //--

} // depot

