/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#include "build.h"
#include "font.h"
#include "fontLibrary.h"
#include "core/io/include/fileHandle.h"
#include "core/resource/include/depot.h"

BEGIN_BOOMER_NAMESPACE()

//--

FontPtr LoadFontFromMemory(Buffer ptr)
{
    return RefNew<Font>(ptr);
}

FontPtr LoadFontFromFile(IReadFileHandle* file)
{
    if (file)
    {
        const auto size = file->size();

        auto data = Buffer::Create(POOL_IMAGE, size, 16);
        if (data)
        {
            if (file->readSync(data.data(), size) == size)
            {
                return LoadFontFromMemory(data);
            }
            else
            {
                TRACE_WARNING("Failed to load {} bytes from file");
            }
        }
        else
        {
            TRACE_WARNING("Out of memory allocating data buffer for font");
        }
    }

    return nullptr;
}

FontPtr LoadFontFromAbsolutePath(StringView absolutePath)
{
    if (const auto file = OpenForReading(absolutePath))
        return LoadFontFromFile(file);
    return nullptr;
}

FontPtr LoadFontFromDepotPath(StringView depotPath)
{
    if (const auto file = GetService<DepotService>()->createFileReader(depotPath))
        return LoadFontFromFile(file);
    return nullptr;
}

END_BOOMER_NAMESPACE()
