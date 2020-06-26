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

namespace scene
{
    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(Component);
    RTTI_END_TYPE();

    Component::Component()
        : m_entity(nullptr)
    {}

    Component::~Component()
    {}

    void Component::update(float dt)
    {
        handleUpdate(dt);
    }

    void Component::handleUpdate(float dt)
    {
        // TODO
    }

    //--

} // scene