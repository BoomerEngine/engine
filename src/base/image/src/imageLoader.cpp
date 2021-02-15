/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: loader #]
***/

#include "build.h"
#include "image.h"
#include "imageView.h"

#include "base/io/include/ioFileHandle.h"
#include "base/resource/include/depotService.h"

#include "freeImageLoader.h"

namespace base
{
    //--

    image::ImagePtr LoadImageFromMemory(const void* memory, uint64_t size)
    {
        auto loadedImage = image::LoadImageWithFreeImage(memory, size);
        if (loadedImage)
            return base::RefNew<base::image::Image>(loadedImage->view());

        return nullptr;
    }

    image::ImagePtr LoadImageFromMemory(Buffer ptr)
    {
        if (ptr)
            return LoadImageFromMemory(ptr.data(), ptr.size());
        return nullptr;
    }

    image::ImagePtr LoadImageFromFile(io::IReadFileHandle* file)
    {
        if (file)
        {
            const auto size = file->size();

            auto data = Buffer::Create(POOL_IMAGE, size, 16);
            if (data)
            {
                if (file->readSync(data.data(), size) == size)
                {
                    return LoadImageFromMemory(data);
                }
                else
                {
                    TRACE_WARNING("Failed to load {} bytes from file");
                }
            }
            else
            {
                TRACE_WARNING("Out of memory allocating data buffer for image");
            }
        }

        return nullptr;
    }

    image::ImagePtr LoadImageFromAbsolutePath(StringView absolutePath)
    {
        if (const auto file = io::OpenForReading(absolutePath))
            return LoadImageFromFile(file);
        return nullptr;
    }

    image::ImagePtr LoadImageFromDepotPath(StringView depotPath)
    {
        if (const auto file = GetService<DepotService>()->createFileReader(depotPath))
            return LoadImageFromFile(file);
        return nullptr;
    }

    //--

} // base