/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world\streaming #]
***/

#pragma once

#include "base/resource/include/resource.h"

namespace base
{
    namespace world
    {

        //---

        /// sector in the streaming grid, contains streaming islands that have a streaming box fully contained within the sector, baked by the world compiler
        class BASE_WORLD_API StreamingSector : public res::IResource
        {
            RTTI_DECLARE_POOL(POOL_WORLD_STREAMING)
            RTTI_DECLARE_VIRTUAL_CLASS(StreamingSector, res::IResource);

        public:
            struct Setup
            {
                Array<StreamingIslandPtr> islands;
            };

            StreamingSector();
            StreamingSector(const Setup& setup);

            // streaming region
            INLINE const Box& streamingBounds() const { return m_streamingBox; }

            // root islands
            INLINE const Array<StreamingIslandPtr>& rootIslands() const { return m_islands; }

            //--

        private:
            Box m_streamingBox;
            Array<StreamingIslandPtr> m_islands;
        };

        //--

    } // world
} // base