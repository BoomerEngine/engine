/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: helpers #]
***/

#include "build.h"
#include "cameraController.h"

#include "base/input/include/inputStructures.h"
#include "base/app/include/localServiceContainer.h"
#include "base/ui/include/uiInputAction.h"
#include "base/ui/include/uiElement.h"

#include "rendering/scene/include/renderingFrameCamera.h"

namespace ui
{
    //----

    namespace config
    {
        base::ConfigProperty<float> cvCameraNormalSpeed("Editor.Camera", "DefaultSpeed", 2.0f);
        base::ConfigProperty<float> cvCameraFastMultiplier("Editor.Camera", "FastCameraSpeedMultiplier", 5.0f);
        base::ConfigProperty<float> cvCameraSlowMultiplier("Editor.Camera", "SlowCameraSpeedMultiplier", 0.1f);
        base::ConfigProperty<float> cvCameraMaxAcceleration("Editor.Camera", "MaxAcceleration", 50.0f);
        base::ConfigProperty<float> cvCameraMaxDeacceleration("Editor.Camera", "MaxDeacceleration", 50.0f);
    }

    //---

    CameraController::CameraController()
        : m_currentPosition(0,0,0)
        , m_currentRotation(0,0,0)
        , m_internalLinearVelocity(0,0,0)
        , m_internalAngularVelocity(0,0,0)
        , m_speedFactor(0.0f)
        , m_stamp(0)
    {
        resetInput();
    }

    void CameraController::invalidateCameraStamp()
    {
        m_stamp += 1;
    }

    void CameraController::resetInput()
    {
    }

    void CameraController::speedFactor(float speedFactor)
    {
        m_speedFactor = std::clamp(speedFactor, -2.0f, 2.0f);
    }

    void CameraController::animate(float timeDelta)
    {
    }

    bool CameraController::processKeyEvent(const base::input::KeyEvent& evt)
    {
        return false;
    }

    namespace helper
    {
        enum MovementKey
        {
            KEY_FOWARD,
            KEY_BACKWARD,
            KEY_LEFT,
            KEY_RIGHT,
            KEY_UP,
            KEY_DOWN,
            KEY_FAST,
            KEY_SLOW,

            KEY_MAX,
        };

        bool MapMovementKey(const base::input::KeyCode keyCode, MovementKey& outMovementKey)
        {
            switch (keyCode)
            {
                case base::input::KeyCode::KEY_W: outMovementKey = KEY_FOWARD; return true;
                case base::input::KeyCode::KEY_S: outMovementKey = KEY_BACKWARD; return true;
                case base::input::KeyCode::KEY_A: outMovementKey = KEY_LEFT; return true;
                case base::input::KeyCode::KEY_D: outMovementKey = KEY_RIGHT; return true;
                case base::input::KeyCode::KEY_Q: outMovementKey = KEY_UP; return true;
                case base::input::KeyCode::KEY_E: outMovementKey = KEY_DOWN; return true;
                case base::input::KeyCode::KEY_LEFT_CTRL: outMovementKey = KEY_SLOW; return true;
                case base::input::KeyCode::KEY_LEFT_SHIFT: outMovementKey = KEY_FAST; return true;
                case base::input::KeyCode::KEY_RIGHT_CTRL: outMovementKey = KEY_SLOW; return true;
                case base::input::KeyCode::KEY_RIGHT_SHIFT: outMovementKey = KEY_FAST; return true;
            }

            return false;
        }

        /// mouse input handling for the free fly camera
        class MouseCameraControlFreeFly : public IInputAction
        {
        public:
            MouseCameraControlFreeFly(IElement* ptr, CameraController *owner, uint8_t mode)
                : IInputAction(ptr)
                , m_owner(owner)
                , m_mode(mode)
                , m_inputVelocity(0,0,0)
            {
                memzero(m_movementKeys, sizeof(m_movementKeys));
            }

            virtual void onUpdateRedrawPolicy(WindowRedrawPolicy& outRedrawPolicy) override final
            {
                outRedrawPolicy = WindowRedrawPolicy::ActiveOnly;
            }

            virtual InputActionResult onKeyEvent(const base::input::KeyEvent &evt) override final
            {
                MovementKey movementKey = (MovementKey)0;
                if (MapMovementKey(evt.keyCode(), movementKey))
                {
                    if (evt.pressed())
                        m_movementKeys[movementKey] = true;
                    else if (evt.released())
                        m_movementKeys[movementKey] = false;
                    return InputActionResult();
                }

                if (evt.keyCode() == base::input::KeyCode::KEY_ESCAPE && evt.pressed())
                    return nullptr;

                m_owner->processKeyEvent(evt);
                return InputActionResult();
            }

            virtual InputActionResult onMouseEvent(const base::input::MouseClickEvent &evt, const ElementWeakPtr &hoverStack) override final
            {
                // mutate mouse mode
                if (evt.leftClicked())
                    m_mode |= 1;
                else if (evt.rightClicked())
                    m_mode |= 2;
                else if (evt.midClicked())
                    m_mode |= 4;
                else if (evt.leftReleased())
                    m_mode &= ~1;
                else if (evt.rightReleased())
                    m_mode &= ~2;
                else if (evt.midReleased())
                    m_mode &= ~4;

                // exit mouse mode when all mouse buttons are released
                if (m_mode == 0)
                    return InputActionResult(nullptr);
        
                // continue
                return InputActionResult();
            }

            virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent &evt, const ElementWeakPtr&hoverStack) override final
            {
                base::Angles deltaRot(0, 0, 0);
                base::Vector3 deltaPos(0, 0, 0);

                // update mouse angles
                /*if (mode == MoveMode_Slide)
                {
                    // compute 3D velocity
                    base::Vector3 cameraDirForward;
                    owner->currentRotation().angleVectors(&cameraDirForward, nullptr, nullptr);

                    auto cameraSpeed = config::cvCameraNormalSpeed.get() * pow(10.0f, owner->speedFactor());
                    auto deltaDist = owner->currentDistanceToOrigin() * (-evt.delta().y * 0.001f * cameraSpeed);
                    deltaPos = cameraDirForward * deltaDist;
                }
                else */if (m_mode == MoveMode_Look)
                {
                    deltaRot.pitch = evt.delta().y * 0.25f;
                    deltaRot.yaw = evt.delta().x * 0.25f;

                    // compute 3D velocity
                    base::Vector3 cameraDirForward;
                    m_owner->rotation().someAngleVectors(&cameraDirForward, nullptr, nullptr);

                    deltaPos = cameraDirForward * evt.delta().z * 0.5f;
                }
                else if (m_mode == MoveMode_Pan)
                {
                    base::Vector3 cameraDirRight, cameraDirUp(0, 0, 1);
                    m_owner->rotation().someAngleVectors(nullptr, &cameraDirRight, nullptr);

                    float cameraSpeed = config::cvCameraNormalSpeed.get() * pow(10.0f, m_owner->speedFactor());
                    if (m_movementKeys[KEY_FAST]) cameraSpeed *= config::cvCameraFastMultiplier.get();
                    if (m_movementKeys[KEY_SLOW]) cameraSpeed *= config::cvCameraSlowMultiplier.get();

                    deltaPos = cameraDirRight * evt.delta().x * 0.02f * cameraSpeed;
                    deltaPos -= cameraDirUp * evt.delta().y * 0.02f * cameraSpeed;
                }

                // apply movement to camera controller
                m_owner->moveTo(m_owner->position() + deltaPos, m_owner->rotation() + deltaRot);

                // continue
                return InputActionResult();
            }

            virtual InputActionResult onUpdate(float dt) override
            {
                computeMovement(dt);
                return InputActionResult();
            }

            void computeMovement(float timeDelta)
            {
                // compute target velocity
                float cameraSpeed = config::cvCameraNormalSpeed.get() * pow(10.0f, m_owner->speedFactor());
                base::Vector3 targetVelocity(0, 0, 0);
                {
                    // aggregate movement
                    base::Vector3 cameraMoveLocalDir(0, 0, 0);
                    cameraMoveLocalDir.x += m_movementKeys[KEY_FOWARD] ? 1.0f : 0.0f;
                    cameraMoveLocalDir.x -= m_movementKeys[KEY_BACKWARD] ? 1.0f : 0.0f;
                    cameraMoveLocalDir.y += m_movementKeys[KEY_RIGHT] ? 1.0f : 0.0f;
                    cameraMoveLocalDir.y -= m_movementKeys[KEY_LEFT] ? 1.0f : 0.0f;
                    cameraMoveLocalDir.z += m_movementKeys[KEY_UP] ? 1.0f : 0.0f;
                    cameraMoveLocalDir.z -= m_movementKeys[KEY_DOWN] ? 1.0f : 0.0f;
                    if (!cameraMoveLocalDir.isZero())
                    {
                        // prevent faster than max movement when both strafing and moving forward (Quake style... :D)
                        cameraMoveLocalDir.normalize();

                        // scale by the speed
                        if (m_movementKeys[KEY_FAST]) cameraSpeed *= config::cvCameraFastMultiplier.get();
                        if (m_movementKeys[KEY_SLOW]) cameraSpeed *= config::cvCameraSlowMultiplier.get();

                        // compute 3D velocity
                        base::Vector3 cameraDirForward, cameraDirRight, cameraDirUp;
                        m_owner->rotation().angleVectors(cameraDirForward, cameraDirRight, cameraDirUp);

                        targetVelocity += cameraDirForward * cameraMoveLocalDir.x;
                        targetVelocity += cameraDirRight * cameraMoveLocalDir.y;
                        targetVelocity += cameraDirUp * cameraMoveLocalDir.z;
                        targetVelocity *= cameraSpeed;
                    }
                }

                // accelerate towards target velocity
                {
                    // calculate required velocity change
                    auto delta = targetVelocity - m_inputVelocity;

                    // limit the maximum velocity change
                    auto acc = targetVelocity.isZero() ? config::cvCameraMaxDeacceleration.get() : config::cvCameraMaxAcceleration.get();
                    m_inputVelocity += base::ClampLength(delta, acc * timeDelta * (cameraSpeed / config::cvCameraNormalSpeed.get()));
                }

                // move the camera
                auto deltaPos = m_inputVelocity * timeDelta;
                m_owner->moveTo(m_owner->position() + deltaPos, m_owner->rotation());
            }

        private:
            CameraController *m_owner;

            static const uint32_t MoveMode_Slide = 1;
            static const uint32_t MoveMode_Look = 2;
            static const uint32_t MoveMode_Pan = 4;

            base::Vector3 m_inputVelocity;

            base::Point m_startPoint;
            bool m_movementKeys[KEY_MAX];

            uint8_t m_mode;
        };

        //---

        /// mouse input handling for the orbit camera
        class MouseCameraControlOrbitAroundPoint : public IInputAction
        {
        public:
            MouseCameraControlOrbitAroundPoint(IElement* ptr, CameraController *owner, uint8_t mode, const base::AbsolutePosition& orbitCenter)
                : IInputAction(ptr), m_owner(owner), m_mode(mode)
            {
                m_orbitCenter = orbitCenter;
                m_distance = owner->position().distance(orbitCenter.approximate());

                auto dir = orbitCenter.approximate() - owner->position();
                m_rotation = dir.toRotator();

                auto targetRot = m_rotation.toQuat();
                auto currentRot = owner->rotation().toQuat();

                m_rotationOffset = targetRot.inverted() * currentRot;
            }

            virtual void onUpdateRedrawPolicy(WindowRedrawPolicy& outRedrawPolicy) override final
            {
                outRedrawPolicy = WindowRedrawPolicy::ActiveOnly;
            }

            virtual InputActionResult onKeyEvent(const base::input::KeyEvent &evt) override final
            {
                m_owner->processKeyEvent(evt);
                return InputActionResult();
            }

            /*virtual void onRender3D(rendering::scene::FrameParams& frame) override final
            {
                {
                    rendering::runtime::SolidGeometryBuilder solid;
                    solid.color(base::Color::LIGHTGREEN);
                    solid.addSphere(m_orbitCenter.approximate(), 0.1f);
                    frame.addGeometry(solid);
                }
            }*/

            virtual InputActionResult onMouseEvent(const base::input::MouseClickEvent &evt, const ElementWeakPtr&hoverStack) override final
            {
                // mutate mouse mode
                if (evt.leftClicked())
                    m_mode |= 1;
                else if (evt.rightClicked())
                    m_mode |= 2;
                else if (evt.midClicked())
                    m_mode |= 4;
                else if (evt.leftReleased())
                    m_mode &= ~1;
                else if (evt.rightReleased())
                    m_mode &= ~2;
                else if (evt.midReleased())
                    m_mode &= ~4;

                // exit mouse mode when all mouse buttons are released
                if (m_mode == 0)
                    return InputActionResult(nullptr);

                // continue
                return InputActionResult();
            }

            virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent &evt, const ElementWeakPtr&hoverStack) override final
            {
                // rotate local reference frame
                m_rotation.pitch += evt.delta().y * 0.25f;
                m_rotation.yaw += evt.delta().x * 0.25f;

                // calcualte position and rotation of the camera
                auto pos = m_orbitCenter.approximate() - m_distance * m_rotation.forward();
                auto rot = m_rotation.toQuat() * m_rotationOffset;
                auto cameraAngles = rot.toRotator();
                cameraAngles.roll = 0.0f;
                m_owner->moveTo(pos, cameraAngles);

                return InputActionResult();
            }

        private:
            CameraController *m_owner;
            base::AbsolutePosition m_orbitCenter;
            base::Angles m_rotation;
            base::Quat m_rotationOffset;

            float m_distance;
            uint8_t m_mode;
        };

        //--

    } // helper

    InputActionPtr CameraController::handleGeneralFly(IElement* ptr, uint8_t button)
    {
        return base::CreateSharedPtr<helper::MouseCameraControlFreeFly>(ptr, this, button);
    }

    InputActionPtr CameraController::handleOrbitAroundPoint(IElement* ptr, uint8_t button, const base::AbsolutePosition& orbitCenterPos)
    {
        return base::CreateSharedPtr<helper::MouseCameraControlOrbitAroundPoint>(ptr, this, button, orbitCenterPos);
    }

    void CameraController::moveTo(const base::Vector3& position, const base::Angles& rotation)
    {
        m_currentPosition = position;
        m_currentRotation = rotation;
        invalidateCameraStamp();
    }

    void CameraController::processMouseWheel(const base::input::MouseMovementEvent& evt, float delta)
    {
        auto dir = m_currentRotation.forward();
        m_currentPosition += dir * delta;
        invalidateCameraStamp();
    }

    void CameraController::computeRenderingCamera(rendering::scene::CameraSetup& outCamera) const
    {
        // setup position and rotation
        outCamera.position = m_currentPosition;
        outCamera.rotation = m_currentRotation.toQuat();

        // setup clipping planes
        // TODO: allow clipping planes to be configured by the user
        /*if (isIsometric())
        {
            outCamera.nearPlane = -1000.0f;
            outCamera.farPlane = 1000.0f;
            outCamera.zoom = m_currentZoom;
        }
        else*/
        {
            outCamera.nearPlane = 0.05f;
            outCamera.farPlane = 8000.0f;
            outCamera.zoom = 1.0f;
        }
    }

    //---

} // ui
