/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
*
***/

#include "build.h"
#include "worldCompiledScene.h"
#include "worldStreamingSector.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace world
    {

        ///---

        RTTI_BEGIN_TYPE_CLASS(CompiledSceneStreamingCell);
            RTTI_PROPERTY(streamingBox);
            RTTI_PROPERTY(data);
        RTTI_END_TYPE();

        CompiledSceneStreamingCell::CompiledSceneStreamingCell()
        {
        }

        ///----

        RTTI_BEGIN_TYPE_CLASS(CompiledScene);
            RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4cscene");
            RTTI_METADATA(res::ResourceDescriptionMetadata).description("Compiled Scene");
            RTTI_METADATA(res::ResourceTagColorMetadata).color(0x9d, 0x02, 0x08);
            RTTI_PROPERTY(m_streamingCells);
        RTTI_END_TYPE();

        CompiledScene::CompiledScene()
        {
        }

        CompiledScene::CompiledScene(const Setup& setup)
        {
            m_streamingCells = std::move(setup.cells);
        }

        ///----

    } // world
} // base
