/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes #]
***/

#pragma once

#include "rendering/scene/include/renderingSelectable.h"
#include "assets/gizmos/include/gizmoReferenceSpace.h"

namespace ed
{
    //--

    /// editor edit mode
    class ASSETS_SCENE_EDITOR_API ISceneEditMode : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ISceneEditMode, IObject)

    public:
        ISceneEditMode(ActionHistory* actionHistory);
        virtual ~ISceneEditMode();

        //--

        // action history used within the edit mode
        INLINE ActionHistory* actionHistory() const { return m_actionHistory; }

        // get active editor scene container, valid only if edit mode is active
        INLINE ScenePreviewContainer* container() const { return m_container; }

        //--

        // activate edit mode
        void acivate(ScenePreviewContainer* container);

        // deactivate edit mode
        void deactivate(ScenePreviewContainer* container);

        //--

        // get the UI interface to show in the "Details" when this edit mode is active
        virtual ui::ElementPtr queryUserInterface() const;

        // get the nodes that are selected in the edit mod
        virtual Array<SceneContentNodePtr> querySelection() const;

        // do we have anything selected ?
        virtual bool hasSelection() const;

        // create toolbar content 
        virtual void configurePanelToolbar(ScenePreviewContainer* container, const ScenePreviewPanel* panel, ui::ToolBar* toolbar);

        // configure parent editor edit menu
        virtual void configureEditMenu(ui::MenuButtonContainer* menu);

        // configure parent editor view menu
        virtual void configureViewMenu(ui::MenuButtonContainer* menu);

        //--

        // create gizmos for viewport
        virtual GizmoGroupPtr configurePanelGizmos(ScenePreviewContainer* container, const ScenePreviewPanel* panel);

        // compute the reference frame for gizmo operations
        virtual GizmoReferenceSpace calculateGizmoReferenceSpace() const;

        // create gizmo transform action
        virtual GizmoActionContextPtr createGizmoAction(ScenePreviewContainer* container, const ScenePreviewPanel* panel) const;

        //--

        // render edit mode content, called for each panel separately
        virtual void handleRender(ScenePreviewPanel* panel, rendering::scene::FrameParams& frame);

        // handle custom viewport click (if no handler is returned we default to camera movement/selection)
        virtual ui::InputActionPtr handleMouseClick(ScenePreviewPanel* panel, const input::MouseClickEvent& evt);

        // handle custom viewport action-less keyboard event
        virtual bool handleKeyEvent(ScenePreviewPanel* panel, const base::input::KeyEvent& evt);

        // handle viewport selection result
        virtual void handlePointSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables);

        // handle viewport selection result
        virtual void handleAreaSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables);

        // handle viewport context menu
        virtual void handleContextMenu(ScenePreviewPanel* panel, bool ctrl, bool shift, const ui::Position& absolutePosition, const base::Point& clientPosition, const rendering::scene::Selectable& objectUnderCursor, const base::AbsolutePosition* positionUnderCursor);

        // update state
        virtual void handleUpdate(float dt);

        // handle context menu in the structure view
        virtual void handleTreeContextMenu(ui::MenuButtonContainer* menu, const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection);

        // handle selection change in the tree
        virtual void handleTreeSelectionChange(const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection);

        // handle request to delete tree nodes
        virtual void handleTreeDeleteNodes(const Array<SceneContentNodePtr>& selection);

        // handle request to cut tree nodes
        virtual void handleTreeCutNodes(const Array<SceneContentNodePtr>& selection);

        // handle request to copy tree nodes
        virtual void handleTreeCopyNodes(const Array<SceneContentNodePtr>& selection);

        // handle request to paste nodes at given tree node
        virtual void handleTreePasteNodes(const SceneContentNodePtr& target, SceneContentNodePasteMode mode);

        // handle a resource drop on a scene node
        virtual bool handleTreeResourceDrop(const SceneContentNodePtr& target, const ManagedFile* file);

        // handle a drag&drop of another node
        virtual bool handleTreeNodeDrop(const SceneContentNodePtr& target, const SceneContentNodePtr& source);

        //--

        // general clipboard operations
        virtual void handleGeneralCopy();
        virtual void handleGeneralCut();
        virtual void handleGeneralPaste();
        virtual void handleGeneralDelete();
        virtual void handleGeneralDuplicate();

        // general clipboard operation filter
        virtual bool checkGeneralCopy() const;
        virtual bool checkGeneralCut() const;
        virtual bool checkGeneralPaste() const;
        virtual bool checkGeneralDelete() const;
        virtual bool checkGeneralDuplicate() const;

    private:
        ScenePreviewContainer* m_container = nullptr;
        ActionHistory* m_actionHistory = nullptr;
    };

    //--

    // create default set of tools for grid control
    extern ASSETS_SCENE_EDITOR_API void CreateDefaultGridButtons(ScenePreviewContainer* container, ui::ToolBar* toolbar);

    // create default set of tools for selection
    extern ASSETS_SCENE_EDITOR_API void CreateDefaultSelectionButtons(ScenePreviewContainer* container, ui::ToolBar* toolbar);

    // create default set of tools for gizmos
    extern ASSETS_SCENE_EDITOR_API void CreateDefaultGizmoButtons(ScenePreviewContainer* container, ui::ToolBar* toolbar);

    // create default set of tools for object creation
    extern ASSETS_SCENE_EDITOR_API void CreateDefaultCreationButtons(ScenePreviewContainer* container, ui::ToolBar* toolbar);

    //--

} // ed
