/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\cooking  #]
***/

#pragma once

#include "resource.h"

namespace base
{
    namespace res
    {

        //---

        /// resource thumbnail data
        class BASE_RESOURCES_API ResourceThumbnail : public IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceThumbnail, IResource);

        public:
            ResourceThumbnail();

            Buffer imagePixels; // raw image pixels, RGBA32
            uint32_t imageWidth = 0;
            uint32_t imageHeight = 0;

            Array<StringBuf> comments; // additional comments
        };

        //---

    } // res
} // base
