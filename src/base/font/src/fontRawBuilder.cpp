/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: font #]
***/

#include "build.h"
#include "font.h"

#include "base/io/include/ioFileHandle.h"
#include "base/memory/include/buffer.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/resource/include/resourceCooker.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace font
    {
        //---

        // a simple raw cooker for the font file
        class FontRawCooker : public base::res::IResourceCooker
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FontRawCooker, base::res::IResourceCooker);

            virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override
            {
                // load the raw content from the source file
                const auto& rawSourceFilePath = cooker.queryResourcePath();
                auto rawContent = cooker.loadToBuffer(rawSourceFilePath);
                if (!rawContent)
                    return nullptr;

                // TODO: validate font

                // pack data into the font
                return RefNew<Font>(rawContent);
            }
        };

        RTTI_BEGIN_TYPE_CLASS(FontRawCooker);
            RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<Font>();
            RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("ttf").addSourceExtension("otf");
        RTTI_END_TYPE();

        //---

    } // font
} // base
