/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneProxyDesc.h"

namespace rendering
{
    namespace scene
    {

        //----

        ProxyMeshDesc::ProxyMeshDesc()
            : ProxyBaseDesc(ProxyType::Mesh)
        {
        }

        //---

    } // scene
} // rendering
