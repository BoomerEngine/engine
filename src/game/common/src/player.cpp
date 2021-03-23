/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "screen.h"
#include "screenStack.h"
#include "player.h"
#include "playerEntity.h"

#include "gpu/device/include/commandWriter.h"
#include "engine/rendering/include/cameraContext.h"
#include "engine/world/include/worldEntity.h"
#include "engine/world/include/world.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGamePlayer);
RTTI_END_TYPE();

IGamePlayer::IGamePlayer(IGamePlayerIdentity* identity, IGamePlayerConnection* connection)
    : m_identity(AddRef(identity))
    , m_connection(AddRef(connection))
{
}

IGamePlayer::~IGamePlayer()
{
}

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGamePlayerLocal);
RTTI_END_TYPE();

IGamePlayerLocal::IGamePlayerLocal(IGamePlayerIdentity* identity, IGamePlayerConnection* connection)
    : IGamePlayer(identity, connection)
{
    m_cameraContext = RefNew<rendering::CameraContext>();
}

IGamePlayerLocal::~IGamePlayerLocal()
{
    m_cameraContext.reset();
}

bool IGamePlayerLocal::handleInput(IGameScreen* screen, const InputEvent& evt)
{
    if (auto ent = entity())
        return ent->handleInput(evt);

    return false;
}

void IGamePlayerLocal::handleRender(IGameScreen* screen, gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, float visibility)
{
    if (auto ent = entity())
    {
        if (auto world = ent->world())
        {
            WorldRenderingContext ctx;

            ctx.cameraContext = m_cameraContext;

            IWorldViewEntity* view = ent;
            view->evaluateCamera(ctx.cameraSetup);

            world->renderViewport(cmd, output, ctx);
        }
    }
}

//--
    
END_BOOMER_NAMESPACE()
