/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\cooking #]
***/

#include "build.h"
#include "resourceThumbnail.h"

namespace base
{
    namespace res
    {
        //--

        RTTI_BEGIN_TYPE_CLASS(ResourceThumbnail);
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("thumb");
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Thumbnail");
            RTTI_PROPERTY(imagePixels);
            RTTI_PROPERTY(imageWidth);
            RTTI_PROPERTY(imageHeight);
            RTTI_PROPERTY(comments);
        RTTI_END_TYPE();

        ResourceThumbnail::ResourceThumbnail()
        {}

        //--

    } // res
} // base
