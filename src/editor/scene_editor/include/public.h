/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "editor_scene_editor_glue.inl"

BEGIN_BOOMER_NAMESPACE(ed)

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

class SceneContentDataNode;
typedef RefPtr<SceneContentDataNode> SceneContentDataNodePtr;
typedef RefWeakPtr<SceneContentDataNode> SceneContentDataNodeWeakPtr;

class SceneContentDirNode; // layer directory (layer group) node
class SceneContentFileNode; // layer file

class SceneContentEntityNode; // entity node
typedef RefPtr<SceneContentEntityNode> SceneContentEntityNodePtr; // entity node

class SceneContentBehaviorNode; // component node
typedef RefPtr<SceneContentBehaviorNode> SceneContentBehaviorNodePtr;

class SceneContentClipboardNode;
typedef RefPtr<SceneContentClipboardNode> SceneContentClipboardNodePtr;

class SceneContentClipboardData;
typedef RefPtr<SceneContentClipboardData> SceneContentClipboardDataPtr;

class SceneContentWorldLayer;
typedef RefPtr<SceneContentWorldLayer> SceneContentWorldLayerPtr;

class SceneContentWorldDir;
typedef RefPtr<SceneContentWorldDir> SceneContentWorldDirPtr;

//--

// events emitted by structure
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_STRUCTURE_NODE_ADDED, SceneContentNodePtr)
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_STRUCTURE_NODE_REMOVED, SceneContentNodePtr)
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_STRUCTURE_NODE_RENAMED, SceneContentNodePtr)
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_STRUCTURE_NODE_VISIBILITY_CHANGED, SceneContentNodePtr)
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_STRUCTURE_NODE_VISUAL_FLAG_CHANGED, SceneContentNodePtr)
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_STRUCTURE_NODE_MODIFIED_FLAG_CHANGED, SceneContentNodePtr)

// events emitted by node itself (only if part of structure)
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_NODE_ADDED, SceneContentNodePtr)
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_NODE_REMOVED, SceneContentNodePtr)
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_NODE_RENAMED, SceneContentNodePtr)
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_NODE_VISIBILITY_CHANGED, SceneContentNodePtr)
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_NODE_VISUAL_FLAG_CHANGED, SceneContentNodePtr)
DECLARE_GLOBAL_EVENT(EVENT_CONTENT_NODE_MODIFIED_FLAG_CHANGED, SceneContentNodePtr)

//--

enum class SceneGizmoMode : uint8_t
{
    Translation,
    Rotation,
    Scale,
};

//---

enum class SceneGizmoTarget : uint8_t
{
    WholeHierarchy,
    SelectionOnly,
};

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
    Behavior,
};

//--

enum class SceneContentNodePasteMode : uint8_t
{
    Relative, // leave all placements are they are (in parent space)
    Absolute, // place nodes at their original absolute world positions
};

//--

/// dirty visualization flags
enum class SceneContentNodeDirtyBit : uint32_t
{
    Visibility = FLAG(0), // node visibility
    Selection = FLAG(1), // selection state is dirty and requires syncing
    Transform = FLAG(2), // transform state has changed
    Content = FLAG(3), // whole content is dirty
};

typedef DirectFlags<SceneContentNodeDirtyBit> SceneContentNodeDirtyFlags;

///---

END_BOOMER_NAMESPACE(ed)
