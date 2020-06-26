/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#pragma once

#include "sceneAttachment.h"

namespace scene
{
    //---

    // transformation attachment between two elements - keeps the elements 
    class SCENE_COMMON_API TransformAttachment : public IAttachment
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TransformAttachment, IAttachment);

    public:
        TransformAttachment();
        virtual ~TransformAttachment();
    };

    //---

} // scene
