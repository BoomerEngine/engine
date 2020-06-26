/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/object/include/object.h"
#include "base/resources/include/resourceCookable.h"

#include "editor/asset_browser/include/resourceContentFileEditorTab.h"
#include "editor/asset_browser/include/resourceContentFileEditorTab.h"
#include "editor/asset_browser/include/editorService.h"
#include "editor/asset_browser/include/editorConfig.h"

namespace ed
{
    namespace world
    {
        class PlaceableNodeTemplateRegistry;

        /// scene editor is a separate window
        class EDITOR_SCENE_MAIN_API SceneEditorTab : public resources::EditorTab, public IConfigUser
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneEditorTab, resources::EditorTab);

        public:
            SceneEditorTab(const depot::ManagedFilePtr& file);
            ~SceneEditorTab();

            /// get scene content
            INLINE ContentStructure& content() const { return *m_content; }

            /// get the scene we use for visualization
            INLINE const scene::ScenePtr& scene() const { return m_scene; }

            /// get the temp object system
            INLINE SceneTempObjectSystem& tempObjects() const { return *m_tempObjects; }

            /// get gizmo settings
            INLINE const SceneGizmoSettings& gizmoSettings() const { return m_gizmoSettings; }

            /// get grid settings
            INLINE const SceneGridSettings& gridSettings() const { return m_gridSettings; }

            /// get selection settings
            INLINE const SceneSelectionSettings& selectionSettings() const { return m_selectionSettings; }

            /// get all rendering panels we use to observer the scene
            INLINE const base::Array<SceneRenderingPanel*>& renderingPanels() const { return m_renderingPanels;  }

            /// get active selection of element in the scene, changed via operations in edit mode, tree selection or undo/redo actions
            INLINE const base::HashSet<ContentNodePtr>& selectionSet() const { return m_selection; }
            INLINE const base::Array<ContentNodePtr>& selection() const { return m_selection.keys(); }

            //--

            // change gizmo settings in the app
            void gizmoSettings(const SceneGizmoSettings& settings);

            // change grid settings
            void gridSettings(const SceneGridSettings& settings);

            // change selection settings
            void selectionSettings(const SceneSelectionSettings& settings);

            //--

            // viewport has new selection request
            // NOTE: selection may be totally override by current edit mode, usually when in sub-edit mode we can't change selection of beyond the sub-objects types)
            void viewportSelectionChanged(SceneRenderingPanel* panel, bool ctrl, bool shift, const base::Array<rendering::scene::Selectable>& objects);

            // general mouse click event in viewport
            ui::InputActionPtr viewportMouseClickEvent(SceneRenderingPanel* panel, const ui::ElementArea& area, const base::input::MouseClickEvent& evt);

            // general mouse movement event in viewport
            bool viewportMouseMoveEvent(SceneRenderingPanel* panel, const base::input::MouseMovementEvent& evt);

            // general mouse movement event in viewport
            bool viewportMouseWheelEvent(SceneRenderingPanel* panel, const base::input::MouseMovementEvent& evt, float delta);

            // handle specific context menu
            bool viewportContextMenu(SceneRenderingPanel* panel, const ui::ElementArea& area, const ui::Position& absolutePosition, const base::Array<rendering::scene::Selectable>& objectUnderCursor, const base::Vector3* knownWorldPositionUnderCursor);

            // handle a key event
            bool viewportKeyEvent(SceneRenderingPanel* panel, const base::input::KeyEvent& evt);

            // handle drag&drop
            ui::DragDropHandlerPtr viewportDragAndDrop(SceneRenderingPanel* panel, const ui::DragDropDataPtr& data, const ui::Position& entryPosition);

            // handle in-viewport rendering
            void viewportRenderDebugFragments(SceneRenderingPanel* panel, rendering::scene::FrameInfo& frame);

            //--

            // clear selection - deselect all nodes
            void clearSelection();

            // select/unselect a single node
            void changeSelection(const ContentNodePtr& node, bool selected);

            // set new selection
            void changeSelection(const base::Array<ContentNodePtr>& newSelection);

            // check if node is selected
            bool isSelected(const ContentNodePtr& node) const;

            //--

            // attach a rendering panel
            void attachRenderingPanel(SceneRenderingPanel* panel);

            // detach rendering panel
            void detachRenderingPanel(SceneRenderingPanel* panel);

        private:
            base::RefPtr<ContentStructure> m_content;
            base::UniquePtr<PlaceableNodeTemplateRegistry> m_palette;
            base::RefPtr<ui::DockPanel> m_editModePanel;

            base::Array<SceneRenderingPanel*> m_renderingPanels;

            scene::ScenePtr m_scene;
            base::RefPtr<SceneTempObjectSystem> m_tempObjects;

            base::Array<float> m_positionGridSizes;
            base::Array<float> m_rotationGridSizes;

            ConfigPath m_fileConfigPath;

            SceneGizmoSettings m_gizmoSettings;
            SceneGridSettings m_gridSettings;
            SceneSelectionSettings m_selectionSettings;

            base::Array<base::RefPtr<ISceneEditMode>> m_allEditModes;
            base::RefPtr<ISceneEditMode> m_activeEditMode;

            base::HashSet<ContentNodePtr> m_selection;

            //-

            // BrowserTab
            virtual bool handleExternalCloseRequest() override;
            virtual base::StringBuf title() const override;
            virtual base::StringBuf tabCaption() const override;
            virtual ui::TooltipPtr tabTooltip() const override;
            virtual bool canCloseTab() const override final;
            virtual bool canDetach() const override final;
            virtual base::image::ImagePtr tabIcon() const override;
            virtual bool handleTabContextMenu(const ui::ElementArea& area, const ui::Position& absolutePosition) override;

            // EditorTab
            virtual bool isEditingFile(const depot::ManagedFilePtr& file) const override;
            virtual void showFile(const depot::ManagedFilePtr& file) const override;

            // IConfigUser
            virtual void onSaveConfiguration() override;

            //--

            void initializeActions();
            void initializePanels();
            void initializeGridTables();
            void initializePreviewPanel();
            void initializeEditModes();
            
            void treeSelectionChanged(const base::Array<ContentElementPtr>& elements);
            void treeFocusOnItem(const ContentElementPtr& elem);

            void fillAreaSelectionModes();
            void fillPointSelectionModes();
            void fillGizmoSpaceModes();
            void fillGizmoTargetModes();
            void fillPositionGridSizes();
            void fillRotationGridSizes();

            void restoreSceneConfig();
            void updateGridSetup();
            void internalTick(float dt);

            void postInternalConfiguration(base::ClassType classType, const void* data);

            template< typename T >
            INLINE void postInternalConfiguration(const T& data) { postInternalConfiguration(T::GetStaticClass(), &data); }

            //--

            void cmdSave();
            void cmdUndo();
            void cmdRedo();
            bool canUndo() const;
            bool canRedo() const;
            bool canSave() const;

            void cmdCopy();
            void cmdCut();
            void cmdPaste();
            void cmdDelete();
            bool canCopy() const;
            bool canCut() const;
            bool canPaste() const;
            bool canDelete() const;

            void cmdToggleTransparentSelection(const ui::ElementPtr& ptr);
            void cmdToggleWholePrefabSelection(const ui::ElementPtr& ptr);

            void cmdSelectTranslationGizmo(const ui::ElementPtr& owner);
            void cmdSelectRotationGizmo(const ui::ElementPtr& owner);
            void cmdSelectScaleGizmo(const ui::ElementPtr& owner);
            void cmdCycleGizmo(const ui::ElementPtr& owner);

            void cmdToggleFilterX(const ui::ElementPtr& owner);
            void cmdToggleFilterY(const ui::ElementPtr& owner);
            void cmdToggleFilterZ(const ui::ElementPtr& owner);

            void cmdSelectGizmoSpace(const ui::ElementPtr& owner);

            void cmdTogglePositionGrid();
            void cmdToggleRotationGrid();

            void cmdActivateEditMode(int index);

            //--

            friend class SceneRenderingPanel;
        };

    } // world
} // ed
