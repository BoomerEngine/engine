/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: helpers #]
***/

#include "build.h"
#include "viewportCameraController.h"

#include "core/input/include/inputStructures.h"
#include "core/app/include/localServiceContainer.h"

#include "engine/ui/include/uiInputAction.h"
#include "engine/ui/include/uiElement.h"
#include "engine/ui/include/uiElementConfig.h"

#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/device.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//----

namespace config
{
    ConfigProperty<float> cvCameraNormalSpeed("Editor.Camera", "DefaultSpeed", 5.0f);
    ConfigProperty<float> cvCameraFastMultiplier("Editor.Camera", "FastCameraSpeedMultiplier", 5.0f);
    ConfigProperty<float> cvCameraSlowMultiplier("Editor.Camera", "SlowCameraSpeedMultiplier", 0.1f);
    ConfigProperty<float> cvCameraMaxAcceleration("Editor.Camera", "MaxAcceleration", 50.0f);
    ConfigProperty<float> cvCameraMaxDeacceleration("Editor.Camera", "MaxDeacceleration", 50.0f);
}

//---

RTTI_BEGIN_TYPE_ENUM(CameraMode);
    RTTI_ENUM_OPTION(FreePerspective);
    RTTI_ENUM_OPTION(OrbitPerspective);
    RTTI_ENUM_OPTION(FreeOrtho);
    RTTI_ENUM_OPTION(Front);
    RTTI_ENUM_OPTION(Back);
    RTTI_ENUM_OPTION(Left);
    RTTI_ENUM_OPTION(Right);
    RTTI_ENUM_OPTION(Top);
    RTTI_ENUM_OPTION(Bottom);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_CLASS(ViewportCameraControllerSettings);
    RTTI_PROPERTY(mode);
    RTTI_PROPERTY(orthoZoom);
    RTTI_PROPERTY(origin);
    RTTI_PROPERTY(position);
    RTTI_PROPERTY(rotation);
RTTI_END_TYPE();

ViewportCameraControllerSettings::ViewportCameraControllerSettings()
    : position(2, 0, 0)
    , origin(0, 0, 0)
    , rotation(0, 0, 0)
{}

bool ViewportCameraControllerSettings::perspective() const
{
    switch (mode)
    {
    case CameraMode::FreePerspective:
    case CameraMode::OrbitPerspective:
        return true;
    }

    return false;
}

bool ViewportCameraControllerSettings::ortho() const
{
    return !perspective();
}

bool ViewportCameraControllerSettings::fixed() const
{
    switch (mode)
    {
    case CameraMode::Top:
    case CameraMode::Bottom:
    case CameraMode::Left:
    case CameraMode::Right:
    case CameraMode::Front:
    case CameraMode::Back:
        return true;
    }

    return false;
}

float ViewportCameraControllerSettings::calcRelativeScreenSize(ui::Size viewportSize) const
{
    static auto ScreenSize = GetService<DeviceService>()->device()->maxRenderTargetSize().toVector();

    const auto screenSizePercX = viewportSize.x / ScreenSize.x;
    const auto screenSizePercY = viewportSize.y / ScreenSize.y;
    const auto screenSize = std::max<float>(screenSizePercX, screenSizePercY);

    return screenSize;
}

void ViewportCameraControllerSettings::computeRenderingCamera(ui::Size viewportSize, CameraSetup& outCamera) const
{
    outCamera.aspect = (viewportSize.y > 1.0f) ? (viewportSize.x / viewportSize.y) : 1.0f;

    if (perspective())
    {
        outCamera.position = position.approximate();
        outCamera.rotation = rotation.toQuat();
        outCamera.fov = std::clamp<float>(perspectiveFov, 1.0f, 179.0f);
        outCamera.zoom = 1.0f;
    }
    else
    {
        outCamera.fov = 0.0f;

        if (fixed())
        {
            outCamera.nearPlane = -8000.0f;
            outCamera.farPlane = 8000.0f;

            switch (mode)
            {
                case CameraMode::Top:
                {
                    outCamera.position = origin.approximate();
                    outCamera.position.z = 0.0f;
                    outCamera.rotation = Angles(90.0f, 0.0f, 0.0f).toQuat();
                    break;
                }   

                case CameraMode::Bottom:
                {
                    outCamera.position = origin.approximate();
                    outCamera.position.z = 0.0f;
                    outCamera.rotation = Angles(-90.0f, 0.0f, 0.0f).toQuat();
                    break;
                }

                case CameraMode::Back:
                {
                    outCamera.position = origin.approximate();
                    outCamera.position.x = 0.0f;
                    outCamera.rotation = Angles(0.0f, 0.0f, 0.0f).toQuat();
                    break;
                }

                case CameraMode::Front:
                {
                    outCamera.position = origin.approximate();
                    outCamera.position.x = 0.0f;
                    outCamera.rotation = Angles(0.0f, 180.0f, 0.0f).toQuat();
                    break;
                }

                case CameraMode::Left:
                {
                    outCamera.position = origin.approximate();
                    outCamera.position.y = 0.0f;
                    outCamera.rotation = Angles(0.0f, 90.0f, 0.0f).toQuat();
                    break;
                }

                case CameraMode::Right:
                {
                    outCamera.position = origin.approximate();
                    outCamera.position.y = 0.0f;
                    outCamera.rotation = Angles(0.0f, -90.0f, 0.0f).toQuat();
                    break;
                }
            }
        }
        else
        {
            outCamera.nearPlane = -1.0f;
            outCamera.farPlane = 8000.0f;
            outCamera.rotation = rotation.toQuat();
            outCamera.position = origin.approximate();
            outCamera.position.z = 0.0f;
            outCamera.position -= rotation.forward() * outCamera.farPlane * 0.5f;
                
        }

           
        outCamera.zoom = orthoZoom * calcRelativeScreenSize(viewportSize);
    }
}

//---

ViewportCameraController::ViewportCameraController()
{
}

void ViewportCameraController::configure(const ViewportCameraControllerSettings& settings)
{
    m_settings = settings;
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

    bool MapMovementKey(const input::KeyCode keyCode, MovementKey& outMovementKey)
    {
        switch (keyCode)
        {
            case input::KeyCode::KEY_W: outMovementKey = KEY_FOWARD; return true;
            case input::KeyCode::KEY_S: outMovementKey = KEY_BACKWARD; return true;
            case input::KeyCode::KEY_A: outMovementKey = KEY_LEFT; return true;
            case input::KeyCode::KEY_D: outMovementKey = KEY_RIGHT; return true;
            case input::KeyCode::KEY_Q: outMovementKey = KEY_UP; return true;
            case input::KeyCode::KEY_E: outMovementKey = KEY_DOWN; return true;
            case input::KeyCode::KEY_LEFT_CTRL: outMovementKey = KEY_SLOW; return true;
            case input::KeyCode::KEY_LEFT_SHIFT: outMovementKey = KEY_FAST; return true;
            case input::KeyCode::KEY_RIGHT_CTRL: outMovementKey = KEY_SLOW; return true;
            case input::KeyCode::KEY_RIGHT_SHIFT: outMovementKey = KEY_FAST; return true;
        }

        return false;
    }

    /// mouse input handling for the free fly camera
    class MouseCameraControlFreeFly : public IInputAction
    {
    public:
        MouseCameraControlFreeFly(IElement* ptr, ViewportCameraControllerSettings& settings, uint8_t mode, float sensitivity)
            : IInputAction(ptr)
            , m_settings(settings)
            , m_mode(mode)
            , m_sensitivity(sensitivity)
            , m_inputVelocity(0,0,0)
        {
            memzero(m_movementKeys, sizeof(m_movementKeys));
        }

        virtual void onUpdateRedrawPolicy(WindowRedrawPolicy& outRedrawPolicy) override final
        {
            outRedrawPolicy = WindowRedrawPolicy::ActiveOnly;
        }

        virtual InputActionResult onKeyEvent(const input::KeyEvent &evt) override final
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

            if (evt.keyCode() == input::KeyCode::KEY_ESCAPE && evt.pressed())
                return nullptr;

            return InputActionResult();
        }

        virtual InputActionResult onMouseEvent(const input::MouseClickEvent &evt, const ElementWeakPtr &hoverStack) override final
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

        virtual InputActionResult onMouseMovement(const input::MouseMovementEvent &evt, const ElementWeakPtr&hoverStack) override final
        {
            Angles deltaRot(0, 0, 0);
            Vector3 deltaPos(0, 0, 0);

            // update mouse angles
            /*if (mode == MoveMode_Slide)
            {
                // compute 3D velocity
                Vector3 cameraDirForward;
                owner->currentRotation().angleVectors(&cameraDirForward, nullptr, nullptr);

                auto cameraSpeed = config::cvCameraNormalSpeed.get() * pow(10.0f, owner->speedFactor());
                auto deltaDist = owner->currentDistanceToOrigin() * (-evt.delta().y * 0.001f * cameraSpeed);
                deltaPos = cameraDirForward * deltaDist;
            }
            else */if (m_mode == MoveMode_Look)
            {
                deltaRot.pitch = evt.delta().y * 0.25f * m_sensitivity;
                deltaRot.yaw = evt.delta().x * 0.25f * m_sensitivity;

                // compute 3D velocity
                Vector3 cameraDirForward;
                m_settings.rotation.someAngleVectors(&cameraDirForward, nullptr, nullptr);

                deltaPos = cameraDirForward * evt.delta().z * 0.5f;
            }
            else if (m_mode == MoveMode_Pan)
            {
                Vector3 cameraDirRight, cameraDirUp(0, 0, 1);
                m_settings.rotation.someAngleVectors(nullptr, &cameraDirRight, nullptr);

                float cameraSpeed = config::cvCameraNormalSpeed.get() * pow(2.0f, m_settings.speedLog);
                if (m_movementKeys[KEY_FAST]) cameraSpeed *= config::cvCameraFastMultiplier.get();
                if (m_movementKeys[KEY_SLOW]) cameraSpeed *= config::cvCameraSlowMultiplier.get();

                deltaPos = cameraDirRight * evt.delta().x * 0.01f * cameraSpeed * m_sensitivity;
                deltaPos -= cameraDirUp * evt.delta().y * 0.01f * cameraSpeed * m_sensitivity;
            }

            // apply movement to camera controller
            m_settings.position += deltaPos;
            m_settings.origin += deltaPos;
            m_settings.rotation += deltaRot;

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
            float cameraSpeed = config::cvCameraNormalSpeed.get() * pow(2.0f, m_settings.speedLog);
            Vector3 targetVelocity(0, 0, 0);
            {
                // aggregate movement
                Vector3 cameraMoveLocalDir(0, 0, 0);
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
                    Vector3 cameraDirForward, cameraDirRight, cameraDirUp;
                    m_settings.rotation.angleVectors(cameraDirForward, cameraDirRight, cameraDirUp);

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
                m_inputVelocity += ClampLength(delta, acc * timeDelta * (cameraSpeed / config::cvCameraNormalSpeed.get()));
            }

            // move the camera
            auto deltaPos = m_inputVelocity * timeDelta;
            m_settings.position += deltaPos;
        }

    private:
        ViewportCameraControllerSettings& m_settings;

        static const uint32_t MoveMode_Slide = 1;
        static const uint32_t MoveMode_Look = 2;
        static const uint32_t MoveMode_Pan = 4;

        Vector3 m_inputVelocity;

        Point m_startPoint;
        bool m_movementKeys[KEY_MAX];

        float m_sensitivity = 1.0f;
        uint8_t m_mode = 0;
    };

    //---

    /// mouse input handling for the orbit camera
    class MouseCameraControlOrbitAroundPoint : public IInputAction
    {
    public:
        MouseCameraControlOrbitAroundPoint(IElement* ptr, ViewportCameraControllerSettings& settings, uint8_t mode, bool updatePosition, float sensitivity)
            : IInputAction(ptr)
            , m_settings(settings)
            , m_mode(mode)
            , m_sensitivity(sensitivity)
            , m_updatePosition(updatePosition)
        {
            m_distance = m_settings.position.distance(m_settings.origin);
        }

        virtual void onUpdateRedrawPolicy(WindowRedrawPolicy& outRedrawPolicy) override final
        {
            outRedrawPolicy = WindowRedrawPolicy::ActiveOnly;
        }

        virtual InputActionResult onKeyEvent(const input::KeyEvent &evt) override final
        {
            return InputActionResult();
        }

        /*virtual void onRender3D(FrameParams& frame) override final
        {
            {
                runtime::SolidGeometryBuilder solid;
                solid.color(Color::LIGHTGREEN);
                solid.addSphere(m_orbitCenter.approximate(), 0.1f);
                frame.addGeometry(solid);
            }
        }*/

        virtual InputActionResult onMouseEvent(const input::MouseClickEvent &evt, const ElementWeakPtr&hoverStack) override final
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

        virtual InputActionResult onMouseMovement(const input::MouseMovementEvent &evt, const ElementWeakPtr&hoverStack) override final
        {
            // rotate local reference frame
            m_settings.rotation.pitch = std::clamp<float>(m_settings.rotation.pitch + m_sensitivity * evt.delta().y * 0.25f, -89.9f, 89.9f);
            m_settings.rotation.yaw += m_sensitivity * evt.delta().x * 0.25f;

            // calculate position and rotation of the camera
            if (m_updatePosition)
                m_settings.position = m_settings.origin - (m_distance * m_settings.rotation.forward());

            return InputActionResult();
        }

    private:
        ViewportCameraControllerSettings& m_settings;

        float m_distance = 1.0f;;
        float m_sensitivity = 1.0f;
        bool m_updatePosition = false;
        uint8_t m_mode = 0;
    };

    //--

    /// mouse input handling for the orbit camera
    class MouseCameraControlOriginShift : public IInputAction
    {
    public:
        MouseCameraControlOriginShift(IElement* ptr, ViewportCameraControllerSettings& settings, uint8_t mode, Vector3 xDir, Vector3 yDir, float sensitivity)
            : IInputAction(ptr)
            , m_settings(settings)
            , m_mode(mode)
            , m_xDir(xDir)
            , m_yDir(yDir)
            , m_sensitivity(sensitivity)
        {
        }

        virtual void onUpdateRedrawPolicy(WindowRedrawPolicy& outRedrawPolicy) override final
        {
            outRedrawPolicy = WindowRedrawPolicy::ActiveOnly;
        }

        virtual InputActionResult onKeyEvent(const input::KeyEvent& evt) override final
        {
            return InputActionResult();
        }

        virtual InputActionResult onMouseEvent(const input::MouseClickEvent& evt, const ElementWeakPtr& hoverStack) override final
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

        virtual InputActionResult onMouseMovement(const input::MouseMovementEvent& evt, const ElementWeakPtr& hoverStack) override final
        {
            const auto speed = pow(2.0f, m_settings.speedLog);

            auto delta = evt.delta().x * m_sensitivity * m_xDir * speed;
            delta += evt.delta().y * m_sensitivity * m_yDir * speed;

            m_settings.position += delta;
            m_settings.origin += delta;

            return InputActionResult();
        }

    private:
        ViewportCameraControllerSettings& m_settings;
        float m_sensitivity = 1.0f;
        uint8_t m_mode = 0;

        Vector3 m_xDir;
        Vector3 m_yDir;
    };

    //--

} // helper

InputActionPtr ViewportCameraController::handleGeneralFly(IElement* ptr, uint8_t button, float sensitivity)
{
    return RefNew<helper::MouseCameraControlFreeFly>(ptr, m_settings, button, sensitivity);
}

InputActionPtr ViewportCameraController::handleOrbitAroundPoint(IElement* ptr, uint8_t button, bool updatePosition, float sensitivity)
{
    return RefNew<helper::MouseCameraControlOrbitAroundPoint>(ptr, m_settings, button, updatePosition, sensitivity);
}

InputActionPtr ViewportCameraController::handleOriginShift(IElement* ptr, uint8_t button, const Vector3& xDir, const Vector3& yDir, float sensitivity)
{
    return RefNew<helper::MouseCameraControlOriginShift>(ptr, m_settings, button, xDir, yDir, sensitivity);
}

void ViewportCameraController::processMouseWheel(const input::MouseMovementEvent& evt, float delta)
{
    switch (m_settings.mode)
    {
        case CameraMode::OrbitPerspective:
        case CameraMode::FreePerspective:
        {
            auto dir = m_settings.rotation.forward();

            if (evt.keyMask().isShiftDown())
                delta *= 2.0f;
            else if (evt.keyMask().isCtrlDown())
                delta /= 2.0f;

            m_settings.position += dir * delta;
            break;
        }

        case CameraMode::FreeOrtho:
        case CameraMode::Left:
        case CameraMode::Top:
        case CameraMode::Front:
        case CameraMode::Right:
        case CameraMode::Bottom:
        case CameraMode::Back:
        {
            float zoom = 0.05f;

            if (evt.keyMask().isShiftDown())
                zoom *= 2.0f;
            else if (evt.keyMask().isCtrlDown())
                zoom /= 2.0f;
                
            if (delta < 0.0f)
                m_settings.orthoZoom *= (1.0f + zoom);
            else if (delta > 0.0f)
                m_settings.orthoZoom /= (1.0f + zoom);

            if (m_settings.orthoZoom < 0.1f)
                m_settings.orthoZoom = 0.1f;
            break;
        }
    }
}

//---

END_BOOMER_NAMESPACE_EX(ui)
