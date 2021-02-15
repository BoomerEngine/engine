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

        /// Uncooked scene
        /// Layer is composed of a list of node templates
        class BASE_WORLD_API RawScene : public res::IResource
        {
            RTTI_DECLARE_POOL(POOL_WORLD_OBJECTS)
            RTTI_DECLARE_VIRTUAL_CLASS(RawScene, res::IResource);

        public:
            RawScene();

        private:
        };

        //---

    } // game
} // base