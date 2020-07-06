/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: camera #]
***/

#pragma once

#include "game/world/include/worldEntity.h"

namespace game
{
    namespace test
    {

        //--

        /// a simple fly around camera 
        class FlyCameraEntity : public Entity
        {
            RTTI_DECLARE_VIRTUAL_CLASS(FlyCameraEntity, Entity);

        public:
            FlyCameraEntity();
            virtual ~FlyCameraEntity();

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

            base::RefPtr<InputComponent> m_input;
            base::RefPtr<CameraComponent> m_camera;

            virtual void handlePreTick(float dt) override;
            virtual void handleGameInput(const InputEventPtr& gameInput) override;

            void updateCamera(float dt);
        };

    } // test
} // game

