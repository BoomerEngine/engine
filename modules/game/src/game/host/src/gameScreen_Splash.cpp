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
#include "rendering/driver/include/renderingShaderLibrary.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingFramebuffer.h"
#include "rendering/mesh/include/renderingMesh.h"

namespace game
{
    //---

    static base::res::StaticResource<rendering::ShaderLibrary> resShaderSplash("engine/shaders/effects/loading_splash_cube.fx");
    static base::res::StaticResource<rendering::Mesh> resTestMesh("engine/scene/bistro/exterior.obj");

    //---

    RTTI_BEGIN_TYPE_CLASS(Screen_Splash);
        RTTI_PROPERTY(m_shader);
    RTTI_END_TYPE();

    Screen_Splash::Screen_Splash()
    {
        m_fadeInTime = 2.0f;
        m_shader = resShaderSplash.loadAndGetAsRef();
    }

    Screen_Splash::~Screen_Splash()
    {}

    ScreenTransitionRequest Screen_Splash::handleUpdate(double dt)
    {
        m_effectTime += dt;

        {

        }
        
        return TBaseClass::handleUpdate(dt);
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

    void Screen_Splash::handleRender(rendering::command::CommandWriter& cmd, const HostViewport& hostViewport)
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
            data.fade = m_fade.m_fraction;
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

    bool Screen_Splash::handleInput(const base::input::BaseEvent& evt)
    {
        //if (m_effectTime > 3.0f)

        return false;
    }

    void Screen_Splash::handleAttach()
    {
        m_effectTime = 0.0f;
        TBaseClass::handleAttach(); // starts a fade in
    }

    void Screen_Splash::handleDetach()
    {

    }
    
    //--

} // game


