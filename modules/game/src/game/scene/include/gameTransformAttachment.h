/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity\attachments #]
*
***/

#pragma once

#include "gameAttachment.h"

namespace game
{
    //---

    // transformation attachment between two elements - keeps the elements 
    class GAME_SCENE_API TransformAttachment : public IAttachment
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TransformAttachment, IAttachment);

    public:
        TransformAttachment();
        virtual ~TransformAttachment();
    };

    //---

} // game
