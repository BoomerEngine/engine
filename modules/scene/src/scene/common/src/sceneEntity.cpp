/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#include "build.h"

#include "sceneComponent.h"
#include "sceneEntity.h"
#include "sceneEntityTickSystem.h"

namespace scene
{
    //--

    RTTI_BEGIN_TYPE_CLASS(Entity);
    RTTI_END_TYPE();

    Entity::Entity()
        : m_componentsBeingUpdated(false)
    {}

    Entity::~Entity()
    {}

    void Entity::attachComponent(const ComponentPtr& comp)
    {
        ASSERT(comp);
        ASSERT(!m_components.contains(comp));
        ASSERT(!comp->isAttached());

        ASSERT(!comp->m_entity);
        m_components.pushBack(comp);
        comp->m_entity = this;

        if (auto attachedToScene  = scene())
            comp->attachToScene(attachedToScene);
    }

    void Entity::dettachComponent(const ComponentPtr& comp)
    {
        ASSERT(comp);
        ASSERT(m_components.contains(comp));

        if (auto attachedToScene  = scene())
        {
            ASSERT(comp->scene() == attachedToScene);
            comp->detachFromScene();
        }

        ASSERT(this == comp->m_entity);
        m_components.remove(comp);
        comp->m_entity = nullptr;
    }

    void Entity::update(float dt)
    {
        handlePreComponentUpdate(dt);

        {
            ASSERT(!m_componentsBeingUpdated);
            m_componentsBeingUpdated = true;

            for (auto& comp : m_components)
                comp->update(dt);

            m_componentsBeingUpdated = false;


        }

        handlePostComponentUpdate(dt);
    }

    float Entity::calculateRequiredStreamingDistance() const
    {
        float dist = 0.0f;

        for (auto& comp : m_components)
            dist = std::max(dist, comp->calculateRequiredStreamingDistance());

        return dist;
    }

    void Entity::handlePreComponentUpdate(float dt)
    {

    }

    void Entity::handlePostComponentUpdate(float dt)
    {

    }

    void Entity::handleSceneAttach(Scene* scene)
    {
        TBaseClass::handleSceneAttach(scene);

        for (auto& comp : m_components)
            comp->attachToScene(scene);
    }

    void Entity::handleSceneDetach(Scene* scene)
    {
        TBaseClass::handleSceneDetach(scene);

        for (auto& comp : m_components)
            comp->detachFromScene();
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(StaticEntity);
    RTTI_END_TYPE();

    StaticEntity::StaticEntity()
    {}

    //---

} // scene
