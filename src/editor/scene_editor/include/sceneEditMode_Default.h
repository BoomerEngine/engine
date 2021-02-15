/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#pragma once

#include "sceneEditMode.h"
#include "editor/gizmos/include/gizmoGroup.h"

namespace ed
{
    //--

    class SceneDefaultPropertyInspectorPanel;
    class SceneObjectPalettePanel;
    class SceneObjectDragDropCreationHandler;

    //--

    /// default scene editor edit mode - entity operations
    class EDITOR_SCENE_EDITOR_API SceneEditMode_Default : public ISceneEditMode
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SceneEditMode_Default, ISceneEditMode)

    public:
        SceneEditMode_Default(ActionHistory* actionHistory);
        virtual ~SceneEditMode_Default();

        //--

        // ISceneEditMode
        virtual ui::ElementPtr queryUserInterface() const override;
        virtual Array<SceneContentNodePtr> querySelection() const override;
        virtual bool hasSelection() const override;
        virtual void configurePanelToolbar(ScenePreviewContainer* container, const ScenePreviewPanel* panel, ui::ToolBar* toolbar) override;

        virtual GizmoGroupPtr configurePanelGizmos(ScenePreviewContainer* container, const ScenePreviewPanel* panel) override;
        virtual GizmoReferenceSpace calculateGizmoReferenceSpace() const override;
        virtual GizmoActionContextPtr createGizmoAction(ScenePreviewContainer* container, const ScenePreviewPanel* panel) const override;
        virtual void configureEditMenu(ui::MenuButtonContainer* menu) override;
        virtual void configureViewMenu(ui::MenuButtonContainer* menu) override;

        virtual void handleRender(ScenePreviewPanel* panel, rendering::scene::FrameParams& frame) override;
        virtual ui::InputActionPtr handleMouseClick(ScenePreviewPanel* panel, const input::MouseClickEvent& evt) override;
        virtual bool handleKeyEvent(ScenePreviewPanel* panel, const base::input::KeyEvent& evt) override;
        virtual void handleContextMenu(ScenePreviewPanel* panel, bool ctrl, bool shift, const ui::Position& absolutePosition, const base::Point& clientPosition, const rendering::scene::Selectable& objectUnderCursor, const base::AbsolutePosition* positionUnderCursor) override;
        virtual void handlePointSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables) override;
        virtual void handleAreaSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables) override;
        virtual ui::DragDropHandlerPtr handleDragDrop(ScenePreviewPanel* panel, const ui::DragDropDataPtr& data, const ui::Position& absolutePosition, const base::Point& clientPosition) override;
        virtual void handleUpdate(float dt);
        
        virtual void handleTreeContextMenu(ui::MenuButtonContainer* menu, const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection) override;
        virtual void handleTreeSelectionChange(const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection) override;
        virtual void handleTreeDeleteNodes(const Array<SceneContentNodePtr>& selection) override;
        virtual void handleTreeCutNodes(const Array<SceneContentNodePtr>& selection) override;
        virtual void handleTreeCopyNodes(const Array<SceneContentNodePtr>& selection) override;
        virtual void handleTreePasteNodes(const SceneContentNodePtr& target, SceneContentNodePasteMode mode) override;
        virtual bool handleTreeResourceDrop(const SceneContentNodePtr& target, const ManagedFile* file) override;
        virtual bool handleTreeNodeDrop(const SceneContentNodePtr& target, const SceneContentNodePtr& source) override;

        //--

        virtual void handleGeneralCopy() override;
        virtual void handleGeneralCut() override;
        virtual void handleGeneralPaste() override;
        virtual void handleGeneralDelete() override;
        virtual void handleGeneralDuplicate() override;

        virtual bool checkGeneralCopy() const override;
        virtual bool checkGeneralCut() const override;
        virtual bool checkGeneralPaste() const override;
        virtual bool checkGeneralDelete() const override;
        virtual bool checkGeneralDuplicate() const override;

        //--

        virtual void configSave(ScenePreviewContainer* container, const ui::ConfigBlock& block) const;
        virtual void configLoad(ScenePreviewContainer* container, const ui::ConfigBlock& block);

        //--

        INLINE const HashSet<SceneContentNodePtr>& selection() const { return m_selection; }

        INLINE const Array<SceneContentDataNodePtr>& selectionRoots() const { return m_selectionRoots; }

        INLINE const Array<SceneContentEntityNodePtr>& selectionRootEntities() const { return m_selectionRootEntities; }

        INLINE void bindObjectPalette(SceneObjectPalettePanel* panel) { m_objectPalette = panel; }

        void actionChangeSelection(const Array<SceneContentNodePtr>& selection);

        void changeSelection(const Array<SceneContentNodePtr>& selection);

        void handleSelectionChanged();

        void handleTransformsChanged();

        void activeNode(const SceneContentNode* node);

        void focusNode(const SceneContentNode* node);

        void focusNodes(const Array<SceneContentNodePtr>& nodes);

        void buildTransformNodeListFromSelection(Array<SceneContentDataNodePtr>& outTransformList) const;

        void reset();

        //--

        void cmdAddPrefabFile(const Array<SceneContentNodePtr>& nodes, const ManagedFile* file);
        void cmdRemovePrefabFile(const Array<SceneContentNodePtr>& nodes, const Array<const ManagedFile*>& files);
        void cmdMovePrefabFileUp(const Array<SceneContentNodePtr>& nodes, const ManagedFile* file);
        void cmdMovePrefabFileDown(const Array<SceneContentNodePtr>& nodes, const ManagedFile* file);
        void cmdEnablePrefabFile(const Array<SceneContentNodePtr>& nodes, const Array<const ManagedFile*>& files);
        void cmdDisablePrefabFile(const Array<SceneContentNodePtr>& nodes, const Array<const ManagedFile*>& files);

        //--

        static void EnsureParentsFirst(const Array<SceneContentNodePtr>& nodes, Array<SceneContentDataNodePtr>& outTransformList);
        static void ExtractSelectionRoots(const Array<SceneContentNodePtr>& nodes, Array<SceneContentDataNodePtr>& outRoots);
        static void ExtractSelectionRoots(const Array<SceneContentNodePtr>& nodes, Array<SceneContentNodePtr>& outRoots);
        static void ExtractSelectionHierarchy(const SceneContentDataNode* node, Array<SceneContentDataNodePtr>& outNodes);
        static void ExtractSelectionHierarchyWithFilter(const SceneContentDataNode* node, Array<SceneContentDataNodePtr>& outNodes, const HashSet<SceneContentDataNode*>& coreSet, int depth = 0);
        static void ExtractSelectionHierarchyWithFilter2(const SceneContentDataNode* node, Array<SceneContentDataNodePtr>& outNodes, const HashMap<SceneContentNode*, int>& coreSet);

    protected:
        void processObjectDeletion(const Array<SceneContentNodePtr>& selection);
        void processObjectCopy(const Array<SceneContentNodePtr>& selection);
        void processObjectCut(const Array<SceneContentNodePtr>& selection);
        void processObjectPaste(const SceneContentNodePtr& context, const SceneContentClipboardDataPtr& data, SceneContentNodePasteMode mode, const AbsoluteTransform* worldPlacement=nullptr);
        void processObjectDuplicate(const Array<SceneContentNodePtr>& selection);
        void processObjectHide(const Array<SceneContentNodePtr>& selection);
        void processObjectShow(const Array<SceneContentNodePtr>& selection);
        void processObjectToggleVis(const Array<SceneContentNodePtr>& selection);
        void processUnhideAll();

        void processSaveAsPrefab(const Array<SceneContentNodePtr>& selection);
        void processUnwrapPrefab(const Array<SceneContentNodePtr>& selection, bool explode);
        void processReplaceWithClipboard(const Array<SceneContentNodePtr>& selection);

        void processGenericPrefabAction(const Array<SceneContentNodePtr>& nodes, const std::function<world::NodeTemplatePtr(world::NodeTemplate* currentData)>& func);

        void processCreateLayer(const Array<SceneContentNodePtr>& selection);
        void processCreateDirectory(const Array<SceneContentNodePtr>& selection);

        RefPtr<SceneDefaultPropertyInspectorPanel> m_panel;

        //--

        HashSet<SceneContentNodePtr> m_selection;

        Array<SceneContentDataNodePtr> m_selectionRoots;
        Array<SceneContentEntityNodePtr> m_selectionRootEntities;

        bool m_canCopySelection = false;
        bool m_canCutSelection = false;
        bool m_canDeleteSelection = false;

        void updateSelectionFlags();

        //--

        RefPtr<ui::ClassPickerBox> m_entityClassSelector;
        RefPtr<ui::ClassPickerBox> m_behaviorClassSelector;

        SceneContentNodeWeakPtr m_activeNode;

        void processVisualSelection(bool ctrl, bool shift, const base::Array<rendering::scene::Selectable>& selectables);

        void createEntityAtNodes(const Array<SceneContentNodePtr>& selection, ClassType entityClass, const AbsoluteTransform* initialPlacement = nullptr, const ManagedFile* resourceFile = nullptr);
        //void createEntityWithComponentAtNodes(const Array<SceneContentNodePtr>& selection, ClassType componentClass, const AbsoluteTransform* initialPlacement = nullptr, const ManagedFile* resourceFile = nullptr);
        //void createComponentAtNodes(const Array<SceneContentNodePtr>& selection, ClassType componentClass, const AbsoluteTransform* initialPlacement = nullptr, const ManagedFile* resourceFile = nullptr);
        void createPrefabAtNodes(const Array<SceneContentNodePtr>& selection, const ManagedFile* prefab, const AbsoluteTransform* initialPlacement = nullptr);
        
        void changeGizmo(SceneGizmoMode mode);
        void changeGizmoNext();
        void changePositionGridSize(int delta);

        bool handleInternalKeyAction(input::KeyCode key, bool shift, bool alt, bool ctrl);

        //--

        struct ContextMenuSetup
        {
            bool viewportBased = false;

            Array<SceneContentNodePtr> selection;

            SceneContentNodePtr contextTreeItem;
            SceneContentNodePtr contextClickedItem;
            AbsolutePosition contextWorldPosition;
            bool contextWorldPositionValid = false;
        };

        void buildContextMenu(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup);

        void buildContextMenu_Focus(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup);
        void buildContextMenu_ShowHide(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup);
        void buildContextMenu_Create(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup);
        void buildContextMenu_Clipboard(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup);
        void buildContextMenu_Prefab(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup);
        void buildContextMenu_ContextNode(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup);
        void buildContextMenu_Resources(ui::MenuButtonContainer* menu, const ContextMenuSetup& setup);

        Array<SceneContentNodePtr> m_contextMenuContextNodes;
        AbsoluteTransform m_contextMenuPlacementTransform; // hack - this should be passed via capture but there's no nice way to wire it
        bool m_contextMenuPlacementTransformValid = false;

        SceneObjectPalettePanel* m_objectPalette = nullptr;

        //--

        RefWeakPtr<SceneObjectDragDropCreationHandler> m_dragDropHandler;

        void renderDragDrop(ScenePreviewPanel* panel, rendering::scene::FrameParams& frame);
        void updateDragDrop();

        //--
    };
    
    //--

    struct ActionMoveSceneNodeData
    {
        SceneContentDataNodePtr node;
        EulerTransform oldTransform;
        EulerTransform newTransform;
    };

    // create transform action
    extern EDITOR_SCENE_EDITOR_API ActionPtr CreateSceneNodeTransformAction(Array<ActionMoveSceneNodeData>&& nodes, SceneEditMode_Default* mode, bool fullRefresh);

    //--

    class ISceneObjectPreview : public IReferencable
    {
    public:
        ISceneObjectPreview();

        virtual void attachToWorld(world::World* world) = 0;
        virtual void detachFromWorld(world::World* world) = 0;
    };

    //--

} // ed
