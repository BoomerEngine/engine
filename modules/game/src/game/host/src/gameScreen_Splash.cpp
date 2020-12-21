/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: screens #]
***/

#include "build.h"
#include "gameHost.h"
#include "gameScreen_Splash.h"

#include "rendering/texture/include/renderingTexture.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingFramebuffer.h"
#include "rendering/device/include/renderingShaderFile.h"
#include "rendering/device/include/renderingShader.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingShaderData.h"

namespace game
{
    //---

    static base::res::StaticResource<rendering::ShaderFile> resShaderSplash("/engine/shaders/effects/loading_splash_cube.fx");

    //---

    RTTI_BEGIN_TYPE_CLASS(Screen_Splash);
        RTTI_PROPERTY(m_shader);
    RTTI_END_TYPE();

    Screen_Splash::Screen_Splash()
    {
        m_effectTime = 0.0f;

        if (const auto data = resShaderSplash.loadAndGet())
			m_shader = AddRef(data->rootShader());
    }

    Screen_Splash::~Screen_Splash()
    {}

    bool Screen_Splash::supportsNativeFadeInFadeOut() const
    {
        return true;
    }

    void Screen_Splash::handleUpdate(IGame* game, double dt)
    {
        return TBaseClass::handleUpdate(game, dt);
    }

    struct SpashScreenParams
    {
       

        //rendering::ConstantsView consts;
        //ImageView source;
    };

    void Screen_Splash::handleRender(IGame* game, rendering::command::CommandWriter& cmd, const HostViewport& hostViewport)
    {
        rendering::command::CommandWriterBlock block(cmd, "EffectSplash");

        // TODO: compute based blit ? tests show that PS if still faster in 2019

        if (auto shader = resShaderSplash.loadAndGet())
        {
            rendering::FrameBuffer fb;
            fb.color[0].view(hostViewport.backBufferColor);
            fb.color[0].loadOp = rendering::LoadOp::DontCare;
            fb.color[0].storeOp = rendering::StoreOp::Store;
			fb.depth.view(hostViewport.backBufferDepth);

            base::Rect viewport = base::Rect(0, 0, hostViewport.width, hostViewport.height);

            cmd.opBeingPass(hostViewport.backBufferLayout, fb, 1, viewport);

			struct
			{
				float screenSizeX = 0;
				float screenSizeY = 0;
				float screenInvSizeX = 0;
				float screenInvSizeY = 0;
				float time = 0.0f;
				float fade = 1.0f;
			} data;

            data.screenSizeX = hostViewport.width;
            data.screenSizeY = hostViewport.height;
            data.screenInvSizeX = 1.0f / hostViewport.width;
            data.screenInvSizeY = 1.0f / hostViewport.height;
            data.fade = hostViewport.fadeLevel;
            data.time = m_effectTime;

			if (!m_shaderPSO)
				m_shaderPSO = m_shader->deviceShader()->createGraphicsPipeline(hostViewport.backBufferLayout);

			rendering::DescriptorEntry desc[1];
			desc[0].constants(data);
			cmd.opBindDescriptor("SpashScreenParams"_id, desc);

//            cmd.opSetPrimitiveType(rendering::PrimitiveTopology::TriangleStrip);
            cmd.opDraw(m_shaderPSO, 0, 4);

            cmd.opEndPass();
        }
    }

    
    //--

} // game


