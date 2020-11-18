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
#include "rendering/device/include/renderingShaderLibrary.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingFramebuffer.h"

namespace game
{
    //---

    static base::res::StaticResource<rendering::ShaderLibrary> resShaderSplash("/engine/shaders/effects/loading_splash_cube.fx");

    //---

    RTTI_BEGIN_TYPE_CLASS(Screen_Splash);
        RTTI_PROPERTY(m_shader);
    RTTI_END_TYPE();

    Screen_Splash::Screen_Splash()
    {
        m_effectTime = 0.0f;
        m_shader = resShaderSplash.loadAndGetAsRef();
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
        struct Constants
        {
            float screenSizeX = 0;
            float screenSizeY = 0;
            float screenInvSizeX = 0;
            float screenInvSizeY = 0;
            float time = 0.0f;
            float fade = 1.0f;
        };

        rendering::ConstantsView consts;
        //ImageView source;
    };

    void Screen_Splash::handleRender(IGame* game, rendering::command::CommandWriter& cmd, const HostViewport& hostViewport)
    {
        rendering::command::CommandWriterBlock block(cmd, "EffectSplash");

        // TODO: compute based blit ? tests show that PS if still faster in 2019

        if (auto shader = resShaderSplash.loadAndGet())
        {
            rendering::FrameBuffer fb;
            fb.color[0].rt = hostViewport.backBufferColor;
            fb.color[0].loadOp = rendering::LoadOp::DontCare;
            fb.color[0].storeOp = rendering::StoreOp::Store;

            rendering::FrameBufferViewportState viewport;
            viewport.minDepthRange = 0.0f;
            viewport.maxDepthRange = 1.0f;
            viewport.viewportRect = base::Rect(0, 0, hostViewport.width, hostViewport.height);

            cmd.opBeingPass(fb, 1, &viewport);

            SpashScreenParams::Constants data;
            data.screenSizeX = hostViewport.width;
            data.screenSizeY = hostViewport.height;
            data.screenInvSizeX = 1.0f / hostViewport.width;
            data.screenInvSizeY = 1.0f / hostViewport.height;
            data.fade = hostViewport.fadeLevel;
            data.time = m_effectTime;

            SpashScreenParams params;
            params.consts = cmd.opUploadConstants(data);
            //params.source = source;
            cmd.opBindParametersInline("SpashScreenParams"_id, params);

            cmd.opSetPrimitiveType(rendering::PrimitiveTopology::TriangleStrip);
            cmd.opDraw(shader, 0, 4);

            cmd.opEndPass();
        }
    }

    
    //--

} // game


