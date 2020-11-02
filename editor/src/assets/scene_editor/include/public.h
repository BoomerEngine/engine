/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "assets_scene_editor_glue.inl"

namespace ed
{

    //--

    class ScenePreviewContainer;
    class ScenePreviewPanel;
    typedef RefPtr<ScenePreviewPanel> ScenePreviewPanelPtr;

    class SceneContentStructure;
    class SceneContentVisualization;

    class ISceneEditMode;
    typedef RefPtr<ISceneEditMode> SceneEditModePtr;
    typedef RefWeakPtr<ISceneEditMode> SceneEditModeWeakPtr;

    class SceneContentNode;
    typedef RefPtr<SceneContentNode> SceneContentNodePtr;
    typedef RefWeakPtr<SceneContentNode> SceneContentNodeWeakPtr;

    class SceneContentDirNode; // layer directory (layer group) node
    class SceneContentFileNode; // layer file

    class SceneContentEntityNode; // entity node
    typedef RefPtr<SceneContentEntityNode> SceneContentEntityNodePtr; // entity node

    //--

    // events emitted by structure
    DECLARE_UI_EVENT(EVENT_CONTENT_STRUCTURE_NODE_ADDED, SceneContentNodePtr)
    DECLARE_UI_EVENT(EVENT_CONTENT_STRUCTURE_NODE_REMOVED, SceneContentNodePtr)
    DECLARE_UI_EVENT(EVENT_CONTENT_STRUCTURE_NODE_RENAMED, SceneContentNodePtr)
    DECLARE_UI_EVENT(EVENT_CONTENT_STRUCTURE_NODE_VISIBILITY_CHANGED, SceneContentNodePtr)
    DECLARE_UI_EVENT(EVENT_CONTENT_STRUCTURE_NODE_VISUAL_FLAG_CHANGED, SceneContentNodePtr)

    // events emitted by node itself (only if part of structure)
    DECLARE_UI_EVENT(EVENT_CONTENT_NODE_ADDED, SceneContentNodePtr)
    DECLARE_UI_EVENT(EVENT_CONTENT_NODE_REMOVED, SceneContentNodePtr)
    DECLARE_UI_EVENT(EVENT_CONTENT_NODE_RENAMED, SceneContentNodePtr)
    DECLARE_UI_EVENT(EVENT_CONTENT_NODE_VISIBILITY_CHANGED, SceneContentNodePtr)
    DECLARE_UI_EVENT(EVENT_CONTENT_NODE_VISUAL_FLAG_CHANGED, SceneContentNodePtr)

    //--

    // node type (enum)
    enum class SceneContentNodeType : uint8_t
    {
        None,
        LayerFile,
        LayerDir,
        WorldRoot,
        PrefabRoot,
        Entity,
    };

    //--

    enum class SceneContentNodePasteMode : uint8_t
    {
        Absolute,
        Relative
    };

    //--

} // ed
