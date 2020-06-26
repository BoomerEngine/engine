 /***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#pragma once

#include "renderingScenePanel.h"
#include "rendering/scene/include/renderingFrameParams.h"

namespace ui
{
    //--

    /// ui widget with a full 3D rendering scene (but not world, just rendering scene)
    class RENDERING_UI_API RenderingFullScenePanel : public RenderingScenePanel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(RenderingFullScenePanel, RenderingScenePanel);

    public:
        RenderingFullScenePanel();
        virtual ~RenderingFullScenePanel();

        //--

        INLINE rendering::scene::Scene* renderingScene() const { return m_scene; }

        //--

        // add default view mode buttons to given toolbar - filters, render mode, etc
        void buildDefaultToolbar(IElement* owner, ToolBar* toolbar);

    protected:
        virtual void handleUpdate(float dt) override;
        virtual void handleRender(rendering::scene::FrameParams& frame) override;

        /// inject render mode options into a popup menu
        virtual void buildRenderModePopup(MenuButtonContainer* menu);

        /// build filtering popup
        virtual void buildFilterPopup(MenuButtonContainer* menu);

    private:
        rendering::scene::ScenePtr m_scene;

        rendering::scene::FrameRenderMode m_renderMode;
        base::StringID m_renderMaterialDebugChannelName;

        void configure(rendering::scene::FrameRenderMode mode, base::StringID materialChannelName = base::StringID::EMPTY());

        void createFilterItem(base::StringView<char> prefix, const rendering::scene::FilterBitInfo* bitInfo, MenuButtonContainer* menu);
    };

    ///---

} // ui