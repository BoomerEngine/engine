/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: helpers #]
***/

#include "build.h"

#include "sceneEditorTempObjects.h"
#include "rendering/scene/include/renderingFrameScreenCanvas.h"
#include "scene/common/include/sceneNodeTemplate.h"

namespace ed
{
    namespace world
    {
        ///---

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISceneTempObject);
        RTTI_END_TYPE();

        ISceneTempObject::ISceneTempObject()
            : m_owner(nullptr)
            , m_selected(false)
        {}

        ISceneTempObject::~ISceneTempObject()
        {
            ASSERT_EX(m_owner == nullptr, "Temporary scene object still attached to scene when destroyed");
        }

        void ISceneTempObject::placement(const scene::NodeTemplatePlacement& placement)
        {
            m_placement = placement;
        }

        void ISceneTempObject::selection(bool selected)
        {
            m_selected = selected;
        }

        void ISceneTempObject::handleTick(float dt)
        {
        }

        void ISceneTempObject::handleRendering(rendering::scene::FrameInfo& frame)
        {
        }

        void ISceneTempObject::attachToScene(SceneTempObjectSystem& owner)
        {
            ASSERT(m_owner == nullptr);
            m_owner = &owner;
            m_owner->handleObjectAttached(sharedFromThis());
        }

        void ISceneTempObject::deattachFromScene()
        {
            ASSERT(m_owner != nullptr);
            m_owner->handleObjectDetached(sharedFromThis());
            m_owner = nullptr;
        }

        ///---

        RTTI_BEGIN_TYPE_CLASS(SceneTempObjectNode);
        RTTI_END_TYPE();

        SceneTempObjectNode::SceneTempObjectNode(const scene::NodeTemplatePtr& nodeTemplate)
            : m_nodeTemplate(nodeTemplate)
        {
            m_placement = nodeTemplate->placement();
        }

        SceneTempObjectNode::~SceneTempObjectNode()
        {}

        void SceneTempObjectNode::placement(const scene::NodeTemplatePlacement& placement)
        {
            m_nodeTemplate->placement(placement);
            m_placement = placement;
        }

        void SceneTempObjectNode::selection(bool selected)
        {
            TBaseClass::selection(selected);
        }

        void SceneTempObjectNode::handleTick(float dt)
        {
            TBaseClass::handleTick(dt);
        }

        void SceneTempObjectNode::handleRendering(rendering::scene::FrameInfo& frame)
        {
            TBaseClass::handleRendering(frame); 

            {
                rendering::scene::ScreenCanvas dd(frame);

                base::Vector3 screenPos;
                if (dd.calcScreenPosition(m_placement.T, screenPos))
                {
                    dd.lineColor(base::Color::LIGHTGREEN);
                    dd.alignHorizontal(0);
                    dd.alignVertical(0);
                    dd.textBox(screenPos.x, screenPos.y, base::TempString("Loading..."));
                }
            }
        }

        void SceneTempObjectNode::attachToScene(SceneTempObjectSystem& owner)
        {
            TBaseClass::attachToScene(owner);
        }

        void SceneTempObjectNode::deattachFromScene()
        {
            TBaseClass::deattachFromScene();
        }

        ///---

        SceneTempObjectSystem::SceneTempObjectSystem(const scene::ScenePtr& scene)
            : m_scene(scene)
            , m_updating(false)
            , m_objectsRemovedWhileUpdating(false)
        {
        }

        SceneTempObjectSystem::~SceneTempObjectSystem()
        {
            ASSERT_EX(m_objects.empty(), "Destroying temp scene with objects still attached to it");
        }
        
        void SceneTempObjectSystem::handleObjectAttached(const base::RefPtr< ISceneTempObject >& obj)
        {
            if (obj)
                m_objects.pushBack(obj);
        }

        void SceneTempObjectSystem::handleObjectDetached(const base::RefPtr< ISceneTempObject >& obj)
        {
            if (obj)
            {
                auto index = m_objects.find(obj);
                ASSERT(index != INDEX_NONE);

                if (m_updating)
                {
                    m_objects[index] = nullptr;
                    m_objectsRemovedWhileUpdating = true;
                }
                else
                {
                    m_objects.eraseUnordered(index);
                }
            }
        }

        void SceneTempObjectSystem::tick(float dt)
        {
            ASSERT(!m_updating);

            m_updating = true;
            m_objectsRemovedWhileUpdating = false;

            for (auto& obj : m_objects)
                if (obj)
                    obj->handleTick(dt);

            m_updating = false;

            if (m_objectsRemovedWhileUpdating)
            {
                m_objects.removeUnordered(nullptr);
                m_objectsRemovedWhileUpdating = false;
            }
        }

        void SceneTempObjectSystem::render(rendering::scene::FrameInfo& frame)
        {
            for (auto& obj : m_objects)
                obj->handleRendering(frame);
        }

        ///--

    } // mesh
} // ed