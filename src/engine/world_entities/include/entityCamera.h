/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#pragma once

#include "engine/world/include/worldViewEntity.h"

BEGIN_BOOMER_NAMESPACE()

//--

// a scene component that is usable as a camera
class ENGINE_WORLD_ENTITIES_API CameraEntity : public IWorldViewEntity
{
    RTTI_DECLARE_VIRTUAL_CLASS(CameraEntity, IWorldViewEntity);

public:
    CameraEntity();
    virtual ~CameraEntity();

    ///---

    /// calculate camera parameters
    virtual void evaluateCamera(CameraSetup& outSetup) const override;

    ///---

protected:
    float m_nearPlaneDistance = 0.0f;
    float m_farPlaneDistance = 0.0f;
    bool m_forceNearPlane = false;
    bool m_forceFarPlane = false;

    float m_fov = 90.0f;
    bool m_forceFOV = false;
};

//--

END_BOOMER_NAMESPACE()
