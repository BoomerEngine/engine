/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "screen.h"

#include "gpu/device/include/framebuffer.h"
#include "gpu/device/include/commandWriter.h"
#include "engine/canvas/include/canvas.h"
#include "engine/canvas/include/service.h"

BEGIN_BOOMER_NAMESPACE()

//---

ConfigProperty<float> cvGameScreenDefaultFadeInTime("GameScreen", "DefaultFadeInTime", 0.2f);
ConfigProperty<float> cvGameScreenDefaultFadeOutTime("GameScreen", "DefaultFadeOutTime", 0.2f);

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGameScreen);
RTTI_END_TYPE();

IGameScreen::IGameScreen()
{}

IGameScreen::~IGameScreen()
{}

//--

float IGameScreen::queryFadeInTime() const
{
    return cvGameScreenDefaultFadeInTime.get();
}

float IGameScreen::queryFadeOutTime() const
{
    return cvGameScreenDefaultFadeOutTime.get();
}

bool IGameScreen::queryOpaqueState() const
{
    return true;
}

bool IGameScreen::queryFilterAllInput() const
{
    return true;
}

bool IGameScreen::queryInputCaptureState() const
{
    return true;
}

//--

void IGameScreen::handleAttached()
{

}

void IGameScreen::handleDetached()
{

}

void IGameScreen::handleUpdate(double dt)
{

}

bool IGameScreen::handleInput(const InputEvent& evt)
{
    return false;
}

void IGameScreen::handleRenderImGuiDebugOverlay()
{

}

void IGameScreen::handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, float visibility)
{

}

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGameScreenCanvas);
RTTI_END_TYPE();

IGameScreenCanvas::IGameScreenCanvas()
{}

void IGameScreenCanvas::handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, float visibility)
{
    Canvas c(output.width, output.height);
    handleRender(c, visibility);

    gpu::FrameBuffer fb;
    fb.color[0].view(output.color);// .clear(0.2, 0.2f, 0.2f, 1.0f);
    fb.depth.view(output.depth).clearDepth().clearStencil();

    cmd.opBeingPass(fb);

    static auto* service = GetService<CanvasService>();
    service->render(cmd, c);

    cmd.opEndPass();
}

//--

END_BOOMER_NAMESPACE()
