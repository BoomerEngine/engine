/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entities #]
*
***/

#include "build.h"
#include "cameraEntity.h"

BEGIN_BOOMER_NAMESPACE(game)

//--

RTTI_BEGIN_TYPE_CLASS(CameraEntity);
    RTTI_CATEGORY("Camera");
    RTTI_PROPERTY(m_forceNearPlane).editable();
    RTTI_PROPERTY(m_forceFarPlane).editable();
    RTTI_PROPERTY(m_fov).editable();
RTTI_END_TYPE();

//--

CameraEntity::CameraEntity()
{}

CameraEntity::~CameraEntity()
{}

/*void CameraComponent::render(const CameraRenderingContext& context, const CameraRenderingTarget& target)
{
    if (auto* world = this->world())
    {

    }

    *outParams.position = absoluteTransform().position();
    outParams.rotation = absoluteTransform().rotation();
    outParams.fov = m_fov;
    outParams.forceNearPlane = m_forceNearPlane;
    outParams.forceFarPlane = m_forceFarPlane;
}*/

///--

void CameraEntity::fov(float value)
{
    m_fov = std::clamp<float>(value, 1.0f, 179.0f);
}

void CameraEntity::nearPlane(float value)
{
    m_forceNearPlane = std::clamp<float>(value, 0.0001f, 100000.0f);
}

void CameraEntity::farPlane(float value)
{
    m_forceFarPlane = std::clamp<float>(value, 0.0001f, 100000.0f);
}

//--
        
END_BOOMER_NAMESPACE(game)