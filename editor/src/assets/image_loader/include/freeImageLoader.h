/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: image #]
***/

#pragma once

#include "base/resources/include/resource.h"
#include "base/memory/include/buffer.h"

namespace base 
{
    namespace image
    {

        ///---

        /// image data loaded using the free image library
        struct ASSETS_IMAGE_LOADER_API FreeImageLoadedData : public IReferencable
        {
            uint32_t width = 0;
            uint32_t height = 0;

            uint32_t pixelPitch = 0;
            uint32_t rowPitch = 0;

            uint8_t channels = 0;
            PixelFormat format = PixelFormat::Uint8_Norm;

            uint8_t* data = nullptr;

            //-

            FreeImageLoadedData(void* object);
            ~FreeImageLoadedData();

            // get view of the whole data
            ImageView view() const;

        protected:
            void* object = nullptr;
        };

        /// load a image using the free image library
        extern ASSETS_IMAGE_LOADER_API RefPtr<FreeImageLoadedData> LoadImageWithFreeImage(const void* data, uint64_t dataSize);

        ///---

    } // image
} // base