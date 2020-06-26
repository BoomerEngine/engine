/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "scene_objects_glue.inl"

namespace scene
{
    namespace objects
    {

        class IAttachment;
        typedef base::RefPtr<IAttachment> AttachmentPtr;

        class TransformAttachment;
        typedef base::RefPtr<TransformAttachment> TransformAttachmentPtr;

        class Element;
        typedef base::RefPtr<Element> ElementPtr;
        typedef base::RefWeakPtr<Element> ElementWeakPtr;

    } // objects
} // scene
