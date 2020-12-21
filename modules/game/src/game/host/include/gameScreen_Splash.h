/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: screens #]
***/

#pragma once

#include "gameScreen.h"

namespace game
{
    //--

    // simple splash screen that displays an image, image is displayed with on screen quad with material
    class Screen_Splash : public IScreen
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Screen_Splash, IScreen);

    public:
        Screen_Splash();
        virtual ~Screen_Splash();

    protected:
        virtual bool supportsNativeFadeInFadeOut() const override;
        virtual void handleUpdate(IGame* game, double dt) override;
        virtual void handleRender(IGame* game, rendering::command::CommandWriter& cmd, const HostViewport& viewport) override;

    private:
        rendering::TexturePtr m_texture;
        rendering::ShaderDataPtr m_shader;
		rendering::GraphicsPipelineObjectPtr m_shaderPSO;

        float m_effectTime = 0.0f;
    };
    
    //--

} // game
