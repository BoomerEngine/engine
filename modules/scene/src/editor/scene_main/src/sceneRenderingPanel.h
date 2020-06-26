/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: preview #]
***/

#pragma once

#include "ui/viewport/include/uiPreviewRenderingPanel.h"
#include "editor/asset_browser/include/editorConfig.h"

namespace ed
{
    namespace world
    {

        ///--

        /// specialized rendering panel with 3D scene
        class EDITOR_SCENE_MAIN_API SceneRenderingPanel : public ui::PreviewRenderingPanel, public IConfigUser
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneRenderingPanel, ui::PreviewRenderingPanel);

        public:
            SceneRenderingPanel(SceneEditorTab* editor, SceneRenderingContainer* sceneContainer, const ConfigPath& configPath);
            virtual ~SceneRenderingPanel();

            /// get the editor
            INLINE SceneEditorTab* editor() const { return m_sceneEditor; }

            /// get the scene
            /// NOTE: scene is not owned by the panel but by the container
            INLINE const base::RefPtr<scene::Scene>& scene() const { return m_scene; }

            /// get the scene container this panel belongs to
            INLINE SceneRenderingContainer* sceneContainer() const { return m_sceneContainer; }

            //--

            // show everything in the scene
            void showEverything();

        protected:
            // ui::RenderingPanel
            virtual void handleUpdate(float dt) override;
            virtual void handleRender(rendering::scene::FrameInfo& frame) override;
            virtual void handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables) override;
            virtual void handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables) override;

            // ui::IElement
            virtual bool handleMouseWheel(const base::input::MouseMovementEvent &evt, float delta) override;
            virtual bool handleMouseMovement(const base::input::MouseMovementEvent& evt) override;
            virtual bool handleKeyEvent(const base::input::KeyEvent &evt) override;
            virtual bool handleContextMenu(const ui::ElementArea &area, const ui::Position &absolutePosition) override;
            virtual ui::InputActionPtr handleMouseClick(const ui::ElementArea& area, const base::input::MouseClickEvent& evt) override;
            virtual ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;

            // IConfigurationEntry
            void restoreConfiguration();
            virtual void onSaveConfiguration();

            //--

            base::RefPtr<scene::Scene> m_scene;

            SceneEditorTab* m_sceneEditor;
            SceneRenderingContainer* m_sceneContainer;

            ConfigPath m_config;
            base::StringBuf m_name;
        };

        //--

        // wrapper for the scene panel, contains the toolbars
        class EDITOR_SCENE_MAIN_API SceneRenderingPanelWrapper : public ui::IElement
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneRenderingPanelWrapper, ui::IElement);

        public:
            SceneRenderingPanelWrapper(SceneEditorTab* editor, SceneRenderingContainer* sceneContainer, const ConfigPath& configPath);
            ~SceneRenderingPanelWrapper();

            /// get the rendering panel
            INLINE const base::RefPtr<SceneRenderingPanel>& panel() const { return m_panel; }

        private:
            base::RefPtr<SceneRenderingPanel> m_panel;

            SceneEditorTab* m_editor;
        };

        //--

    } // world
} // ed
