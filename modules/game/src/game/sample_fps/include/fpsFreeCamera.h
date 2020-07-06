/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: fps #]
***/

#pragma once
#include "game/world/include/worldEntity.h"
#include "game/world/include/worldInputComponent.h"
#include "game/world/include/worldCameraComponent.h"

namespace fps
{

    //--

    /// a simple fly around camera for FPS game
    class FreeCameraEntity : public game::Entity
    {
        RTTI_DECLARE_VIRTUAL_CLASS(FreeCameraEntity, game::Entity);

    public:
        FreeCameraEntity();
        virtual ~FreeCameraEntity();

        void place(const base::Vector3& initialPos, const base::Angles& initialRotation);

        void activate();
        void deactivate();

    private:
        float m_buttonForwardBackward = 0.0f;
        float m_buttonLeftRight = 0.0f;
        float m_buttonUpDown = 0.0f;
        float m_mouseDeltaX = 0.0f;
        float m_mouseDeltaY = 0.0f;

        base::Angles m_rotation;

        base::RefPtr<game::InputComponent> m_input;
        base::RefPtr<game::CameraComponent> m_camera;

        virtual void handlePreTick(float dt) override;
        virtual void handleGameInput(const game::InputEventPtr& gameInput) override;

        void updateCamera(float dt);
    };

    //--

} // fps
