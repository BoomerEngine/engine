/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: components\camera #]
*
***/

#include "build.h"
#include "renderingCameraComponent.h"

namespace rendering
{
    //--

    /*RTTI_BEGIN_TYPE_CLASS(CameraRenderingContext);
        RTTI_BIND_NATIVE_COPY(CameraRenderingContext);
        RTTI_BIND_NATIVE_CTOR_DTOR(CameraRenderingContext);
    RTTI_END_TYPE();

    CameraRenderingContext::CameraRenderingContext()
    {}

    //--

    RTTI_BEGIN_TYPE_CLASS(CameraRenderingTarget);
        RTTI_BIND_NATIVE_COPY(CameraRenderingTarget);
        RTTI_BIND_NATIVE_CTOR_DTOR(CameraRenderingTarget);
        RTTI_PROPERTY(width);
        RTTI_PROPERTY(height);
        //rendering::ImageView backBufferColor;
        //rendering::ImageView backBufferDepth;
    RTTI_END_TYPE();

    CameraRenderingTarget::CameraRenderingTarget()
    {}*/

    //--

    RTTI_BEGIN_TYPE_CLASS(CameraComponent);
        RTTI_CATEGORY("Camera");
        RTTI_PROPERTY(m_forceNearPlane).editable();
        RTTI_PROPERTY(m_forceFarPlane).editable();
        RTTI_PROPERTY(m_fov).editable();
    RTTI_END_TYPE();

    //--

    CameraComponent::CameraComponent()
    {}

    CameraComponent::~CameraComponent()
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

    //--
        
} // game