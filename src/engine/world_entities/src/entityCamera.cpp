/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#include "build.h"
#include "entityCamera.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_CLASS(CameraEntity);
    RTTI_CATEGORY("FOV");
    RTTI_PROPERTY(m_fov).editable().overriddable();
    RTTI_PROPERTY(m_forceFOV).editable().overriddable();
    RTTI_CATEGORY("Range");
    RTTI_PROPERTY(m_forceNearPlane).editable().overriddable();
    RTTI_PROPERTY(m_forceFarPlane).editable().overriddable();
    RTTI_PROPERTY(m_nearPlaneDistance).editable().overriddable();
    RTTI_PROPERTY(m_farPlaneDistance).editable().overriddable();
    RTTI_CATEGORY("Effects");
RTTI_END_TYPE();

//--

CameraEntity::CameraEntity()
{}

CameraEntity::~CameraEntity()
{}

void CameraEntity::evaluateCamera(CameraSetup& outSetup) const
{
    static const float MIN_PLANE = 0.00001f;
    static const float MAX_PLANE = 100000.0f;

    outSetup.position = cachedWorldTransform().T;
    outSetup.rotation = cachedWorldTransform().R;

    if (m_forceFOV)
        outSetup.fov = std::clamp<float>(m_fov, 1.0f, 179.0f);

    if (m_forceNearPlane)
        outSetup.nearPlane = std::clamp<float>(m_nearPlaneDistance, MIN_PLANE, MAX_PLANE);

    if (m_forceFarPlane)
        outSetup.farPlane = std::clamp<float>(m_forceFarPlane, outSetup.nearPlane + 1.0f, MAX_PLANE);
}

//--

END_BOOMER_NAMESPACE()
