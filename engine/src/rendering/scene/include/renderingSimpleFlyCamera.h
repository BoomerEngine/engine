/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: camera #]
***/

#pragma once

#include "base/containers/include/refCounted.h"

namespace rendering
{
    namespace scene
    {

        //--

        /// a simple fly around camera 
        class RENDERING_SCENE_API FlyCamera : public base::IReferencable
        {
        public:
            FlyCamera();
            ~FlyCamera();

            void update(float dt);

            void place(const base::Vector3& initialPos, const base::Angles& initialRotation);

            void compute(rendering::scene::FrameParams& params) const;

            bool processRawInput(const base::input::BaseEvent& evt);

        private:
            float m_buttonForwardBackward = 0.0f;
            float m_buttonLeftRight = 0.0f;
            float m_buttonUpDown = 0.0f;
            float m_mouseDeltaX = 0.0f;
            float m_mouseDeltaY = 0.0f;

            base::Angles m_rotation;
            base::Vector3 m_position;

            CameraContextPtr m_context;
        };

        //--

    } // scene
} // rendering

