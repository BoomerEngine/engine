/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\streaming #]
*
***/

#include "build.h"
#include "worldStreamingIsland.h"
#include "worldStreamingSector.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace world
    {
        ///---

        RTTI_BEGIN_TYPE_CLASS(StreamingSector);
            RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4sector");
            RTTI_METADATA(res::ResourceDescriptionMetadata).description("World Sector");
            RTTI_METADATA(res::ResourceTagColorMetadata).color(0x99, 0x22, 0x44);
            RTTI_PROPERTY(m_islands);
        RTTI_END_TYPE();

        StreamingSector::StreamingSector()
        {}

        StreamingSector::StreamingSector(const Setup& setup)
            : m_islands(setup.islands)
        {
            m_streamingBox.clear();

            for (auto& island : m_islands)
            {
                m_streamingBox.merge(island->streamingBounds());
                island->parent(this);
            }
        }

        ///---

    } // world
} // game


