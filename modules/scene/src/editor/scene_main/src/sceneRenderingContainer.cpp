/*
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: preview #]
***/

#include "build.h"
#include "sceneEditor.h"
#include "sceneRenderingPanel.h"
#include "sceneRenderingContainer.h"
#include "ui/toolkit/include/uiButton.h"
#include "ui/toolkit/include/uiStaticContent.h"

namespace ed
{
    namespace world
    {

        //---

        RTTI_BEGIN_TYPE_CLASS(SceneRenderingContainer);
        RTTI_END_TYPE();

        SceneRenderingContainer::SceneRenderingContainer(SceneEditorTab* editor, const ConfigPath& config)
            : ui::DockPanel("Scene", false, false)
            , m_editor(editor)
            , m_config(config)
        {
            layoutMode(ui::LayoutMode::Vertical);

            // create the static layout elements
            m_panelContainer = base::RefNew<ui::IElement>();
            m_panelContainer->layoutMode(ui::LayoutMode::Vertical);
            m_panelContainer->customProportion(1.0f);
            m_panelContainer->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            m_panelContainer->customVerticalAligment(ui::ElementVerticalLayout::Expand);
            attachChild(m_panelContainer);

            // create initial set of panels
            createPanels();
        }

        SceneRenderingContainer::~SceneRenderingContainer()
        {
        }

        void SceneRenderingContainer::focusOnBounds(const base::Box& worldBounds)
        {
            for (auto& ptr : m_panels)
            {
                ptr->setupCameraAroundBounds(worldBounds);
            }
        }

        void SceneRenderingContainer::updateSelectionSettings(const SceneSelectionSettings& selectionSettings)
        {
            for (auto& panel : m_panels)
            {
                panel->areaSelectionMode(selectionSettings.m_areaMode);
                panel->pointSelectionMode(selectionSettings.m_pointMode);
            }
        }

        bool SceneRenderingContainer::captureCameraPlacement(base::Vector3& outPosition, base::Angles& outRotation) const
        {
            if (auto panel = m_panels.front())
            {
                auto& camera = panel->cameraController();
                outPosition = camera.position();
                outRotation = camera.rotation();
                return true;
            }

            return false;
        }


        void SceneRenderingContainer::createPanels()
        {
            // detach all panels
            m_panelContainer->removeAllChildren();
            m_wrappers.reset();
            m_panels.reset();

            // create new panels
            // TODO: 2x2 splitter
            {
                auto wrapper = base::RefNew<SceneRenderingPanelWrapper>(m_editor, this, m_config["ScenePanel0"]);
                wrapper->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
                wrapper->customVerticalAligment(ui::ElementVerticalLayout::Expand);
                wrapper->customProportion(1.0f);
                m_panelContainer->attachChild(wrapper);

                m_wrappers.pushBack(wrapper);
                m_panels.pushBack(wrapper->panel());
            }
        }
    
        //--

        

        //---

    } // world
} // ed
