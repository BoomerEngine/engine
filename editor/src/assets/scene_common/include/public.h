/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "assets_scene_common_glue.inl"

namespace ed
{

    //--

    class EditorScene;
    typedef RefPtr<EditorScene> EditorScenePtr;

    class EditorNode;
    typedef RefPtr<EditorNode> EditorNodePtr;
    typedef RefWeakPtr<EditorNode> EditorNodeWeakPtr;

    class EditorNodePivot;
    typedef RefPtr<EditorNodePivot> EditorNodePivotPtr;

    class EditorNodeTransform;
    typedef RefPtr<EditorNodeTransform> EditorNodeTransformPtr;

    class IEditorSceneEditMode;
    typedef RefPtr<IEditorSceneEditMode> EditorSceneEditModePtr;
    typedef RefWeakPtr<IEditorSceneEditMode> EditorSceneEditModeWeakPtr;

    class EditorScenePreviewContainer;
    typedef RefPtr<EditorScenePreviewContainer> EditorScenePreviewContainerPtr;

    class EditorScenePreviewPanel;
    typedef RefPtr<EditorScenePreviewPanel> EditorScenePreviewPanelPtr;

    //--

} // ed
