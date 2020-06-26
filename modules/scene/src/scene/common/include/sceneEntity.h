/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
*
***/

#pragma once

#include "sceneElement.h"

namespace scene
{

    //---

    // a basic element of runtime element hierarchy, elements can be linked together via runtime attachments to form complex arrangements
    // NOTE: this has 2 main derived classes: Component and Entity
    class SCENE_COMMON_API Entity : public Element
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Entity, Element);

    public:
        Entity();
        virtual ~Entity();

        /// get list of components in this entity
        INLINE const base::Array<ComponentPtr>& components() const { return m_components; }

        //--

        /// attach component to entity
        void attachComponent(const ComponentPtr& comp);

        /// detach component from entity
        void dettachComponent(const ComponentPtr& comp);

        //--

        /// update this entity
        void update(float dt);

        /// handle update of the entity
        virtual void handlePreComponentUpdate(float dt);

        /// handle update of the entity
        virtual void handlePostComponentUpdate(float dt);

        /// calculate required streaming distance for the element
        virtual float calculateRequiredStreamingDistance() const;

    protected:
        /// handle attachment to scene
        virtual void handleSceneAttach(Scene* scene) override final;

        /// handle detachment from scene
        virtual void handleSceneDetach(Scene* scene) override final;

    private:
        base::Array<ComponentPtr> m_components;

        base::Array<Component*> m_componentsAddedDuringUpdate;
        base::Array<Component*> m_componentsRemovedDuringUpdate;
        bool m_componentsBeingUpdated;
    };

    //---

    /// static content entity - the simplest entity of all
    /// NOTE: this entity MAY BE REMOVED/merged by content processing tools
    class SCENE_COMMON_API StaticEntity : public Entity
    {
        RTTI_DECLARE_VIRTUAL_CLASS(StaticEntity, Entity);

    public:
        StaticEntity();
    };

    ///---

} // scene

