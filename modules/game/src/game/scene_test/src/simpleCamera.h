/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: camera #]
***/

#pragma once

#include "game/scene/include/gameEntity.h"

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
            FlyCameraEntity(base::Vector3 initialPos = base::Vector3(0,0,0), base::Angles initialRotation=base::Angles(0,0,0));
            virtual ~FlyCameraEntity();

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

