/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#include "build.h"
#include "renderingScenePanel.h"

#include "rendering/driver/include/renderingCommandBuffer.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/scene/include/renderingFrameParams.h"
#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingSelectable.h"
#include "rendering/scene/include/renderingFrameRenderingService.h"
#include "rendering/scene/include/renderingFrameCameraContext.h"

#include "base/ui/include/uiInputAction.h"
#include "base/canvas/include/canvasGeometry.h"
#include "base/canvas/include/canvasGeometryBuilder.h"
#include "base/canvas/include/canvas.h"

namespace ui
{

    //--

    RTTI_BEGIN_TYPE_CLASS(RenderingScenePanel);
    RTTI_END_TYPE();

    //----

    RenderingPanelSelectionQuery::RenderingPanelSelectionQuery(const base::Rect& area, base::Array<rendering::scene::EncodedSelectable> entries)
        : m_area(area)
        , m_entries(std::move(entries))
    {
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
                if (raw.position().squaredDistanceTo(point) <= selectionRange)
                {
                    if (raw.SelectableDepth <= bestDepth)
                    {
                        bestSelectable = rendering::scene::Selectable(raw);
                        bestDepth = raw.SelectableDepth;
                    }
                }
            }
        }

        // TODO: modes

        // nothing found
        if (bestSelectable.empty())
            return false;

        // return found selectable
        outSelectable = bestSelectable;
        return true;
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
                if (clippedRect.contains(raw.position()))
                    outSelectables.insert(rendering::scene::Selectable(raw));
            }
        }
        else if (mode == AreaSelectionMode::ExcludeBorder)
        {
            // get the inset
            auto innerSize = clippedRect.inner(base::Point(1, 1));

            // collect all elements
            for (const auto& raw : m_entries)
            {
                if (innerSize.contains(raw.position()))
                    outSelectables.insert(rendering::scene::Selectable(raw));
            }

            // remove the border elements
            for (const auto& raw : m_entries)
            {
                auto rawPos = raw.position();
                if (clippedRect.contains(rawPos) && !innerSize.contains(rawPos))
                    outSelectables.remove(rendering::scene::Selectable(raw));
            }
        }
        else
        {
            // TODO: other modes
        }

        return true;
    }

    //---

    RenderingPanelDepthBufferQuery::RenderingPanelDepthBufferQuery(uint32_t width, uint32_t height, const rendering::scene::Camera& camera, const base::image::ImagePtr& depthImage)
        : m_width(width)
        , m_height(height)
        , m_camera(camera)
        , m_depthImage(depthImage)
    {}

    bool RenderingPanelDepthBufferQuery::calcWorldPosition(int x, int y, base::AbsolutePosition& outPos) const
    {
        if (x < 0 || y < 0 || x >= (int)m_width || y >= (int)m_height)
            return false;

        // get adjusted pixel
        auto adjY = (m_depthImage->height() - 1) - y;
        auto width = (m_depthImage->width());

        // get depth at pixel
        const auto* depthValues = (const float*)m_depthImage->data();
        auto depth = depthValues[x + adjY * width];
        TRACE_INFO("Depth under cursor: {}", depth);
        if (depth == 1.0f) // far plane, nothing rendered here
            return false;

        // convert to screen coords
        float sx = -1.0f + 2.0f * (x / (float)(m_width - 1));
        float sy = -1.0f + 2.0f * (y / (float)(m_height - 1));

        // convert to linear Z
        //const auto& viewToScreen = m_camera.viewToScreen();
        //auto linearDepth = viewToScreen.m[2][3] / (depth - viewToScreen.m[2][2]);

        // convert to world position
        base::Vector4 screenPos(sx, sy, depth, 1.0f);
        auto worldPos = m_camera.screenToWorld().transformVector4(screenPos);
        if (worldPos.w == 0.0f)
            return false;

        outPos = base::AbsolutePosition(worldPos.projected().xyz());
        return true;
    }

    //----

    base::ConfigProperty<float> cvViewportAxisLength("Editor", "ViewportAxisLength", 40.0f);
    base::ConfigProperty<float> cvViewportUpdateInterval("Editor", "ViewportUpdateInterval", 1000.0f);

    //---

    RenderingScenePanel::RenderingScenePanel()
        : m_pointSelectionMode(PointSelectionMode::Closest)
        , m_areaSelectionMode(AreaSelectionMode::ExcludeBorder)
        , m_cameraForceOrbitModeFlag(false)
        , m_updateTimer(this, "Update"_id)
    {
        m_cameraContext = base::CreateSharedPtr<rendering::scene::CameraContext>("PreviewPanelCamera");

        m_lastUpdateTime.resetToNow();
        m_updateTimer = [this]() {
            auto dt = m_lastUpdateTime.timeTillNow().toSeconds();
            m_lastUpdateTime.resetToNow();
            handleUpdate(dt);
        };

        m_updateTimer.startRepeated(1.0f / cvViewportUpdateInterval.get());

        m_filterFlags = rendering::scene::FilterFlags::DefaultEditor();
    }

    RenderingScenePanel::~RenderingScenePanel()
    {}

    bool RenderingScenePanel::computeContentBounds(base::Box& outBox) const
    {
        return false;
    }

    void RenderingScenePanel::areaSelectionMode(AreaSelectionMode mode)
    {
        m_areaSelectionMode = mode;
    }

    void RenderingScenePanel::pointSelectionMode(PointSelectionMode mode)
    {
        m_pointSelectionMode = mode;
    }

    void RenderingScenePanel::cameraSpeedFactor(float speedFactor)
    {
        m_cameraController.speedFactor(speedFactor);
    }

    void RenderingScenePanel::setupCameraForceOrbitMode(bool flag)
    {
        m_cameraForceOrbitModeFlag = flag;
    }

    void RenderingScenePanel::setupCamera(const base::Angles& rotation, const base::Vector3& position)
    {
        m_cameraController.moveTo(position, rotation);
    }

    void RenderingScenePanel::setupCameraAtAngle(const base::Angles& rotation, float distance)
    {
        auto pos = -rotation.forward() * distance;
        m_cameraController.moveTo(pos, rotation);
    }

    void RenderingScenePanel::setupCameraAroundBounds(const base::Box& bounds, float distanceFactor /*= 1.0f*/, const base::Angles* newRotation/*=nullptr*/)
    {
        // get new rotation or use the existing one
        auto rotation = newRotation ? *newRotation : m_cameraController.rotation();

        // compute the maximum object size
        auto distance = bounds.extents().length() * 3.0f * distanceFactor;
        auto position = bounds.center() - distance * rotation.forward();
        m_cameraController.moveTo(position, rotation);
    }

    base::Point RenderingScenePanel::clientPositionFromNormalizedPosition(const base::Vector3& normalizedPosition) const
    {
        auto viewportSize = cachedDrawArea().size();
        auto cx = normalizedPosition.x * viewportSize.x;
        auto cy = normalizedPosition.y * viewportSize.y;
        return base::Point(cx, cy);
    }

    base::Vector3 RenderingScenePanel::normalizedScreenPosition(int x, int y, float z) const
    {
        auto viewportSize = cachedDrawArea().size();
        auto wx = viewportSize.x > 0 ? (x / viewportSize.x) : 0.0f;
        auto wy = viewportSize.y > 0 ? (y / viewportSize.y) : 0.0f;
        return base::Vector3(wx, wy, z);
    }

    bool RenderingScenePanel::worldSpaceRayForClientPixelExact(int x, int y, base::AbsolutePosition& outStart, base::Vector3& outDir) const
    {
        const auto& camera = cachedCamera();

        auto coords = normalizedScreenPosition(x, y, 0.5f);
        base::Vector3 start;
        if (!camera.calcWorldSpaceRay(coords, start, outDir))
            return false;

        outStart = base::AbsolutePosition(start, base::Vector3());
        return true;
    }

    bool RenderingScenePanel::worldSpaceRayForClientPixel(int x, int y, base::Vector3& outStart, base::Vector3& outDir) const
    {
        const auto& camera = cachedCamera();

        auto coords = normalizedScreenPosition(x, y, 0.5f);
        return camera.calcWorldSpaceRay(coords, outStart, outDir);
    }

    bool RenderingScenePanel::screenToWorld(const base::Vector3* normalizedScreenPos, base::AbsolutePosition* outWorldPosition, uint32_t count) const
    {
        const auto& camera = cachedCamera();

        for (uint32_t i = 0; i < count; ++i)
        {
            base::Vector3 worldPos;
            if (!camera.projectWorldToScreen(normalizedScreenPos[i], worldPos))
                return false;

            outWorldPosition[i] = base::AbsolutePosition(worldPos, base::Vector3::ZERO());
        }

        return true;
    }

    bool RenderingScenePanel::worldToScreen(const base::AbsolutePosition* worldPosition, base::Vector3* outScreenPosition, uint32_t count) const
    {
        const auto& camera = cachedCamera();

        for (uint32_t i = 0; i < count; ++i)
        {
            base::Vector3 screenPos;
            auto simpleWorldPos = worldPosition[i].approximate();
            if (!camera.projectWorldToScreen(simpleWorldPos, screenPos))
                return false;

            outScreenPosition[i] = screenPos;
        }

        return true;
    }

    bool RenderingScenePanel::worldToClient(const base::AbsolutePosition* worldPosition, base::Vector3* outClientPosition, uint32_t count) const
    {
        const auto& camera = cachedCamera();

        auto viewportSize = cachedDrawArea().size();
        auto viewportWidthScale = viewportSize.x;
        auto viewportHeightScale = viewportSize.y;

        for (uint32_t i = 0; i < count; ++i)
        {
            base::Vector3 screenPos;
            auto simpleWorldPos = worldPosition[i].approximate();
            if (!camera.projectWorldToScreen(simpleWorldPos, screenPos))
                return false;

            outClientPosition[i].x = screenPos.x * viewportWidthScale;
            outClientPosition[i].y = screenPos.y * viewportHeightScale;
            outClientPosition[i].z = screenPos.z; // no conversion
        }

        return true;
    }

    float RenderingScenePanel::calculateViewportScaleFactor(const base::AbsolutePosition& worldPosition, bool useDPI) const
    {
        const auto& camera = cachedCamera();

        auto viewportSize = cachedDrawArea().size();
        auto viewportWidthScale = viewportSize.x;
        auto viewportHeightScale = viewportSize.y;

        auto approximateWorldPos = worldPosition.approximate();
        return camera.calcScreenSpaceScalingFactor(approximateWorldPos, viewportWidthScale, viewportHeightScale) * 1.0f;// cachedStyleParams().m_scale;
    }

    //--

    void RenderingScenePanel::handleUpdate(float dt)
    {
        m_cameraController.animate(dt);
        m_engineTimeCounter += dt;
        m_gameTimeCounter += dt;
    }

    bool RenderingScenePanel::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (m_cameraController.processKeyEvent(evt))
            return true;

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

            SelectionClickInputHandler(IElement* ptr, CameraController* camera, const base::Point& startPoint, const TSelectionFunction& selectionFunc)
                : IInputAction(ptr)
                , m_startPoint(startPoint)
                , m_selectionFunction(selectionFunc)
                , m_camera(camera)
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
                    return nullptr;
                }
                else if (evt.rightClicked())
                {
                    return m_camera->handleGeneralFly(element(), 3);
                }
                else
                {
                    return nullptr;
                }
            }

            virtual InputActionResult onMouseMovement(const base::input::MouseMovementEvent& evt, const ElementWeakPtr& hoveredElement) override
            {
                auto dist = evt.absolutePosition().distanceTo(m_startPoint);
                if (dist >= 4.0f)
                    return m_camera->handleGeneralFly(element(), 1);

                return InputActionResult();
            }

        private:
            CameraController* m_camera;
            TSelectionFunction m_selectionFunction;
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
                    base::canvas::GeometryBuilder b;
                    b.fillColor(base::Color(50, 50, 180, 100));
                    b.rect(0.0f, 0.0f, rect.width(), rect.height());
                    b.fill();

                    auto rectPlacement = rect.min.toVector() + element()->cachedDrawArea().absolutePosition();

                    auto area = element()->cachedDrawArea();
                    canvas.placement(rectPlacement.x, rectPlacement.y);
                    canvas.place(b);
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
        m_cameraController.resetInput();
    }

    InputActionPtr RenderingScenePanel::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (evt.keyMask().isCtrlDown() && evt.rightClicked())
        {
            auto rotateFunc = [this](float dp, float dy) { rotateGlobalLight(dp, dy); };
            return base::CreateSharedPtr<helper::MouseLightRotation>(this, rotateFunc);
        }
        else  if (evt.leftClicked() && !evt.keyMask().isAltDown())
        {
            if (m_cameraForceOrbitModeFlag)
            {
                if (auto ret = m_cameraController.handleOrbitAroundPoint(this, 1, base::AbsolutePosition(0,0,0)))
                {
                    m_renderInputAction = ret;
                    return ret;
                }
            }
            else
            {
                // create the selection input
                if (evt.keyMask().isShiftDown())
                {
                    // create an area selection mode
                    auto selectionFunc = [this](bool ctrl, bool shift, const base::Rect& rect) { handleAreaSelection(ctrl, shift, rect); };
                    return base::CreateSharedPtr<helper::SelectionAreaInputHandler>(this, evt.absolutePosition(), selectionFunc);
                }
                else
                {
                    // create a point selection mode
                    // NOTE: this can mutate into the camera movement
                    auto selectionFunc = [this](bool ctrl, bool shift, const base::Point& point) { handlePointSelection(ctrl, shift, point); };
                    return base::CreateSharedPtr<helper::SelectionClickInputHandler>(this, &m_cameraController, evt.absolutePosition(), selectionFunc);
                }
            }
        }
        else if (evt.rightClicked())
        {
            if (m_cameraForceOrbitModeFlag)
            {
                if (auto ret = m_cameraController.handleOrbitAroundPoint(this, 2, base::AbsolutePosition(0, 0, 0)))
                {
                    m_renderInputAction = ret;
                    return ret;
                }
            }
            else
            {
                // go to the camera input directly
                return m_cameraController.handleGeneralFly(this, 2);
            }
        }
        else if (evt.midClicked())
        {
            // orbit around point
            if (evt.keyMask().isAltDown())
            {
                // orbit around point
                base::AbsolutePosition worldOrbitPosition;
                auto viewportPos = (evt.absolutePosition() - cachedDrawArea().absolutePosition());
                if (queryWorldPositionUnderCursor(viewportPos, worldOrbitPosition))
                {
                    if (auto ret = m_cameraController.handleOrbitAroundPoint(this, 4, worldOrbitPosition))
                    {
                        m_renderInputAction = ret;
                        return ret;
                    }
                }
            }

            // go to the camera input directly
            return m_cameraController.handleGeneralFly(this, 4);
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

    const rendering::scene::Camera& RenderingScenePanel::cachedCamera() const
    {
        if (m_cameraController.stamp() != m_cachedCameraTimestamp)
        {
            auto nonConstSelf = const_cast<RenderingScenePanel*>(this);

            rendering::scene::CameraSetup setup;
            nonConstSelf->handleCamera(setup);

            m_cachedCamera.setup(setup);
            m_cachedCameraTimestamp = m_cameraController.stamp();
        }

        return m_cachedCamera;
    }

    void RenderingScenePanel::handleCamera(rendering::scene::CameraSetup& outCamera)
    {
        // setup camera
        m_cameraController.computeRenderingCamera(outCamera);

        // render area not cached
        const auto& renderArea = cachedDrawArea(); // TODO: would be perfect not to use cached shit
        auto renderAreaWidth = range_cast<uint32_t>(renderArea.size().x);
        auto renderAreaHeight = range_cast<uint32_t>(renderArea.size().y);

        // setup camera's aspect ratio because the camera controller is not aware of it
        //outCamera.aspect = (float)renderAreaWidth / (float)renderAreaHeight;
        outCamera.aspect = (float)renderAreaWidth / (float)renderAreaHeight;
    }

    void RenderingScenePanel::handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables)
    {
        // nothing
    }

    void RenderingScenePanel::handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
    {
        // nothing
    }

    void RenderingScenePanel::handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition)
    {
        // compute selection area around pixel
        int radius = 4;
        base::Rect selectionArea(clientPosition.x - radius, clientPosition.y - radius, clientPosition.x + radius, clientPosition.y + radius);

        // query the raw selection data from the rendering
        auto selectionData = querySelection(selectionArea);

        // build selectables
        base::Array<rendering::scene::Selectable> selectables;
        if (selectionData)
        {
            rendering::scene::Selectable singleSelectable;
            if (selectionData->extractPointSelection(m_pointSelectionMode, clientPosition, singleSelectable))
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
        auto selectionData = querySelection(clientRect);

        // build selectables
        base::HashSet<rendering::scene::Selectable> selectables;
        if (selectionData)
        {
            selectionData->extractAreaSelection(m_areaSelectionMode, clientRect, selectables);
        }

        // handle the selection with the list of selectables
        handleAreaSelection(ctrl, shift, clientRect, selectables.keys());
    }

    void RenderingScenePanel::handleRender(rendering::scene::FrameParams& frame)
    {
        // draw grid
        if (m_drawInternalGrid && (frame.filters & rendering::scene::FilterBit::ViewportWorldGrid))
            drawGrid(frame);

        // draw world axes
        if (m_drawInternalWorldAxis && (frame.filters & rendering::scene::FilterBit::ViewportWorldAxes))
        {
            rendering::scene::DebugLineDrawer dd(frame.geometry.solid);
            dd.axes(base::Matrix::IDENTITY(), 1.0f);
        }

        // draw screen axis
        if (m_drawInternalCameraAxis && (frame.filters & rendering::scene::FilterBit::ViewportCameraAxes))
            drawViewAxes(frame);

        // draw camera info
        if (m_drawInternalCameraData && (frame.filters & rendering::scene::FilterBit::ViewportCameraInfo))
            drawCameraInfo(frame);

        // draw input action
        //if (auto action = m_renderInputAction.lock())
          //  action->onRender3D(frame);
    }

    void RenderingScenePanel::drawGrid(rendering::scene::FrameParams& frame)
    {
        static base::Array<base::Vector3> points;

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
                        points.emplaceBack(-gridSize, pos, 0.0f);
                        points.emplaceBack(gridSize, pos, 0.0f);
                        points.emplaceBack(pos, -gridSize, 0.0f);
                        points.emplaceBack(pos, gridSize, 0.0f);
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
                        points.emplaceBack(-gridSize, pos, 0.0f);
                        points.emplaceBack(gridSize, pos, 0.0f);
                        points.emplaceBack(pos, -gridSize, 0.0f);
                        points.emplaceBack(pos, gridSize, 0.0f);
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
                        points.emplaceBack(-gridSize, pos, 0.0f);
                        points.emplaceBack(gridSize, pos, 0.0f);
                        points.emplaceBack(pos, -gridSize, 0.0f);
                        points.emplaceBack(pos, gridSize, 0.0f);
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
                        points.emplaceBack(-gridSize, pos, 0.0f);
                        points.emplaceBack(gridSize, pos, 0.0f);
                        points.emplaceBack(pos, -gridSize, 0.0f);
                        points.emplaceBack(pos, gridSize, 0.0f);
                    }
                }
            }
        }

        {
            rendering::scene::DebugLineDrawer dd(frame.geometry.solid);
            dd.color(base::Color(100, 100, 100));
            dd.lines(points.typedData(), points.size());

            base::Vector3 centerLines[4];
            centerLines[0] = base::Vector3(-1000.0f, 0.0f, 0.0f);
            centerLines[1] = base::Vector3(1000.0f, 0.0f, 0.0f);
            centerLines[2] = base::Vector3(0.0f, -1000.0f, 0.0f);
            centerLines[3] = base::Vector3(0.0f, 1000.0f, 0.0f);
            dd.color(base::Color(130, 150, 150));
            dd.lines(centerLines, 4);
        }
    }

    void RenderingScenePanel::drawViewAxes(rendering::scene::FrameParams& frame)
    {
        // calculate placement
        auto scale = cvViewportAxisLength.get() * 1.0f;// cachedStyleParams().m_scale;
        auto bx = 15.0f + scale;
        auto by = frame.resolution.finalCompositionHeight - 15.0f - scale;

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

    void RenderingScenePanel::drawCameraInfo(rendering::scene::FrameParams& frame)
    {
        auto bx = frame.resolution.finalCompositionWidth - 20;
        auto by = frame.resolution.finalCompositionHeight - 20;

        auto pos = m_cameraController.position();
        auto rot = m_cameraController.rotation();

        auto params = rendering::scene::DebugTextParams().right().bottom().color(base::Color::WHITE);
        rendering::scene::DebugSolidDrawer dd(frame.geometry.screen);
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

    base::RefPtr<RenderingPanelSelectionQuery> RenderingScenePanel::querySelection(const base::Rect& area)
    {
        // render area not cached
        auto renderAreaWidth = (int)cachedDrawArea().size().x;
        auto renderAreaHeight = (int)cachedDrawArea().size().y;

        // clamp the area
        auto captureAreaMinX = std::clamp<int>(area.min.x, 0, renderAreaWidth);
        auto captureAreaMinY = std::clamp<int>(area.min.y, 0, renderAreaHeight);
        auto captureAreaMaxX = std::clamp<int>(area.max.x, captureAreaMinX, renderAreaWidth);
        auto captureAreaMaxY = std::clamp<int>(area.max.y, captureAreaMinY, renderAreaHeight);
        if (captureAreaMinX >= captureAreaMaxX || captureAreaMinY >= captureAreaMaxY)
            return nullptr;

        // prepare capture context for frame rendering
        auto captureBuffer = base::CreateSharedPtr<rendering::DownloadBuffer>();

        // prepare capture settings
        rendering::scene::FrameParams_Capture capture;
        capture.area = base::Rect(captureAreaMinX, captureAreaMinY, captureAreaMaxX, captureAreaMaxY);
        capture.dataBuffer = captureBuffer;
        capture.mode = rendering::scene::FrameCaptureMode::SelectionRect;

        // render the frame
        //renderScene()

        // buffer with data must be available
        if (!captureBuffer->isReady())
        {
            TRACE_WARNING("DocumentObjectIDSet buffer not ready after selection rendering");
            return nullptr;
        }

        // no data
        if (!captureBuffer->data())
        {
            TRACE_WARNING("No data extracted into selection buffer");
            return nullptr;
        }

        // get the raw data
        auto maxEntires = range_cast<uint32_t>(captureBuffer->data().size() / sizeof(rendering::scene::EncodedSelectable));
        const auto* ptr = (const rendering::scene::EncodedSelectable*) captureBuffer->data().data();

        // process the raw data
        base::Array<rendering::scene::EncodedSelectable> allSelectables;
        allSelectables.reserve(maxEntires);
        auto selectionRect = base::Rect::EMPTY();
        for (uint32_t i = 0; i < maxEntires; ++i, ++ptr)
        {
            if (ptr->SelectableId)
            {
                allSelectables.pushBack(*ptr);
                selectionRect.merge(ptr->position());
            }
        }

        // no data
        if (allSelectables.empty())
            return nullptr;

        // create wrapper
        return base::CreateSharedPtr<RenderingPanelSelectionQuery>(capture.area, std::move(allSelectables));
    }

    bool RenderingScenePanel::queryWorldPositionUnderCursor(const base::Point& localPoint, base::AbsolutePosition& outPosition)
    {
        // query the depth
        // TODO: only the point!
        auto depth = queryDepth();
        if (!depth)
            return false;

        // calculate the world position
        return depth->calcWorldPosition(localPoint.x, localPoint.y, outPosition);
    }

    base::RefPtr<RenderingPanelDepthBufferQuery> RenderingScenePanel::queryDepth()
    {
        // render area not cached
        auto renderAreaWidth = (int)cachedDrawArea().size().x;
        auto renderAreaHeight = (int)cachedDrawArea().size().y;

        // prepare capture context for frame rendering
        auto captureImage = base::CreateSharedPtr<rendering::DownloadImage>();

        // prepare capture settings
        rendering::scene::FrameParams_Capture capture;
        capture.area = base::Rect(0, 0, renderAreaWidth, renderAreaHeight);
        capture.imageBuffer = captureImage;
        capture.mode = rendering::scene::FrameCaptureMode::DepthRect;

        // render the frame
        //renderInternalScene(cachedDrawArea().size(), captureContext);

        // buffer with data must be available
        if (!captureImage->isReady())
        {
            TRACE_WARNING("DepthBuffer not ready after capture rendering");
            return nullptr;
        }

        // no data
        auto depthImage = captureImage->data();
        if (!depthImage)
        {
            TRACE_WARNING("No data extracted from depth buffer");
            return nullptr;
        }

        // calculate camera
        rendering::scene::CameraSetup cameraSetup;
        handleCamera(cameraSetup);
        rendering::scene::Camera camera;
        camera.setup(cameraSetup);

        // create wrapper
        return base::CreateSharedPtr<RenderingPanelDepthBufferQuery>(renderAreaWidth, renderAreaHeight, camera, depthImage);
    }

    //--

    void RenderingScenePanel::renderScene(const rendering::ImageView& color, const rendering::ImageView& depth, uint32_t width, uint32_t height, const rendering::scene::FrameParams_Capture* capture /*= nullptr*/)
    {
        // compute camera
        rendering::scene::CameraSetup cameraSetup;
        handleCamera(cameraSetup);

        // create camera
        rendering::scene::Camera camera;
        camera.setup(cameraSetup);

        // create frame and render scene content into it
        rendering::scene::FrameParams frame(width, height, camera);
        if (capture)
            frame.capture = *capture;

        // setup params
        frame.index = m_frameIndex++;
        frame.filters = m_filterFlags;
        frame.time.engineRealTime = (float)m_engineTimeCounter;
        frame.time.gameTime = (float)m_gameTimeCounter;

        // debug stuff
        calculateCurrentPixelUnderCursor(frame.debug.mouseHoverPixel);

        // render to frame
        handleRender(frame);

        // generate command buffers
        if (auto* commandBuffer = base::GetService<rendering::scene::FrameRenderingService>()->renderFrame(frame, color))
            device()->submitWork(commandBuffer);
    }

    //--

} // ui