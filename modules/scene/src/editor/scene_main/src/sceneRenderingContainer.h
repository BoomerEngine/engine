/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: preview #]
***/

#pragma once

#include "sceneRenderingContainer.h"
#include "ui/viewport/include/uiPreviewRenderingPanel.h"
#include "ui/widgets/include/uiDockPanel.h"
#include "editor/asset_browser/include/editorConfig.h"

namespace ed
{
    namespace world
    {

        /// base container for scene rendering panel(s), manages scene and rendering panels to view it
        /// highly extendable via the "editor tools", should not be subclassed in different editors
        class EDITOR_SCENE_MAIN_API SceneRenderingContainer : public ui::DockPanel
        {
            RTTI_DECLARE_VIRTUAL_CLASS(SceneRenderingContainer, ui::DockPanel);

        public:
            SceneRenderingContainer(SceneEditorTab* editor, const ConfigPath& config);
            virtual ~SceneRenderingContainer();

            //--

            /// get the current camera position and rotation
            bool captureCameraPlacement(base::Vector3& outPosition, base::Angles& outRotation) const;

            /// update the selection settings
            void updateSelectionSettings(const SceneSelectionSettings& selectionSettings);

            /// focus all viewports on given bounds
            void focusOnBounds(const base::Box& worldBounds);

            //--

        protected:
            ConfigPath m_config;
            SceneEditorTab* m_editor;

            base::RefPtr<ui::IElement> m_panelContainer;

            base::Array<base::RefPtr<SceneRenderingPanelWrapper>> m_wrappers;
            base::Array<base::RefPtr<SceneRenderingPanel>> m_panels;

            //--

            void createPanels();
        };

        //--

    } // world
} // ed
