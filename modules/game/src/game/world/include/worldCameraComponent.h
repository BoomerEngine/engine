/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity\components\camera #]
*
***/

#pragma once

#include "worldComponent.h"

namespace game
{
    //--

    // game camera parameters
    struct GAME_WORLD_API CameraParameters
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(CameraParameters);

        base::AbsolutePosition position;
        base::Quat rotation;
        float fov = 90.0f;

        float forceNearPlane = 0.0f; // 0=use global settings
        float forceFarPlane = 0.0f; // 0=use global settings
    };

    //--

    // a scene component that is usable as a camera
    class GAME_WORLD_API CameraComponent : public Component
    {
        RTTI_DECLARE_VIRTUAL_CLASS(CameraComponent, Component);

    public:
        CameraComponent();
        virtual ~CameraComponent();

        ///--

        /// camera FOV
        INLINE float fov() const { return m_fov; }

        /// camera custom near plane
        INLINE float nearPlane () const { return m_forceNearPlane; }

        /// camera custom far plane
        INLINE float farPlane() const { return m_forceFarPlane; }


        ///--

        /// activate this camera component (will be pushed to the camera stack)
        /// NOTE: does not work if we are not attached to world
        void activate(bool forcePush = true);

        /// deactivate this camera component (will be removed from stack)
        void deactivate();

        ///--

        // set camera fov
        void fov(float value);

        // set camera custom near plane
        void nearPlane(float value);

        // set camera custom far plane
        void farPlane(float value);

        ///---

        /// compute camera parameters to be used by rendering
        virtual void computeCameraParams(CameraParameters& outParams) const;

    protected:
        float m_forceNearPlane = 0.0f;
        float m_forceFarPlane = 0.0f;
        float m_fov = 90.0f;
        bool m_active = false;

        //--

        // Component
        virtual void handleAttach(World* scene) override;
        virtual void handleDetach(World* scene) override;
    };

    //--

} // game
