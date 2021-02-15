/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#include "build.h"
#include "renderingScenePanel.h"

#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingResources.h"
#include "rendering/scene/include/renderingFrameParams.h"
#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingSelectable.h"
#include "rendering/scene/include/renderingFrameRenderingService.h"
#include "rendering/scene/include/renderingFrameCameraContext.h"
#include "rendering/scene/include/renderingScene.h"

#include "rendering/ui_host/include/renderingPanel.h"

#include "base/ui/include/uiInputAction.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvas.h"
#include "base/ui/include/uiToolBar.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/system/include/thread.h"
#include "base/ui/include/uiTrackBar.h"

namespace ui
{
    //--

    static base::StringView RenderModeString(rendering::scene::FrameRenderMode mode)
    {
        switch (mode)
        {
        case rendering::scene::FrameRenderMode::Default: return "[img:viewmode] Default";
        case rendering::scene::FrameRenderMode::WireframeSolid: return "[img:cube] Solid Wireframe";
        case rendering::scene::FrameRenderMode::WireframePassThrough: return "[img:wireframe] Wire Wireframe";
        case rendering::scene::FrameRenderMode::DebugDepth: return "[img:order_front] Debug Depth";
        case rendering::scene::FrameRenderMode::DebugLuminance: return "[img:lightbulb] Debug Luminance";
        case rendering::scene::FrameRenderMode::DebugShadowMask: return "[img:shadow_on] Debug Shadowmask";
        case rendering::scene::FrameRenderMode::DebugAmbientOcclusion: return "[img:shader] Debug Ambient Occlusion";
        case rendering::scene::FrameRenderMode::DebugReconstructedViewNormals: return "[img:axis] Debug Normals";
        }

        return "";
    }

    static base::StringView CameraModeString(CameraMode mode)
    {
        switch (mode)
        {
            case CameraMode::FreePerspective: return "[img:camera] Perspective";
            case CameraMode::OrbitPerspective: return "[img:camera_orbit] Orbit";
            case CameraMode::FreeOrtho: return "[img:camera] Ortho";

            case CameraMode::Front: return "[img:view_front] Front";
            case CameraMode::Back: return "[img:view_back] Back";
            case CameraMode::Left: return "[img:view_left] Left";
            case CameraMode::Right: return "[img:view_right] Right";
            case CameraMode::Top: return "[img:view_top] Top";
            case CameraMode::Bottom: return "[img:view_bottom] Bottom";
        }

        return "";
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(RenderingScenePanel);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(PointSelectionMode);
        RTTI_ENUM_OPTION(Exact);
        RTTI_ENUM_OPTION(LargestArea);
        RTTI_ENUM_OPTION(SmallestArea);
        RTTI_ENUM_OPTION(Closest);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(AreaSelectionMode);
        RTTI_ENUM_OPTION(Everything);
        RTTI_ENUM_OPTION(ExcludeBorder);
        RTTI_ENUM_OPTION(LargestArea);
        RTTI_ENUM_OPTION(SmallestArea);
        RTTI_ENUM_OPTION(Closest);
    RTTI_END_TYPE();

    //----

    RenderingPanelSelectionQuery::RenderingPanelSelectionQuery(const base::Rect& area, base::Array<rendering::scene::EncodedSelectable> entries, const rendering::scene::Camera& camera)
        : m_area(area)
        , m_entries(std::move(entries))
        , m_camera(camera)
    {
    }

    static float SquaredDistanceTo(uint16_t x, uint16_t y, const base::Point& point)
    {
        auto dx = (float)((int)x - point.x);
        auto dy = (float)((int)y - point.y);
        return dx * dx + dy * dy;
    }

    bool RenderingPanelSelectionQuery::extractPointSelection(PointSelectionMode mode, const base::Point& point, rendering::scene::Selectable& outSelectable) const
    {
        // no data yet captured
        if (m_entries.empty())
            return false;

        // the point is not in the area
        if (!m_area.contains(point))
            return false;

        // process the raw data
        rendering::scene::Selectable bestSelectable;
        {
            auto bestDepth = std::numeric_limits<float>::max();
            for (const auto& raw : m_entries)
            {
                static const auto selectionRange = 4.0f * 4.0f;
                if (SquaredDistanceTo(raw.x, raw.y, point) <= selectionRange)
                {
                    if (raw.depth <= bestDepth)
                    {
                        bestSelectable = raw.object;
                        bestDepth = raw.depth;
                    }
                }
            }
        }

        // TODO: modes

        if (bestSelectable)
        {
            outSelectable = bestSelectable;
            return true;
        }

        return false;
    }

    bool RenderingPanelSelectionQuery::extractAreaSelection(AreaSelectionMode mode, const base::Rect& rect, base::HashSet<rendering::scene::Selectable>& outSelectables) const
    {
        // clamp the rect to the rect of the available area
        auto minX = std::max(m_area.min.x, rect.min.x);
        auto minY = std::max(m_area.min.y, rect.min.y);
        auto maxX = std::min(m_area.max.x, rect.max.x);
        auto maxY = std::min(m_area.max.y, rect.max.y);
        if (minX >= maxX || minY >= maxY)
            return false; // empty area

        base::Rect clippedRect(minX, minY, maxX, maxY);

        // process elements
        if (mode == AreaSelectionMode::Everything)
        {
            // collect all elements
            for (const auto& raw : m_entries)
            {
                if (clippedRect.contains(raw.x, raw.y))
                    outSelectables.insert(raw.object);
            }
        }
        else if (mode == AreaSelectionMode::ExcludeBorder)
        {
            // get the inset
            auto innerSize = clippedRect.inner(base::Point(1, 1));

            // collect all elements
            for (const auto& raw : m_entries)
            {
                if (innerSize.contains(raw.x, raw.y))
                    outSelectables.insert(raw.object);
            }

            // remove the border elements
            for (const auto& raw : m_entries)
            {
                if (clippedRect.contains(raw.x, raw.y) && !innerSize.contains(raw.x, raw.y))
                    outSelectables.remove(raw.object);
            }
        }
        else
        {
            // TODO: other modes
        }

        return true;
    }

    //---

    RenderingPanelDepthBufferQuery::RenderingPanelDepthBufferQuery(uint32_t width, uint32_t height, const rendering::scene::Camera& camera, const base::Rect& validDepthValues, base::Array<float>&& depthValues, bool flipped)
        : m_width(width)
        , m_height(height)
        , m_camera(camera)
        , m_depthValues(std::move(depthValues))
        , m_depthValuesRect(validDepthValues)
        , m_flipped(flipped)
    {}

    float RenderingPanelDepthBufferQuery::projectedDepthValueAtPixel(int x, int y, bool* withinRect /*= nullptr*/) const
    {
        if (m_depthValuesRect.contains(x, y))
        {
            x -= m_depthValuesRect.min.x;

            if (m_flipped)
                y = (m_depthValuesRect.max.y-1) - y;
            else
                y -= m_depthValuesRect.min.y;

            const auto offset = x + y * m_depthValuesRect.width();
            if (withinRect)
                *withinRect = true;

            return m_depthValues[offset];
        }

        if (withinRect)
            *withinRect = false;

        return 1.0f; // far plane
    }

    float RenderingPanelDepthBufferQuery::linearDepthValueAtPixel(int x, int y, bool* withinRect /*= nullptr*/) const
    {
        float projectedZ = projectedDepthValueAtPixel(x, y, withinRect);
        return m_camera.projectedZToLinearZ(projectedZ);
    }

    bool RenderingPanelDepthBufferQuery::calcWorldPosition(int x, int y, base::AbsolutePosition& outPos) const
    {
        // query (projected) depth under the cursor
        auto projectedZ = projectedDepthValueAtPixel(x, y);
        if (projectedZ <= 0.0f || projectedZ >= 1.0f)
            return false; // we trust values that are not near or far plane only

        // convert to screen coordinates
        float sx = -1.0f + 2.0f * (x / (float)(m_width - 1));
        float sy = -1.0f + 2.0f * (y / (float)(m_height - 1));
        TRACE_INFO("SX: {}, SY: {}, PZ:{}", sx, sy, projectedZ);

        // convert to world position
        base::Vector4 screenPos(sx, sy, projectedZ, 1.0f);
        auto worldPos = m_camera.screenToWorld().transformVector4(screenPos);
        if (worldPos.w == 0.0f)
            return false;

        outPos = base::AbsolutePosition(worldPos.projected().xyz());
        return true;
    }

    //----

    base::ConfigProperty<float> cvViewportAxisLength("Editor", "ViewportAxisLength", 40.0f);
    base::ConfigProperty<float> cvViewportUpdateInterval("Editor", "ViewportUpdateInterval", 1000.0f);

    //----

    RTTI_BEGIN_TYPE_CLASS(RenderingScenePanelSettings);
        RTTI_PROPERTY(renderMode);
        RTTI_PROPERTY(renderMaterialDebugChannelName);
        //RTTI_PROPERTY(filters);
        RTTI_PROPERTY(cameraSpeedFactor);
        //RTTI_PROPERTY(cameraForceOrbit);
        RTTI_PROPERTY(paused);
        RTTI_PROPERTY(timeDeltaScale);
        RTTI_PROPERTY(areaSelectionMode);
        RTTI_PROPERTY(pointSelectionMode);
        RTTI_PROPERTY(drawInternalGrid);
        RTTI_PROPERTY(drawInternalWorldAxis);
        RTTI_PROPERTY(drawInternalCameraAxis);
        RTTI_PROPERTY(drawInternalCameraData);
    RTTI_END_TYPE();

    RenderingScenePanelSettings::RenderingScenePanelSettings()
    {
        filters = rendering::scene::FilterFlags::DefaultEditor();
    }

    //---

    RenderingScenePanel::RenderingScenePanel()
        : m_updateTimer(this, "Update"_id)
    {
        m_cameraContext = base::RefNew<rendering::scene::CameraContext>();

        m_lastUpdateTime.resetToNow();
        m_updateTimer = [this]() {
            auto dt = m_lastUpdateTime.timeTillNow().toSeconds();
            m_lastUpdateTime.resetToNow();
            handleUpdate(dt);
        };

        m_updateTimer.startRepeated(1.0f / cvViewportUpdateInterval.get());

        m_toolbar = createChildWithType<ToolBar>("PreviewPanelToolbar"_id);
        m_toolbar->customHorizontalAligment(ElementHorizontalLayout::Expand);
        m_toolbar->customVerticalAligment(ElementVerticalLayout::Top);
        m_toolbar->overlay(true);

        m_bottomToolbar = createChildWithType<ToolBar>("PreviewPanelToolbar"_id);
        m_bottomToolbar->customHorizontalAligment(ElementHorizontalLayout::Expand);
        m_bottomToolbar->customVerticalAligment(ElementVerticalLayout::Bottom);
        m_bottomToolbar->overlay(true);

        createToolbarItems();
    }

    RenderingScenePanel::~RenderingScenePanel()
    {}

    bool RenderingScenePanel::computeContentBounds(base::Box& outBox) const
    {
        return false;
    }

    void RenderingScenePanel::panelSettings(const RenderingScenePanelSettings& settings)
    {
        auto oldMode = m_panelSettings.renderMode;

        m_panelSettings = settings;

        if (settings.renderMode != oldMode)
        {
            toolbar()->updateButtonCaption("PreviewPanel.ChangeRenderMode"_id, ui::ToolbarButtonSetup().caption(RenderModeString(settings.renderMode)));
        }
    }

    void RenderingScenePanel::cameraSettings(const CameraControllerSettings& settings)
    {
        auto oldMode = m_cameraController.settings().mode;

        m_cameraController.configure(settings);

        if (settings.mode != oldMode)
        {
            toolbar()->updateButtonCaption("PreviewPanel.ChangeCameraMode"_id, ui::ToolbarButtonSetup().caption(CameraModeString(settings.mode)));
        }
    }

    void RenderingScenePanel::focusOnBounds(const base::Box& bounds, float distanceFactor /*= 1.0f*/, const base::Angles* newRotation/*=nullptr*/)
    {
        auto settings = m_cameraController.settings();
        if (settings.perspective())
        {
            // get new rotation or use the existing one
            if (newRotation)
                settings.rotation = *newRotation;

            // compute the maximum object size
            auto distance = bounds.extents().length() * 3.0f * distanceFactor;
            settings.position = bounds.center() - distance * settings.rotation.forward();
            settings.origin = bounds.center();

            m_cameraController.configure(settings);
        }
    }

    //--

    void RenderingScenePanel::handleUpdate(float dt)
    {
        m_engineTimeCounter += dt;
        m_gameTimeCounter += dt;
    }

    bool RenderingScenePanel::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        return TBaseClass::handleKeyEvent(evt);
    }

    namespace helper
    {
        //--

        // helper input mode for point selection
        class SelectionClickInputHandler : public IInputAction
        {
        public:
            typedef std::function<void(bool ctrl, bool shift, const base::Point & point)> TSelectionFunction;
            typedef std::function<InputActionResult()> TContinuationFunction;

            SelectionClickInputHandler(IElement* ptr, const base::Point& startPoint, const TSelectionFunction& selectionFunc, const TContinuationFunction& continuationFunc)
                : IInputAction(ptr)
                , m_startPoint(startPoint)
                , m_selectionFunction(selectionFunc)
                , m_continuationFunction(continuationFunc)
            {
            }

            virtual InputActionResult onMouseEvent(const base::input::MouseClickEvent& evt, const ElementWeakPtr& hoveredElement) override
            {
                if (evt.leftReleased())
                {
                    auto relativePos = evt.absolutePosition() - element()->cachedDrawArea().absolutePosition();
                    auto ctrlPressed = evt.keyMask().isCtrlDown();
                    auto shiftPressed = evt.keyMask().isShiftDown();
                    m_selectionFunction(ctrlPressed, shiftPressed, relativePos);
                }
                else if (evt.rightClicked())
                {
                    if (m_continuationFunction)
                        return m_continuationFunction();
                }

                return nullptr;
            }

            virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoveredElement) override
            {
                auto dist = evt.absolutePosition().distanceTo(m_startPoint);
                if (dist <= 4.0f)
                    return InputActionResult();

                if (m_continuationFunction)
                    return m_continuationFunction();
                return nullptr;
            }

        private:
            TSelectionFunction m_selectionFunction;
            TContinuationFunction m_continuationFunction;
            base::Point m_startPoint;            
        };

        //--

        // helper input mode for point selection
        class SelectionAreaInputHandler : public IInputAction
        {
        public:
            typedef std::function<void(bool ctrl, bool shift, const base::Rect & rect)> TSelectionFunction;

            SelectionAreaInputHandler(IElement* ptr, const base::Point& startPoint, const TSelectionFunction& selectionFunc)
                : IInputAction(ptr)
                , m_startPoint(startPoint)
                , m_currentPoint(startPoint)
                , m_selectionFunction(selectionFunc)
            {
            }

            virtual InputActionResult onMouseEvent(const base::input::MouseClickEvent& evt, const ElementWeakPtr& hoveredElement) override
            {
                if (evt.leftReleased())
                {
                    auto ctrlPressed = evt.keyMask().isCtrlDown();
                    auto shiftPressed = evt.keyMask().isShiftDown();
                    auto rect = calcSelectionRect();
                    if (!rect.empty())
                        m_selectionFunction(ctrlPressed, shiftPressed, rect);

                    return nullptr;
                }
                else
                {
                    return nullptr;
                }
            }

            virtual void onRender(base::canvas::Canvas& canvas) override
            {
                auto rect = calcSelectionRect();
                if (!rect.empty())
                {
					base::canvas::Geometry g;

					{
						base::canvas::GeometryBuilder b(g);
						b.fillColor(base::Color(50, 50, 180, 100));
						b.rect(0.0f, 0.0f, rect.width(), rect.height());
						b.fill();
					}

                    auto rectPlacement = rect.min.toVector() + element()->cachedDrawArea().absolutePosition();
                    auto area = element()->cachedDrawArea();

                    canvas.place(base::Vector2(rectPlacement.x, rectPlacement.y), g);
                }
            }

            virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoveredElement) override
            {
                m_currentPoint = evt.absolutePosition();
                return InputActionResult();
            }

            base::Rect calcSelectionRect() const
            {
                auto relativeStart = m_startPoint - element()->cachedDrawArea().absolutePosition();
                auto relativeEnd = m_currentPoint - element()->cachedDrawArea().absolutePosition();
                return base::Rect(relativeStart, relativeEnd); // sorts the coordinates
            }

        private:
            CameraController* m_camera;
            TSelectionFunction m_selectionFunction;
            base::Point m_startPoint;
            base::Point m_currentPoint;
        };

        //--


        /// mouse input handling for the light rotation
        class MouseLightRotation : public MouseInputAction
        {
        public:
            typedef std::function<void(float dp, float dy)> TRotateFunction;

            MouseLightRotation(IElement* ptr, const TRotateFunction& func)
                : MouseInputAction(ptr, base::input::KeyCode::KEY_MOUSE1, true)
                , m_rotateLightFunction(func)
            {}

            virtual void onUpdateRedrawPolicy(WindowRedrawPolicy& outRedrawPolicy) override final
            {
                outRedrawPolicy = WindowRedrawPolicy::ActiveOnly;
            }

            virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoveredElement) override final
            {
                auto deltaPitch = -evt.delta().y * 0.5f;
                auto deltaYaw = -evt.delta().x * 0.5f;
                m_rotateLightFunction(deltaPitch, deltaYaw);
                return InputActionResult();
            }

        private:
            TRotateFunction m_rotateLightFunction;
        };

        //--

    } // helper

    void RenderingScenePanel::handleFocusLost()
    {
        TBaseClass::handleFocusLost();
        //m_cameraController.resetInput();
    }

    InputActionPtr RenderingScenePanel::createLeftMouseButtonCameraAction(const ElementArea& area, const base::input::MouseClickEvent& evt, bool allowSelection)
    {
        const float zoomScale = 1.0f / (1 << m_renderTargetZoom);

        if (allowSelection)
        {
            if (evt.keyMask().isShiftDown())
            {
                auto selectionFunc = [this](bool ctrl, bool shift, const base::Rect& rect) { handleAreaSelection(ctrl, shift, rect); };
                return base::RefNew<helper::SelectionAreaInputHandler>(this, evt.absolutePosition(), selectionFunc);
            }
            else
            {
                // create a point selection mode
                // NOTE: this can mutate into the camera movement
                auto selectionFunc = [this](bool ctrl, bool shift, const base::Point& point) { handlePointSelection(ctrl, shift, point); };
                auto copiedEvent = base::RefNew<base::input::MouseClickEvent>(evt.deviceID(), evt.keyCode(), evt.type(), evt.keyMask(), evt.windowPosition(), evt.absolutePosition());
                auto continuationFunc = [this, area, copiedEvent]() -> InputActionPtr { return createLeftMouseButtonCameraAction(area, *copiedEvent, false); };
                return base::RefNew<helper::SelectionClickInputHandler>(this, evt.absolutePosition(), selectionFunc, continuationFunc);
            }
        }

        switch (m_cameraController.settings().mode)
        {
            case CameraMode::FreePerspective:
                return m_cameraController.handleGeneralFly(this, 1, zoomScale);

            case CameraMode::FreeOrtho:
                return m_cameraController.handleOrbitAroundPoint(this, 1, false, zoomScale);

            case CameraMode::OrbitPerspective:
                return m_cameraController.handleOrbitAroundPoint(this, 1, true, zoomScale);
        }

        return nullptr;
    }

    InputActionPtr RenderingScenePanel::createRightMouseButtonCameraAction(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        const float zoomScale = 1.0f / (1 << m_renderTargetZoom);

        switch (m_cameraController.settings().mode)
        {
            case CameraMode::FreePerspective:
                return m_cameraController.handleGeneralFly(this, 2, zoomScale);

            case CameraMode::Top:
            case CameraMode::Bottom:
            case CameraMode::Front:
            case CameraMode::Back:
            case CameraMode::Left:
            case CameraMode::Right:
            case CameraMode::FreeOrtho:
            {
                base::Vector3 orthoMask(1,1,1);
                base::Angles orthoRotation;

                switch (m_cameraController.settings().mode)
                {
                    case CameraMode::Top:
                    {
                        orthoRotation = base::Angles(90.0f, 0.0f, 0.0f);
                        orthoMask.z = 0.0f;
                        break;
                    }

                    case CameraMode::Bottom:
                    {
                        orthoRotation = base::Angles(-90.0f, 0.0f, 0.0f);
                        orthoMask.z = 0.0f;
                        break;
                    }

                    case CameraMode::Back:
                    {
                        orthoRotation = base::Angles(0.0f, 0.0f, 0.0f);
                        orthoMask.x = 0.0f;
                        break;
                    }

                    case CameraMode::Front:
                    {
                        orthoRotation = base::Angles(0.0f, 180.0f, 0.0f);
                        orthoMask.x = 0.0f;
                        break;
                    }

                    case CameraMode::Left:
                    {
                        orthoRotation = base::Angles(0.0f, 90.0f, 0.0f);
                        orthoMask.y = 0.0f;
                        break;
                    }

                    case CameraMode::Right:
                    {
                        orthoRotation = base::Angles(0.0f, -90.0f, 0.0f);
                        orthoMask.y = 0.0f;
                        break;
                    }

                    default:
                        orthoRotation = m_cameraController.settings().rotation;
                        orthoMask.z = 0.0f;
                        break;
                }

                base::Vector3 xDir(0, 0, 0), yDir(0, 0, 0), mask(1, 1, 1);
                orthoRotation.someAngleVectors(nullptr, &xDir, &yDir);

                xDir *= orthoMask;
                yDir *= orthoMask;

                float relativeScreenSize = m_cameraController.settings().calcRelativeScreenSize(Size(1.0f, 1.0f)) * 2.0f;
                xDir *= -relativeScreenSize * m_cameraController.settings().orthoZoom;
                yDir *= relativeScreenSize * m_cameraController.settings().orthoZoom;

                return m_cameraController.handleOriginShift(this, 2, xDir, yDir, zoomScale);
            }

            case CameraMode::OrbitPerspective:
                return m_cameraController.handleOrbitAroundPoint(this, 2, zoomScale);
        }

        return nullptr;
    }

    InputActionPtr RenderingScenePanel::createMiddleMouseButtonCameraAction(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        const float zoomScale = 1.0f / (1 << m_renderTargetZoom);

        /*// orbit around point
        if (evt.keyMask().isAltDown())
        {
            // orbit around point
            base::AbsolutePosition worldOrbitPosition;
            auto viewportPos = (evt.absolutePosition() - cachedDrawArea().absolutePosition());
            if (queryWorldPositionUnderCursor(viewportPos, worldOrbitPosition))
            {
                //m_panelSettings.
                if (auto ret = m_cameraController.handleOrbitAroundPoint(this, 4, zoomScale))
                {
                    m_renderInputAction = ret;
                    return ret;
                }
            }
        }*/

        switch (m_cameraController.settings().mode)
        {
            case CameraMode::FreePerspective:
                return m_cameraController.handleGeneralFly(this, 4, zoomScale);
        }

        return nullptr;
    }

    InputActionPtr RenderingScenePanel::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.keyMask().isCtrlDown() && evt.rightClicked())
        {
            auto rotateFunc = [this](float dp, float dy) { rotateGlobalLight(dp, dy); };
            return base::RefNew<helper::MouseLightRotation>(this, rotateFunc);
        }
        else  if (evt.leftClicked() && !evt.keyMask().isAltDown())
        {
           if (auto ret = createLeftMouseButtonCameraAction(area, evt, true))
           {
               m_renderInputAction = ret;
               return ret;
           }
        }
        else if (evt.rightClicked())
        {
            if (auto ret = createRightMouseButtonCameraAction(area, evt))
            {
                m_renderInputAction = ret;
                return ret;
            }
        }
        else if (evt.midClicked())
        {
            if (auto ret = createMiddleMouseButtonCameraAction(area, evt))
            {
                m_renderInputAction = ret;
                return ret;
            }
        }

        return InputActionPtr();
    }

    bool RenderingScenePanel::handleMouseMovement(const base::input::MouseMovementEvent& evt)
    {
        return TBaseClass::handleMouseMovement(evt);
    }

    bool RenderingScenePanel::handleMouseWheel(const base::input::MouseMovementEvent& evt, float delta)
    {
        float cameraDistance = 0.0f;
        if (delta > 0.0f)
            cameraDistance = 0.5f;
        else if (delta < 0.0f)
            cameraDistance = -0.5f;

        if (evt.keyMask().isCtrlDown())
            cameraDistance *= 0.2f;
        else if (evt.keyMask().isShiftDown())
            cameraDistance *= 5.0f;

        m_cameraController.processMouseWheel(evt, cameraDistance);
        return true;
    }

    void RenderingScenePanel::handleCamera(rendering::scene::CameraSetup& outCamera)
    {
        const auto& renderArea = cachedDrawArea(); // TODO: would be perfect not to use cached shit
        m_cameraController.settings().computeRenderingCamera(renderArea.size(), outCamera);

        m_cachedCamera.setup(outCamera);
    }

    void RenderingScenePanel::handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables)
    {
        // nothing
    }

    void RenderingScenePanel::handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
    {
        // nothing
    }

    void RenderingScenePanel::handleContextMenu(bool ctrl, bool shift, const Position& absolutePosition, const base::Point& clientPosition, const rendering::scene::Selectable& objectUnderCursor, const base::AbsolutePosition* positionUnderCursor)
    {
        // nothing
    }

    static bool GFirstSelectionClickThatMayHaveNoShaders = true;

    void RenderingScenePanel::handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition)
    {
        // compute selection area around pixel
        int radius = 4;
        base::Rect selectionArea(clientPosition.x - radius, clientPosition.y - radius, clientPosition.x + radius, clientPosition.y + radius);

        // query the raw selection data from the rendering
        auto selectionData = querySelection(&selectionArea);

        // HACK: query again on first use
        if (GFirstSelectionClickThatMayHaveNoShaders)
        {
            base::Sleep(300);
            GFirstSelectionClickThatMayHaveNoShaders = false;
            selectionData = querySelection(&selectionArea);
        }

        // build selectables
        base::Array<rendering::scene::Selectable> selectables;
        if (selectionData)
        {
            rendering::scene::Selectable singleSelectable;
            if (selectionData->extractPointSelection(m_panelSettings.pointSelectionMode, clientPosition, singleSelectable))
            {
                selectables.pushBack(singleSelectable);
            }
        }

        // handle the selection with the list of selectables
        handlePointSelection(ctrl, shift, clientPosition, selectables);
    }

    void RenderingScenePanel::handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect)
    {
        // query the raw selection data from the rendering
        auto selectionData = querySelection(&clientRect);

        // HACK: query again on first use
        if (GFirstSelectionClickThatMayHaveNoShaders)
        {
            base::Sleep(300);
            GFirstSelectionClickThatMayHaveNoShaders = false;
            selectionData = querySelection(&clientRect);
        }

        // build selectables
        base::HashSet<rendering::scene::Selectable> selectables;
        if (selectionData)
        {
            selectionData->extractAreaSelection(m_panelSettings.areaSelectionMode, clientRect, selectables);
        }

        // handle the selection with the list of selectables
        handleAreaSelection(ctrl, shift, clientRect, selectables.keys());
    }

    bool RenderingScenePanel::handleContextMenu(const ElementArea& area, const Position& absolutePosition, base::input::KeyMask controlKeys)
    {
        // well, something is wrong (probably capture)
        if (!area.contains(absolutePosition))
            return false;

        // calculate local click pos
        base::Point clickPos;
        clickPos.x = (int)(absolutePosition.x - area.absolutePosition().x);
        clickPos.y = (int)(absolutePosition.y - area.absolutePosition().y);

        // query the raw selection data from the rendering
        const auto clickRect = base::Rect(clickPos.x, clickPos.y, clickPos.x + 1, clickPos.y + 1);
        auto selectionData = querySelection(&clickRect);

        // get the clicked selectable
        rendering::scene::EncodedSelectable selectable;
        if (!selectionData->entries().empty())
        {
            auto index = (clickPos.x - selectionData->area().min.x);
            index += (clickPos.y - selectionData->area().min.y) * selectionData->area().width();
            if (index < selectionData->entries().size())
            {
                selectable = selectionData->entries()[index];
            }
        }

        // determine the click position
        bool clickPositionValid = false;
        base::AbsolutePosition clickPosition;
        if (selectable.object && selectable.depth)
        {
            // render area not cached
            auto renderAreaWidth = (int)cachedDrawArea().size().x;
            auto renderAreaHeight = (int)cachedDrawArea().size().y;

            // convert to screen coordinates
            float sx = -1.0f + 2.0f * (clickPos.x / (float)(renderAreaWidth - 1));
            float sy = -1.0f + 2.0f * (clickPos.y / (float)(renderAreaHeight - 1));

            // convert linear Z from selection to a projected Z
            float sz = selectionData->camera().linearZToProjectedZ(selectable.depth);

            // convert to world position
            base::Vector4 screenPos(sx, sy, sz, 1.0f);
            auto worldPos = selectionData->camera().screenToWorld().transformVector4(screenPos);
            if (worldPos.w > 0.0f)
            {
                clickPosition = base::AbsolutePosition(worldPos.projected().xyz());
                clickPositionValid = true;
            }
        }

        // call general handler but with more data
        handleContextMenu(controlKeys.test(base::input::KeyMaskBit::ANY_CTRL), controlKeys.test(base::input::KeyMaskBit::ANY_SHIFT),
            absolutePosition, clickPos,
            selectable.object, clickPositionValid ? &clickPosition : nullptr);

        return true;
    }

    void RenderingScenePanel::handleRender(rendering::scene::FrameParams& frame)
    {
        // draw grid
        if (m_panelSettings.drawInternalGrid && (frame.filters & rendering::scene::FilterBit::ViewportWorldGrid))
            drawGrid(frame);

        // draw world axes
        if (m_panelSettings.drawInternalWorldAxis && (frame.filters & rendering::scene::FilterBit::ViewportWorldAxes))
        {
            rendering::scene::DebugDrawer dd(frame.geometry.solid);
            dd.axes(base::Matrix::IDENTITY(), 1.0f);
        }

        // draw screen axis
        if (m_panelSettings.drawInternalCameraAxis && (frame.filters & rendering::scene::FilterBit::ViewportCameraAxes))
            drawViewAxes(cachedDrawArea().size().x, cachedDrawArea().size().y, frame);

        // draw camera info
        if (m_panelSettings.drawInternalCameraData && (frame.filters & rendering::scene::FilterBit::ViewportCameraInfo))
            drawCameraInfo(cachedDrawArea().size().x, cachedDrawArea().size().y, frame);

        // use the selected render mode
        frame.mode = m_panelSettings.renderMode;
        frame.debug.materialDebugMode = m_panelSettings.renderMaterialDebugChannelName;

        // draw input action
        //if (auto action = m_renderInputAction.lock())
          //  action->onRender3D(frame);
    }

    void RenderingScenePanel::drawGrid(rendering::scene::FrameParams& frame)
    {
        static base::Array<base::Vector3> points;

        const float z = -0.01f - std::log10f(std::max<float>(0.0f, frame.camera.camera.position().length())) * 0.05f;

        if (points.empty())
        {
            points.reserve(2048);

            // huge grid
            {
                auto gridStep = 100.0f;
                auto gridSteps = 10;
                auto gridSize = gridStep * gridSteps;

                for (int i = -gridSteps; i <= gridSteps; ++i)
                {
                    if (i != 0)
                    {
                        auto pos = (float)i * gridStep;
                        points.emplaceBack(-gridSize, pos, z);
                        points.emplaceBack(gridSize, pos, z);
                        points.emplaceBack(pos, -gridSize, z);
                        points.emplaceBack(pos, gridSize, z);
                    }
                }
            }

            // big grid
            {
                auto gridStep = 10.0f;
                auto gridSteps = 10;
                auto gridSize = gridStep * gridSteps;

                for (int i = -gridSteps; i <= gridSteps; ++i)
                {
                    if (i != 0)
                    {
                        auto pos = (float)i * gridStep;
                        points.emplaceBack(-gridSize, pos, z);
                        points.emplaceBack(gridSize, pos, z);
                        points.emplaceBack(pos, -gridSize, z);
                        points.emplaceBack(pos, gridSize, z);
                    }
                }
            }

            // small grid
            {
                auto gridStep = 1.0f;
                auto gridSteps = 10;
                auto gridSize = gridStep * gridSteps;

                for (int i = -gridSteps; i <= gridSteps; ++i)
                {
                    if (i != 0)
                    {
                        auto pos = (float)i * gridStep;
                        points.emplaceBack(-gridSize, pos, z);
                        points.emplaceBack(gridSize, pos, z);
                        points.emplaceBack(pos, -gridSize, z);
                        points.emplaceBack(pos, gridSize, z);
                    }
                }
            }

            // tiny grid
            {
                auto gridStep = 0.1f;
                auto gridSteps = 10;
                auto gridSize = gridStep * gridSteps;

                for (int i = -gridSteps; i <= gridSteps; ++i)
                {
                    if (i != 0)
                    {
                        auto pos = (float)i * gridStep;
                        points.emplaceBack(-gridSize, pos, z);
                        points.emplaceBack(gridSize, pos, z);
                        points.emplaceBack(pos, -gridSize, z);
                        points.emplaceBack(pos, gridSize, z);
                    }
                }
            }
        }

        {
            rendering::scene::DebugDrawer dd(frame.geometry.solid);
            dd.color(base::Color(100, 100, 100));
            dd.lines(points.typedData(), points.size());

            base::Vector3 centerLines[4];
            centerLines[0] = base::Vector3(-1000.0f, 0.0f, z);
            centerLines[1] = base::Vector3(1000.0f, 0.0f, z);
            centerLines[2] = base::Vector3(0.0f, -1000.0f, z);
            centerLines[3] = base::Vector3(0.0f, 1000.0f, z);
            dd.color(base::Color(130, 150, 150));
            dd.lines(centerLines, 4);
        }
    }

    void RenderingScenePanel::drawViewAxes(uint32_t width, uint32_t height, rendering::scene::FrameParams& frame)
    {
        // calculate placement
        auto scale = cvViewportAxisLength.get() * 1.0f;// cachedStyleParams().m_scale;
        auto bx = 15.0f + scale;
        auto by = height - 15.0f - scale;

        // build orientation matrix using
        auto orientationMatrix = frame.camera.camera.worldToCamera() * rendering::scene::CameraSetup::CameraToView;

        // project the view axes
        auto ex = orientationMatrix.transformVector(base::Vector3::EX()) * scale;
        auto ey = orientationMatrix.transformVector(base::Vector3::EY()) * scale;
        auto ez = orientationMatrix.transformVector(base::Vector3::EZ()) * scale;

        // render the axes with orthographic projection
        base::InplaceArray<base::Vector2, 8> vertices;

        /*auto o = base::Vector2(bx, by);
        auto px = o + base::Vector2(ex.x, ex.y);
        auto py = o + base::Vector2(ey.x, ey.y);
        auto pz = o + base::Vector2(ez.x, ez.y);
        dd.lineColor(base::Color::RED);
        dd.line(o, px);
        dd.lineColor(base::Color::GREEN);
        dd.line(o, py);
        dd.lineColor(base::Color::BLUE);
        dd.line(o, pz);

        // labels
        auto tx = o + base::Vector2(ex.x, ex.y) * 1.05f;
        auto ty = o + base::Vector2(ey.x, ey.y) * 1.05f;
        auto tz = o + base::Vector2(ez.x, ez.y) * 1.05f;
        dd.lineColor(base::Color::WHITE);
        dd.alignHorizontal(0);
        dd.alignVertical(0);
        dd.text(tx.x, tx.y, "X");
        dd.text(ty.x, ty.y, "Y");
        dd.text(tz.x, tz.y, "Z");*/
    }

    void RenderingScenePanel::drawCameraInfo(uint32_t width, uint32_t height, rendering::scene::FrameParams& frame)
    {
        auto bx = width - 20;
        auto by = height - 20;

        auto pos = m_cameraController.settings().position.approximate();
        auto rot = m_cameraController.settings().rotation;

        auto params = rendering::scene::DebugTextParams().right().bottom().color(base::Color::WHITE);
        rendering::scene::DebugDrawer dd(frame.geometry.screen);
        dd.text(bx, by, base::TempString("Camera [{}, {}, {}], Pitch: {}, Yaw: {}", Prec(pos.x, 1), Prec(pos.y, 1), Prec(pos.z, 1), Prec(rot.pitch, 1), Prec(rot.yaw, 1)), params);
    }

    void RenderingScenePanel::rotateGlobalLight(float deltaPitch, float deltaYaw)
    {
        /*auto newPitch = std::clamp<float>(environmentSettings()->readParam<float>("defaultLightPitch", 0.0f) + deltaPitch, -90.0f, 90.0f);
        auto newYaw = base::AngleNormalize(environmentSettings()->readParam<float>("defaultLightYaw", 0.0f) + deltaYaw);
        environmentSettings()->writeParam<float>("defaultLightPitch", newPitch);
        environmentSettings()->writeParam<float>("defaultLightYaw", newYaw);*/
    }

    //--

    class EncodedSelectionDataSink : public rendering::IDownloadDataSink
    {
    public:
        EncodedSelectionDataSink()
        {
            //m_fence = Fibers::GetInstance().createCounter("WaitForSelectionData", 1);
        }

        virtual void processRetreivedData(const void* dataPtr, uint32_t dataSize, const rendering::ResourceCopyRange& info) override final
        {
            auto maxEntires = range_cast<uint32_t>(dataSize / sizeof(rendering::scene::EncodedSelectable));

            m_selectionRect = base::Rect::EMPTY();
            m_selectables.clear();
            m_selectables.reserve(maxEntires);

            // get the raw data
            const auto* ptr = (const rendering::scene::EncodedSelectable*)dataPtr;

            // process the raw data
            auto selectionRect = base::Rect::EMPTY();
            for (uint32_t i = 0; i < maxEntires; ++i, ++ptr)
            {
                if (*ptr)
                {
                    m_selectables.pushBack(*ptr);
                    m_selectionRect.merge(ptr->x, ptr->y);
                }
            }

            //Fibers::GetInstance().signalCounter(m_fence, 1);
        }

        base::RefPtr<RenderingPanelSelectionQuery> waitAndFetch(const rendering::scene::Camera& camera)
        {
            //Fibers::GetInstance().waitForCounterAndRelease(m_fence);
            return base::RefNew<RenderingPanelSelectionQuery>(m_selectionRect, std::move(m_selectables), camera);
        }

    private:
        base::Array<rendering::scene::EncodedSelectable> m_selectables;
        base::Rect m_selectionRect;

        //base::fibers::WaitCounter m_fence;
    };

    //--

    base::RefPtr<RenderingPanelSelectionQuery> RenderingScenePanel::querySelection(const base::Rect* captureArea)
    {
        // render area not cached
        auto renderAreaWidth = (int)cachedDrawArea().size().x;
        auto renderAreaHeight = (int)cachedDrawArea().size().y;

        // clamp the area
        rendering::scene::FrameParams_Capture capture;
        if (captureArea)
        {
            auto captureAreaMinX = std::clamp<int>(captureArea->min.x, 0, renderAreaWidth);
            auto captureAreaMinY = std::clamp<int>(captureArea->min.y, 0, renderAreaHeight);
            auto captureAreaMaxX = std::clamp<int>(captureArea->max.x, captureAreaMinX, renderAreaWidth);
            auto captureAreaMaxY = std::clamp<int>(captureArea->max.y, captureAreaMinY, renderAreaHeight);
            if (captureAreaMinX >= captureAreaMaxX || captureAreaMinY >= captureAreaMaxY)
                return nullptr;

            capture.region = base::Rect(captureAreaMinX, captureAreaMinY, captureAreaMaxX, captureAreaMaxY);
        }
        else
        {
            capture.region = base::Rect(0, 0, renderAreaWidth, renderAreaHeight);
        }

        // prepare capture context for frame rendering
        auto captureBuffer = base::RefNew<EncodedSelectionDataSink>();

        // prepare capture settings
        capture.sink = captureBuffer;
        capture.mode = rendering::scene::FrameCaptureMode::SelectionRect;

        // render the capture frame
        rendering::scene::Camera captureCamera;
        renderCaptureScene(&capture, &captureCamera);

        // flush GPU
        // TODO: proper sync with GPU
        base::GetService<DeviceService>()->sync();
        base::GetService<DeviceService>()->sync();
        base::GetService<DeviceService>()->sync();

        // wait for data and return it
        return captureBuffer->waitAndFetch(captureCamera);
    }

    bool RenderingScenePanel::queryWorldPositionUnderCursor(const base::Point& localPoint, base::AbsolutePosition& outPosition)
    {
        // query the depth
        const base::Rect captureArea(localPoint.x, localPoint.y, localPoint.x + 1, localPoint.y + 1);
        auto depth = queryDepth(&captureArea);
        if (!depth)
            return false;

        // calculate the world position
        return depth->calcWorldPosition(localPoint.x, localPoint.y, outPosition);
    }

    //--

    class DepthBufferDataSink : public rendering::IDownloadDataSink
    {
    public:
        DepthBufferDataSink(const base::Rect& screenRect)
            : m_screenRect(screenRect)
        {
            //m_fence = Fibers::GetInstance().createCounter("WaitForDepthData", 1);

            const auto numPixels = m_screenRect.width() * m_screenRect.height();
            m_depth.resizeWith(numPixels, 1.0f); // far plane
        }
        
        virtual void processRetreivedData(const void* dataPtr, uint32_t dataSize, const rendering::ResourceCopyRange& info) override final
        {
            const auto numPixels = range_cast<uint32_t>(dataSize / sizeof(float));
            DEBUG_CHECK_EX(numPixels == m_depth.size(), "Unexpected pixel count in retrieved data"); // NOTE: we can't use this data as it's probably of different pitch any way

            if (numPixels == m_depth.size())
                memcpy(m_depth.data(), dataPtr, dataSize);

            //Fibers::GetInstance().signalCounter(m_fence, 1);
        }

        base::RefPtr<RenderingPanelDepthBufferQuery> waitAndFetch(const rendering::scene::Camera& camera, uint32_t width, uint32_t height, bool flipped)
        {
            //Fibers::GetInstance().waitForCounterAndRelease(m_fence);
            return base::RefNew<RenderingPanelDepthBufferQuery>(width, height, camera, m_screenRect, std::move(m_depth), flipped);
        }

    private:
        base::Array<float> m_depth;
        base::Rect m_screenRect;

        base::fibers::WaitCounter m_fence;
    };


    base::RefPtr<RenderingPanelDepthBufferQuery> RenderingScenePanel::queryDepth(const base::Rect* captureArea)
    {
        // render area not cached
        auto renderAreaWidth = (int)cachedDrawArea().size().x;
        auto renderAreaHeight = (int)cachedDrawArea().size().y;

        // clamp the area
        rendering::scene::FrameParams_Capture capture;
        if (captureArea)
        {
            auto captureAreaMinX = std::clamp<int>(captureArea->min.x, 0, renderAreaWidth);
            auto captureAreaMinY = std::clamp<int>(captureArea->min.y, 0, renderAreaHeight);
            auto captureAreaMaxX = std::clamp<int>(captureArea->max.x, captureAreaMinX, renderAreaWidth);
            auto captureAreaMaxY = std::clamp<int>(captureArea->max.y, captureAreaMinY, renderAreaHeight);
            if (captureAreaMinX >= captureAreaMaxX || captureAreaMinY >= captureAreaMaxY)
                return nullptr;

            capture.region = base::Rect(captureAreaMinX, captureAreaMinY, captureAreaMaxX, captureAreaMaxY);
        }
        else
        {
            capture.region = base::Rect(0, 0, renderAreaWidth, renderAreaHeight);
        }

        // prepare capture context for frame rendering
        auto captureImage = base::RefNew<DepthBufferDataSink>(capture.region);

        // prepare capture settings
        capture.sink = captureImage;
        capture.mode = rendering::scene::FrameCaptureMode::DepthRect;

        // render the capture frame
        rendering::scene::Camera captureCamera;
        renderCaptureScene(&capture, &captureCamera);

        // flush GPU
        // TODO: proper sync with GPU
        base::GetService<DeviceService>()->sync();
        base::GetService<DeviceService>()->sync();
        base::GetService<DeviceService>()->sync();

        // wait for data and return it
        return captureImage->waitAndFetch(captureCamera, renderAreaWidth, renderAreaHeight, false);
    }

    //--

	void RenderingScenePanel::renderContent(const ViewportParams& viewport, rendering::scene::Camera* outCameraUsedToRender)
    {
        // compute camera
        rendering::scene::CameraSetup cameraSetup;
        handleCamera(cameraSetup);

        // create camera
        rendering::scene::Camera camera;
        camera.setup(cameraSetup);

        // report computed camera
        if (outCameraUsedToRender)
            *outCameraUsedToRender = camera;

        // create frame and render scene content into it
        rendering::scene::FrameParams frame(viewport.width, viewport.height, camera);
        if (viewport.capture)
            frame.capture = *viewport.capture;

        // setup params
        frame.index = m_frameIndex++;
        frame.filters = m_panelSettings.filters;
        frame.cascades.numCascades = 3;
        frame.time.engineRealTime = (float)m_engineTimeCounter;
        frame.time.gameTime = (float)m_gameTimeCounter;
        frame.clear.clearColor = base::Vector4(0.2f, 0.2f, 0.4f, 1.0f);
        frame.clear.clear = true;

        // debug stuff
        calculateCurrentPixelUnderCursor(frame.debug.mouseHoverPixel);

        // render to frame
        handleRender(frame);

		// setup render targets for scene rendering
		rendering::scene::FrameCompositionTarget target;
		target.targetColorRTV = viewport.colorBuffer;
		target.targetDepthRTV = viewport.depthBuffer;
		target.targetRect = base::Rect(0, 0, viewport.width, viewport.height);

        // generate command buffers
        if (auto* commandBuffer = base::GetService<rendering::scene::FrameRenderingService>()->renderFrame(frame, target))
        {
            auto device = base::GetService<rendering::DeviceService>()->device();
            device->submitWork(commandBuffer);
        }
    }

    //--    

    void RenderingScenePanel::createToolbarItems()
    {
        actions().bindCommand("PreviewPanel.ChangeRenderMode"_id) = [this](ui::Button* button)
        {
            if (button)
            {
                auto menu = base::RefNew<MenuButtonContainer>();
                buildRenderModePopup(menu);
                menu->showAsDropdown(button);
            }
        };

        actions().bindCommand("PreviewPanel.ChangeFilters"_id) = [this](ui::Button* button)
        {
            if (button)
            {
                auto menu = base::RefNew<MenuButtonContainer>();
                buildFilterPopup(menu);
                menu->showAsDropdown(button);
            }
        };

        actions().bindCommand("PreviewPanel.ChangeCameraMode"_id) = [this](ui::Button* button)
        {
            if (button)
            {
                auto menu = base::RefNew<MenuButtonContainer>();
                buildCameraPopup(menu);
                menu->showAsDropdown(button);
            }
        };

        toolbar()->createButton("PreviewPanel.ChangeFilters"_id, ui::ToolbarButtonSetup().caption("[img:eye] Filters"));
        toolbar()->createButton("PreviewPanel.ChangeRenderMode"_id, ui::ToolbarButtonSetup().caption(RenderModeString(m_panelSettings.renderMode)));
        toolbar()->createButton("PreviewPanel.ChangeCameraMode"_id, ui::ToolbarButtonSetup().caption(CameraModeString(m_cameraController.settings().mode)));
        toolbar()->createSeparator();
    }

    static const rendering::scene::FrameRenderMode USER_SELECTABLE_RENDER_MODES[] = {
    rendering::scene::FrameRenderMode::Default,
    rendering::scene::FrameRenderMode::WireframeSolid,
    rendering::scene::FrameRenderMode::WireframePassThrough,
    rendering::scene::FrameRenderMode::DebugDepth,
    rendering::scene::FrameRenderMode::DebugLuminance,
    rendering::scene::FrameRenderMode::DebugShadowMask,
    rendering::scene::FrameRenderMode::DebugReconstructedViewNormals,
    rendering::scene::FrameRenderMode::DebugAmbientOcclusion,
    };

    void RenderingScenePanel::buildRenderModePopup(ui::MenuButtonContainer* menu)
    {
        for (auto mode : USER_SELECTABLE_RENDER_MODES)
        {
            if (mode == rendering::scene::FrameRenderMode::DebugDepth)
                menu->createSeparator();

            if (auto title = RenderModeString(mode))
                menu->createCallback(title) = [this, mode, title]() {
                    m_panelSettings.renderMode = mode;
                    toolbar()->updateButtonCaption("PreviewPanel.ChangeRenderMode"_id, ui::ToolbarButtonSetup().caption(title));
                };
        }

        menu->createSeparator();
    }

    void RenderingScenePanel::createFilterItem(base::StringView prefix, const rendering::scene::FilterBitInfo* bitInfo, MenuButtonContainer* menu)
    {
        base::StringBuf name = prefix ? base::StringBuf(base::TempString("{}.{}", prefix, bitInfo->name)) : base::StringBuf(bitInfo->name.view());

        if (bitInfo->bit != rendering::scene::FilterBit::MAX)
        {
            const auto toggled = m_panelSettings.filters.test(bitInfo->bit);

            menu->createCallback(name, toggled ? "[img:tick]" : "") = [this, bitInfo]()
            {
                m_panelSettings.filters ^= bitInfo->bit;
            };
        }

        for (const auto* child : bitInfo->children)
            createFilterItem(name, child, menu);
    }

    void RenderingScenePanel::buildFilterPopup(MenuButtonContainer* menu)
    {
        if (const auto* group = rendering::scene::GetFilterTree())
        {
            for (const auto* child : group->children)
            {
                menu->createSeparator();
                createFilterItem("", child, menu);
            }
        }
    }

    static const CameraMode USER_SELECTABLE_CAMERA_MODES[] = {
        CameraMode::FreePerspective,
        CameraMode::OrbitPerspective,
        CameraMode::FreeOrtho,
        CameraMode::Front,
        CameraMode::Back,
        CameraMode::Left,
        CameraMode::Right,
        CameraMode::Top,
        CameraMode::Bottom,
    };

    void RenderingScenePanel::buildCameraPopup(MenuButtonContainer* menu)
    {
        auto mode = m_cameraController.settings().mode;

        if (mode == CameraMode::FreePerspective || mode == CameraMode::FreeOrtho)
        {
            auto label = menu->createChild<ui::TextLabel>("[img:speed] Camera speed [log2 m/s]:");
            label->customMargins(10, 10, 10, 2);

            auto slider = menu->createChild<ui::TrackBar>();
            slider->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            slider->range(-2.0f, 5.0f);
            //slider->units("log2 m/s");
            slider->value(m_cameraController.settings().speedLog);
            menu->createSeparator();

            slider->bind(EVENT_TRACK_VALUE_CHANGED) = [this](double val)
            {
                auto camera = m_cameraController.settings();
                camera.speedLog = val;
                cameraSettings(camera);
            };
        }

        if (m_cameraController.settings().perspective())
        {
            auto label = menu->createChild<ui::TextLabel>("[img:angle] Camera FOV [deg]:");
            label->customMargins(5, 5, 5, 2);

            auto slider = menu->createChild<ui::TrackBar>();
            slider->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            slider->range(10.0f, 170.0f);
            //slider->units("deg");
            slider->value(m_cameraController.settings().perspectiveFov);
            menu->createSeparator();

            slider->bind(EVENT_TRACK_VALUE_CHANGED) = [this](double val)
            {
                auto camera = m_cameraController.settings();
                camera.perspectiveFov = val;
                cameraSettings(camera);
            };
        }

        if (m_cameraController.settings().ortho())
        {
            auto label = menu->createChild<ui::TextLabel>("[img:zoom] Camera Zoom [m]:");
            label->customMargins(5, 5, 5, 2);

            auto slider = menu->createChild<ui::TrackBar>();
            slider->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
            slider->range(0.1f, 100.0f);
            //slider->units("m");
            slider->value(m_cameraController.settings().orthoZoom);
            menu->createSeparator();

            slider->bind(EVENT_TRACK_VALUE_CHANGED) = [this](double val)
            {
                auto camera = m_cameraController.settings();
                camera.orthoZoom = val;
                cameraSettings(camera);
            };
        }

        auto label = menu->createChild<ui::TextLabel>("Camera mode:");
        label->customMargins(5, 5, 5, 2);

        for (const auto newMode : USER_SELECTABLE_CAMERA_MODES)
        {
            if (newMode == CameraMode::Front)
                menu->createSeparator();

            if (auto title = CameraModeString(newMode))
                menu->createCallback(title) = [this, title, newMode]() {

                auto camera = m_cameraController.settings();
                camera.mode = newMode;
                cameraSettings(camera);

                toolbar()->updateButtonCaption("PreviewPanel.ChangeCameraMode"_id, ui::ToolbarButtonSetup().caption(title));
            };
        }

        menu->createSeparator();
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(RenderingSimpleScenePanel);
    RTTI_END_TYPE();

    //----

    RenderingSimpleScenePanel::RenderingSimpleScenePanel()
    {
        const auto sceneType = rendering::scene::SceneType::EditorPreview;
        m_scene = base::RefNew<rendering::scene::Scene>(sceneType);
    }

    RenderingSimpleScenePanel::~RenderingSimpleScenePanel()
    {
        m_scene.reset();
    }

    void RenderingSimpleScenePanel::handleRender(rendering::scene::FrameParams& frame)
    {
        TBaseClass::handleRender(frame);
        frame.scenes.mainScenePtr = m_scene;
    }

    //--

} // ui