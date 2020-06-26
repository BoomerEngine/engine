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

    // a basic runtime component
    class SCENE_COMMON_API Component : public Element
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Component, Element);

    public:
        Component();
        virtual ~Component();

        /// get entity that this component belongs to
        INLINE Entity* entity() const { return m_entity; }

        //--

        // update component, called during entity update
        void update(float dt);

        // handle component update
        virtual void handleUpdate(float dt);

    protected:
        Entity* m_entity;

        friend class Entity;
    };

    //---

} // scene
