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
#include "managedDirectoryPlaceholder.h"

#include "base/resource_compiler/include/importFileService.h"
#include "base/image/include/image.h"

namespace ed
{

    //--

    static res::StaticResource<image::Image> resDirectoryTexture("/engine/thumbnails/directory.png");

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(ManagedDirectoryPlaceholder);
    RTTI_END_TYPE();

    ManagedDirectoryPlaceholder::ManagedDirectoryPlaceholder(ManagedDepot* depot, ManagedDirectory* parentDir, StringView<char> initialName)
        : ManagedItem(depot, parentDir, initialName)
    {
        m_eventKey = MakeUniqueEventKey();
        m_directoryIcon = resDirectoryTexture.loadAndGetAsRef();
    }

    ManagedDirectoryPlaceholder::~ManagedDirectoryPlaceholder()
    {
    }

    void ManagedDirectoryPlaceholder::rename(StringView<char> name)
    {
        if (ValidateFileName(name))
        {
            m_name = StringBuf(name);
        }
    }

    const image::ImageRef& ManagedDirectoryPlaceholder::typeThumbnail() const
    {
        return m_directoryIcon;
    }

    //--

} // depot
