/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity\components #]
*
***/

#include "build.h"
#include "gameCameraComponent.h"
#include "world.h"
#include "worldCameraSystem.h"

namespace game
{
    //--

    RTTI_BEGIN_TYPE_CLASS(CameraParameters);
        RTTI_PROPERTY(position);
        RTTI_PROPERTY(rotation);
        RTTI_PROPERTY(fov);
        RTTI_PROPERTY(forceNearPlane);
        RTTI_PROPERTY(forceFarPlane);
    RTTI_END_TYPE();

    //--

    RTTI_BEGIN_TYPE_CLASS(CameraComponent);
        RTTI_PROPERTY(m_forceNearPlane).editable();
        RTTI_PROPERTY(m_forceFarPlane).editable();
        RTTI_PROPERTY(m_fov).editable();
        RTTI_PROPERTY(m_active).editable();
    RTTI_END_TYPE();

    //--

    CameraComponent::CameraComponent()
    {}

    CameraComponent::~CameraComponent()
    {}

    void CameraComponent::activate(bool forcePush /*= true*/)
    {
        if (!m_active || forcePush)
        {
            if (auto* cameraSystem = system<WorldCameraSystem>())
            {
                cameraSystem->pushCamera(this, forcePush);
                m_active = true;
            }
        }
    }

    void CameraComponent::deactivate()
    {
        if (m_active)
        {
            if (auto* cameraSystem = system<WorldCameraSystem>())
                cameraSystem->popCamera(this);
    
            m_active = false;
        }
    }

    ///--

    void CameraComponent::fov(float value)
    {
        m_fov = std::clamp<float>(value, 1.0f, 179.0f);
    }

    void CameraComponent::nearPlane(float value)
    {
        m_forceNearPlane = std::clamp<float>(value, 0.0001f, 100000.0f);
    }

    void CameraComponent::farPlane(float value)
    {
        m_forceFarPlane = std::clamp<float>(value, 0.0001f, 100000.0f);
    }

    //---

    void CameraComponent::handleAttach(World* scene)
    {
        TBaseClass::handleAttach(scene);

        // TODO: auto attach camera ? ee, bad idea probably

    }

    void CameraComponent::handleDetach(World* scene)
    {
        TBaseClass::handleDetach(scene);

        if (m_active)
        {
            if (auto* cameraSystem = scene->system<WorldCameraSystem>())
                cameraSystem->popCamera(this);
            m_active = false;
        }
    }

    void CameraComponent::computeCameraParams(CameraParameters& outParams) const
    {
        outParams.position = absoluteTransform().position();
        outParams.rotation = absoluteTransform().rotation();
        outParams.fov = m_fov;
        outParams.forceNearPlane = m_forceNearPlane;
        outParams.forceFarPlane = m_forceFarPlane;
    }

    //--
        
} // game