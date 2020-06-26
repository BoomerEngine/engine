/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fbx #]
***/

#pragma once

#include "base/resources/include/resource.h"

namespace fbx
{

    //---

    /// manifest for importing meshes from FBX file
    class ASSETS_FBX_LOADER_API MaterialCooker : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialCooker, base::res::IResourceCooker);

    public:
        MaterialCooker();

        /// bake a final (cooked) resource using this manifest and provided extra stuff
        virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override final;
    };

    //---

} // fbx
