/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: node #]
***/

#include "build.h"
#include "editorNode.h"
#include "editorScene.h"

#include "game/world/include/world.h"
#include "game/world/include/worldCameraComponent.h"
#include "game/world/include/worldEntity.h"
#include "rendering/scene/include/renderingFrameParams.h"

namespace ed
{
    //---

    RTTI_BEGIN_TYPE_NATIVE_CLASS(EditorScene);
    RTTI_END_TYPE();

    EditorScene::EditorScene(bool fullScene/* = false*/)
    {
        m_world = CreateSharedPtr<game::World>(fullScene ? game::WorldType::Game : game::WorldType::Editor);

        m_cameraEntity = CreateSharedPtr<game::Entity>();
        auto camera = CreateSharedPtr<game::CameraComponent>();
        m_cameraEntity->attachComponent(camera);

        m_world->attachEntity(m_cameraEntity);

        camera->activate();
    }

    EditorScene::~EditorScene()
    {
        if (m_cameraEntity)
        {
            m_world->detachEntity(m_cameraEntity);
            m_cameraEntity.reset();
        }

        m_world.reset();
    }

    void EditorScene::update(float dt)
    {
        PC_SCOPE_LVL0(EditorSceneUpdate);

        DEBUG_CHECK_RETURN(!m_duringUpdate);

        m_duringUpdate = true;

        for (const auto& node : m_nodes)
            node->update(dt);

        m_world->update(dt);

        m_duringUpdate = false;
    }

    void EditorScene::render(rendering::scene::FrameParams& frame)
    {
        PC_SCOPE_LVL0(EditorSceneRender);

        AbsoluteTransform transform;
        transform.position(frame.camera.camera.position());
        transform.rotation(frame.camera.camera.rotation());
        m_cameraEntity->handleTransformUpdate(transform);

        m_world->prepareFrameCamera(frame);
        m_world->renderFrame(frame);

        for (const auto& node : m_nodes)
            node->render(frame);
    }

    void EditorScene::attachNode(EditorNode* node)
    {
        DEBUG_CHECK_RETURN(node);
        DEBUG_CHECK_RETURN(!m_duringUpdate);
        m_nodes.emplaceBack(AddRef(node));
    }

    void EditorScene::detachNode(EditorNode* node)
    {
        DEBUG_CHECK_RETURN(!m_duringUpdate);
        m_nodes.remove(node);
    }

    //--

} // ed