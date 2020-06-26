/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: preview #]
***/

#include "build.h"
#include "sceneEditor.h"
#include "sceneEditorStructure.h"
#include "sceneRenderingPanel.h"
#include "sceneRenderingContainer.h"

#include "ui/toolkit/include/uiWindowOverlay.h"
#include "ui/toolkit/include/uiStaticContent.h"

#include "scene/common/include/sceneRuntime.h"

#include "editor/asset_browser/include/renderingPanelToolbars.h"

namespace ed
{
    namespace world
    {
        //--

        RTTI_BEGIN_TYPE_CLASS(SceneRenderingPanel);
        RTTI_END_TYPE();

        SceneRenderingPanel::SceneRenderingPanel(SceneEditorTab* editor, SceneRenderingContainer* sceneContainer, const ConfigPath& configPath)
            : m_config(configPath)
            , m_sceneEditor(editor)
            , m_sceneContainer(sceneContainer)
            , m_scene(editor->scene())
        {
            // register
            m_sceneEditor->attachRenderingPanel(this);

            // restore camera postiion
            restoreConfiguration();
        }

        SceneRenderingPanel::~SceneRenderingPanel()
        {
            // unregister
            if (m_sceneEditor)
            {
                m_sceneEditor->detachRenderingPanel(this);
                m_sceneEditor = nullptr;
            }
        }

        void SceneRenderingPanel::showEverything()
        {
            base::Box sceneBounds;
            /*if (m_sceneEditor->content().get1calculateSceneBoundingBox(sceneBounds))
            {
                auto rotation = base::Angles(25.0f, 30.0f, 0.0f);
                setupCameraAroundBounds(sceneBounds, 1.2f, &rotation);
            }*/
        }

        void SceneRenderingPanel::onSaveConfiguration()
        {
            m_config.set("CameraMode", 0);
            m_config.set("CameraPosition", cameraController().position());
            m_config.set("CameraRotation", cameraController().rotation());
        }

        void SceneRenderingPanel::restoreConfiguration()
        {
            // try to restore camera
            bool cameraRestored = false;
            uint32_t mode = m_config.get<uint32_t>("CameraMode");
            if (mode == 0)
            {
                auto pos = m_config.get<base::Vector3>("CameraPosition");
                auto rot = m_config.get<base::Angles>("CameraRotation");
                setupCamera(rot, pos);
                cameraRestored = true;
            }

            // use default mode
            if (!cameraRestored)
                showEverything();
        }

        void SceneRenderingPanel::handleUpdate(float dt)
        {
            PC_SCOPE_LVL1(ResourceRenderingPanelUpdate);

            // update content observer
            {
                auto cameraPosition = cameraController().position();
            }

            // update the camera
            TBaseClass::handleUpdate(dt);
        }

        void SceneRenderingPanel::handleRender(rendering::scene::FrameInfo& frame)
        {
            TBaseClass::handleRender(frame);

            editor()->viewportRenderDebugFragments(this, frame);

            if (m_scene)
                m_scene->render(frame);
        }

        bool SceneRenderingPanel::handleContextMenu(const ui::ElementArea &area, const ui::Position &absolutePosition)
        {
            // TODO: determine object(s) under cursor
            base::Array<rendering::scene::Selectable> objects;

            // determine the world space position under cursor
            
            if (m_sceneEditor->viewportContextMenu(this, area, absolutePosition, objects, nullptr))
                return true;

            return TBaseClass::handleContextMenu(area, absolutePosition);
        }

        void SceneRenderingPanel::handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables)
        {
            m_sceneEditor->viewportSelectionChanged(this, ctrl, shift, selectables);
        }

        void SceneRenderingPanel::handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
        {
            m_sceneEditor->viewportSelectionChanged(this, ctrl, shift, selectables);
        }

        bool SceneRenderingPanel::handleMouseWheel(const base::input::MouseMovementEvent &evt, float delta)
        {
            if (m_sceneEditor->viewportMouseWheelEvent(this, evt, delta))
                return true;

            return TBaseClass::handleMouseWheel(evt, delta);
        }

        bool SceneRenderingPanel::handleKeyEvent(const base::input::KeyEvent &evt)
        {
            if (m_sceneEditor->viewportKeyEvent(this, evt))
                return true;

            return TBaseClass::handleKeyEvent(evt);
        }

        bool SceneRenderingPanel::handleMouseMovement(const base::input::MouseMovementEvent& evt)
        {
            if (editor()->viewportMouseMoveEvent(this, evt))
                return true;

            return TBaseClass::handleMouseMovement(evt);
        }

        ui::InputActionPtr SceneRenderingPanel::handleMouseClick(const ui::ElementArea& area, const base::input::MouseClickEvent& evt)
        {
            if (auto action = editor()->viewportMouseClickEvent(this, area, evt))
                return action;

            return TBaseClass::handleMouseClick(area, evt);
        }

        ui::DragDropHandlerPtr SceneRenderingPanel::handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
        {
            if (auto handler = editor()->viewportDragAndDrop(this, data, entryPosition))
                return handler;

            return TBaseClass::handleDragDrop(data, entryPosition);
        }

        //--

        RTTI_BEGIN_TYPE_CLASS(SceneRenderingPanelWrapper);
        RTTI_END_TYPE();

        SceneRenderingPanelWrapper::SceneRenderingPanelWrapper(SceneEditorTab* editor, SceneRenderingContainer* sceneContainer, const ConfigPath& configPath)
            : m_editor(editor)
        {
            layoutMode(ui::LayoutMode::Vertical);

            // create toolbar holder
            auto toolbars = base::CreateSharedPtr<ui::IElement>();
            toolbars->layoutMode(ui::LayoutMode::Horizontal);
            toolbars->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            attachBuildChild(toolbars);

            // create preview panel
            m_panel = base::CreateSharedPtr<SceneRenderingPanel>(editor, sceneContainer, configPath);
            m_panel->customProportion(1.0f);
            m_panel->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_panel->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            attachBuildChild(m_panel);

            //--

            // create general toolbar
            auto toolbar1 = base::CreateSharedPtr<RenderingPanelViewModeToolbar>(m_panel);
            toolbars->attachBuildChild(toolbar1);
            toolbar1->realize();

            // TODO: create scene specific toolbar

            // create camera specific toolbar
            auto separator = base::CreateSharedPtr<ui::IElement>();
            separator->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            separator->customProportion(1.0f);
            toolbars->attachBuildChild(separator);

            // camera speed control 
            auto toolbar2 = base::CreateSharedPtr<RenderingPanelViewModeToolbarCameraSpeedToolbar>(m_panel);
            toolbars->attachBuildChild(toolbar2);
            toolbar2->realize();

        }

        SceneRenderingPanelWrapper::~SceneRenderingPanelWrapper()
        {

        }

        //--

    } // world
} // ed
