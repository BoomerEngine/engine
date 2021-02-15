/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\resources #]
***/

#pragma once

#include "base/resource/include/resource.h"

namespace base
{
    namespace world
    {
        //---

        /// streaming cell
        struct BASE_WORLD_API CompiledSceneStreamingCell
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(CompiledSceneStreamingCell);

        public:
            CompiledSceneStreamingCell();

            Box streamingBox;
            StreamingSectorAsyncRef data; // sector data (packed entity islands + inplace resources)
        };

        //---

        /// Cooked scene
        class BASE_WORLD_API CompiledScene : public res::IResource
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
            RTTI_DECLARE_VIRTUAL_CLASS(CompiledScene, res::IResource);

        public:
            struct Setup
            {
                Array<CompiledSceneStreamingCell> cells;
            };

            CompiledScene();
            CompiledScene(const Setup& setup);

            // get streaming cells
            INLINE const Array<CompiledSceneStreamingCell>& streamingCells() const { return m_streamingCells; }

        private:
            Array<CompiledSceneStreamingCell> m_streamingCells;
        };

        //---

    } // game
} // base