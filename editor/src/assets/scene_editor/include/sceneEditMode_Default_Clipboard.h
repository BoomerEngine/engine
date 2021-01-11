/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#pragma once

#include "sceneEditMode.h"
#include "assets/gizmos/include/gizmoGroup.h"

namespace ed
{
    //--

    // node in the clipboard data
    class ASSETS_SCENE_EDITOR_API SceneContentClipboardNode : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentClipboardNode, IObject);

    public:
        SceneContentNodeType type; // type of content
        StringBuf name;

        EulerTransform localPlacement; // original placement with respect to parent
        AbsoluteTransform worldPlacement; // original placement in the world space

        //--

        world::NodeTemplatePtr packedEntityData;
        ObjectIndirectTemplatePtr packedComponentData;

        //--

        SceneContentClipboardNode();
    };

    // clipboard data
    class ASSETS_SCENE_EDITOR_API SceneContentClipboardData : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneContentClipboardData, IObject);

    public:
        SceneContentNodeType type; // type of root content (must be uniform, we can't store entities AND components in the same clipboard data)
        Array<SceneContentClipboardNodePtr> data; // objects in clipboard

        SceneContentClipboardData();
    };

    // build clipboard data from selected nodes
    extern ASSETS_SCENE_EDITOR_API SceneContentClipboardDataPtr BuildClipboardDataFromNodes(const Array<SceneContentNodePtr>& nodes);

    //--    

} // ed
