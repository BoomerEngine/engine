/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "gameHost.h"
#include "gameWorld.h"
#include "gameFreeCameraHelper.h"
#include "engine/canvas/include/canvas.h"
#include "core/input/include/inputStructures.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_CLASS(GameWorld);
RTTI_END_TYPE();

//--

GameWorld::GameWorld()
{
    m_freeCamera = new FreeCameraHelper();
}

GameWorld::~GameWorld()
{
    delete m_freeCamera;
}

void GameWorld::update(double dt)
{
    TBaseClass::update(dt);

    m_freeCamera->animate(dt);
}

void GameWorld::renderCanvas(canvas::Canvas& canvas)
{
    if (m_freeCameraEnabled)
    {
        const auto pos = m_freeCamera->position();

        canvas.debugPrint(canvas.width() - 20, canvas.height() - 60,
            TempString("Camera Position: [X={}, Y={}, Z={}]", Prec(pos.x, 2), Prec(pos.y, 2), Prec(pos.z, 2)),
            Color::WHITE, 16, font::FontAlignmentHorizontal::Right);
    }
}


bool GameWorld::processInputEvent(const input::BaseEvent& evt)
{
    if (auto keyEvent = evt.toKeyEvent())
    {
        if (keyEvent->pressed() && keyEvent->keyCode() == input::KeyCode::KEY_F11)
        {
            toggleFreeCamera(!m_freeCameraEnabled);
            return true;
        }
    }

    // pass to free camera
    if (m_freeCameraEnabled && m_freeCamera->processInput(evt))
        return true;

    // pass to input stack
    for (auto index : m_inputStack.indexRange().reversed())
    {
        auto ent = m_inputStack[index];
        if (ent->handleInput(evt))
        {
            // TODO: remember entity that got the "press"
            return true;
        }
    }

    return false;
}

void GameWorld::pushInputEntity(Entity* ent)
{
    DEBUG_CHECK_RETURN_EX(ent, "Invalid entity");
    DEBUG_CHECK_RETURN_EX(m_inputStack.contains(ent), "Entity already on input stack");
    m_inputStack.pushBack(AddRef(ent));
}

void GameWorld::popInputEntity(Entity* ent)
{
    DEBUG_CHECK_RETURN_EX(ent, "Invalid entity");
    DEBUG_CHECK_RETURN_EX(!m_inputStack.contains(ent), "Entity not on input stack");
    m_inputStack.remove(ent);
}

void GameWorld::pushViewEntity(Entity* ent)
{
    DEBUG_CHECK_RETURN_EX(ent, "Invalid entity");
    DEBUG_CHECK_RETURN_EX(!m_viewStack.contains(ent), "Entity already on view stack");
    m_viewStack.pushBack(AddRef(ent));
}

void GameWorld::popViewEntity(Entity* ent)
{
    DEBUG_CHECK_RETURN_EX(ent, "Invalid entity");
    DEBUG_CHECK_RETURN_EX(m_viewStack.contains(ent), "Entity not on view stack");
    m_viewStack.remove(ent);
}

//--

void GameWorld::toggleFreeCamera(bool enabled, const Vector3* newPosition /*= nullptr*/, const Angles* newRotation /*= nullptr*/)
{
    if (enabled != m_freeCameraEnabled)
    {
        m_freeCameraEnabled = enabled;

        if (enabled)
            m_freeCamera->moveTo(m_lastViewPosition, m_lastViewRotation.toRotator());
        else
            m_freeCamera->resetInput();
    }

    if (newPosition || newRotation)
    {
        auto pos = newPosition ? *newPosition : m_freeCamera->position();
        auto rot = newRotation ? *newRotation : m_freeCamera->rotation();
        m_freeCamera->moveTo(pos, rot);
    }
}

bool GameWorld::calculateCamera(CameraSetup& outCamera)
{
    // use free camera if it's enabled or we don't have any entity on the stack
    if (m_freeCameraEnabled || m_viewStack.empty())
    {
        outCamera.position = m_freeCamera->position();
        outCamera.rotation = m_freeCamera->rotation().toQuat();
        return true;
    }

    // use top entity from the stack
    for (auto i : m_viewStack.indexRange().reversed())
    {
        const auto entity = m_viewStack[i];
        if (entity->handleCamera(outCamera))
            return true;
    }

    // no valid camera
    return false;
}

RefPtr<StreamingTask> GameWorld::createStreamingTask() const
{
    // collect streaming observers
    InplaceArray<StreamingObserverInfo, 4> observers;

    // use free camera as observer if enabled
    if (m_freeCameraEnabled)
    {
        auto& info = observers.emplaceBack();
        info.position = m_freeCamera->position();
    }

    // use each entity from the stack as the streaming observer
    for (const auto& ent : m_viewStack)
    {
        auto& info = observers.emplaceBack();
        info.position = ent->absoluteTransform().position().approximate();
    }

    // request streaming task
    auto streaming = system<StreamingSystem>();
    return streaming->createStreamingTask(observers);
}

void GameWorld::applyStreamingTask(const StreamingTask* task)
{
    auto streaming = system<StreamingSystem>();
    streaming->applyStreamingTask(task);
}

//--

END_BOOMER_NAMESPACE()



