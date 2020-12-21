/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: components\camera #]
*
***/

#pragma once

#include "base/world/include/worldComponent.h"

namespace rendering
{
	/*
    //--

    struct RENDERING_WORLD_API CameraRenderingContext
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(CameraRenderingContext);

        rendering::command::CommandWriter* commandWriter = nullptr;

        CameraRenderingContext();
    };    

    struct RENDERING_WORLD_API CameraRenderingTarget
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(CameraRenderingTarget);

        uint32_t width = 0;
        uint32_t height = 0;
        rendering::ImageView backBufferColor;
        rendering::ImageView backBufferDepth;

        CameraRenderingTarget();
    };*/

    //--

    // a scene component that is usable as a camera
    class RENDERING_WORLD_API CameraComponent : public base::world::Component
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CameraComponent, base::world::Component);

    public:
        CameraComponent();
        virtual ~CameraComponent();

        ///--

        /// camera FOV
        INLINE float fov() const { return m_fov; }

        /// camera custom near plane
        INLINE float nearPlane() const { return m_forceNearPlane; }

        /// camera custom far plane
        INLINE float farPlane() const { return m_forceFarPlane; }

        ///--

        // set camera fov
        void fov(float value);

        // set camera custom near plane
        void nearPlane(float value);

        // set camera custom far plane
        void farPlane(float value);

        ///---

        /// render world from this camera
        //void render(const CameraRenderingContext& context, const CameraRenderingTarget& target);

        ///---

    protected:
        float m_forceNearPlane = 0.0f;
        float m_forceFarPlane = 0.0f;
        float m_fov = 90.0f;
    };

    //--

} // rendering
