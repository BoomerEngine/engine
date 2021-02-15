/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image #]
***/

#include "build.h"

#include "base/image/include/image.h"
#include "base/image/include/imageVIew.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/resource/include/resource.h"
#include "base/io/include/ioFileHandle.h"
#include "base/resource/include/resourceCooker.h"
#include "base/resource/include/resourceTags.h"

#include "freeImageLoader.h"

#include <freeimage/FreeImage.h>

namespace base
{
    namespace image
    {

        //---

        // FreeImage loader
        class IMPORT_IMAGE_LOADER_API FreeImageCooker : public base::res::IResourceCooker
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FreeImageCooker, base::res::IResourceCooker);

        public:
            virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override final
            {
                // load content of the file to buffer
                const auto& importPath = cooker.queryResourcePath();
                auto content = cooker.loadToBuffer(importPath);
                if (content.size() <= 4)
                {
                    TRACE_ERROR("No bitmap content for file '{}'", importPath);
                    return false;
                }

                // load image
                auto loadedImage = LoadImageWithFreeImage(content.data(), content.size());
                if (!loadedImage)
                {
                    TRACE_ERROR("Unable to load content of image from file '{}'", importPath);
                    return false;
                }

                // create the wrapper for sources
                return base::RefNew<base::image::Image>(loadedImage->view());
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FreeImageCooker);
            RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<base::image::Image>();
            RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtensions("bmp;dds;png;jpg;jpeg;jp2;jpx;tga;tif;tiff;hdr;exr;ppm;pbm;psd;xbm;nef;xpm;gif;webp");
        RTTI_END_TYPE();

        //---

    } // image
} // base