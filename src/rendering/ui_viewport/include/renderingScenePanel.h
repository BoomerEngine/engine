/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#pragma once

#include "cameraController.h"

#include "rendering/scene/include/renderingFrameCamera.h"
#include "rendering/scene/include/renderingSelectable.h"
#include "rendering/scene/include/renderingFrameFilters.h"
#include "rendering/scene/include/renderingFrameParams.h"
#include "rendering/ui_host/include/renderingPanel.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

/// selection mode for point selection
enum class PointSelectionMode : uint8_t
{
    // select exactly the object under the pixel
    Exact,

    // select the object with the largest area in the selection region
    LargestArea,

    // select the object with the smallest area in the selection region
    SmallestArea,

    // select the object that is the closest of all in the area
    Closest,
};

/// selection mode for area selection
enum class AreaSelectionMode : uint8_t
{
    // select everything
    Everything,

    // exclude objects at the border
    ExcludeBorder,

    // choose the object with largest area
    LargestArea,

    // choose the object with smallest area
    SmallestArea,

    // choose the closest object
    Closest,
};

/// selection query
class RENDERING_UI_VIEWPORT_API RenderingPanelSelectionQuery : public base::IReferencable
{
public:
    RenderingPanelSelectionQuery(const base::Rect& area, base::Array<rendering::scene::EncodedSelectable> entries, const rendering::scene::Camera& camera);

    /// get camera data used during rendering
    INLINE const rendering::scene::Camera& camera() const { return m_camera; }

    /// get the selection area
    INLINE const base::Rect& area() const { return m_area; }

    /// get the captured selection entries
    INLINE const base::Array<rendering::scene::EncodedSelectable>& entries() const { return m_entries; }

    /// extract selection list for given point, selects best selectable at given position
    bool extractPointSelection(PointSelectionMode mode, const base::Point& point, rendering::scene::Selectable& outSelectable) const;

    /// extract selection for given area
    bool extractAreaSelection(AreaSelectionMode mode, const base::Rect& rect, base::HashSet<rendering::scene::Selectable>& outSelectables) const;

private:
    friend class RenderingPanel;

    base::Rect m_area; // the selection area to extract
    base::Array<rendering::scene::EncodedSelectable> m_entries;

    rendering::scene::Camera m_camera;
};

///---

/// depth buffer query
class RENDERING_UI_VIEWPORT_API RenderingPanelDepthBufferQuery : public base::IReferencable
{
public:
    RenderingPanelDepthBufferQuery(uint32_t width, uint32_t height, const rendering::scene::Camera& camera, const base::Rect& validDepthValues, base::Array<float>&& depthValues, bool flipped);

    /// get captured resolution
    INLINE uint32_t width() const { return m_width; }
    INLINE uint32_t height() const { return m_height; }

    /// get the camera used to capture the depth buffer
    INLINE const rendering::scene::Camera& camera() const { return m_camera; }

    /// get the range of client space pixels for which we have depth values
    INLINE const base::Rect& depthValuesRect() const { return m_depthValuesRect; }

    /// get projected depth value at pixel, returns 1.0f for values outside captured area
    float projectedDepthValueAtPixel(int x, int y, bool* withinRect = nullptr) const;

    /// get linear depth value at pixel, returns camera's far plane for values outside captured area
    float linearDepthValueAtPixel(int x, int y, bool* withinRect = nullptr) const;

    /// calculate world position for given pixel
    bool calcWorldPosition(int x, int y, base::AbsolutePosition& outPos) const;

private:
    uint32_t m_width;
    uint32_t m_height;
    bool m_flipped = false;

    rendering::scene::Camera m_camera;
    base::Rect m_depthValuesRect;
    base::Array<float> m_depthValues;
};

//--

/// base panel settings
struct RENDERING_UI_VIEWPORT_API RenderingScenePanelSettings
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(RenderingScenePanelSettings);

public:
    rendering::scene::FrameRenderMode renderMode = rendering::scene::FrameRenderMode::Default;
    base::StringID renderMaterialDebugChannelName;

    rendering::scene::FilterFlags filters;

    float cameraSpeedFactor = 1.0f;
    //bool cameraForceOrbit = false;

    bool paused = false;
    float timeDeltaScale = 1.0f;

    AreaSelectionMode areaSelectionMode = AreaSelectionMode::ExcludeBorder;
    PointSelectionMode pointSelectionMode = PointSelectionMode::Closest;

    bool drawInternalGrid = true;
    bool drawInternalWorldAxis = true;
    bool drawInternalCameraAxis = true;
    bool drawInternalCameraData = true;

    //--

    RenderingScenePanelSettings();
};

//--

/// ui widget capable of rendering a 3D scene with a camera and stuff
class RENDERING_UI_VIEWPORT_API RenderingScenePanel : public RenderingPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingScenePanel, RenderingPanel);

public:
    RenderingScenePanel();
    virtual ~RenderingScenePanel();

    //--

    // get the overlay toolbar
    INLINE const ToolBarPtr& toolbar() const { return m_toolbar; }

    // get the bottom overlay toolbar
    INLINE const ToolBarPtr& bottomToolbar() const { return m_bottomToolbar; }

    /// get last computed camera
    INLINE const rendering::scene::Camera& cachedCamera() const { return m_cachedCamera; }

    // get the panel settings
    INLINE const RenderingScenePanelSettings& panelSettings() const { return m_panelSettings; }

    // get the camera settings
    INLINE const CameraControllerSettings& cameraSettings() const { return m_cameraController.settings(); }

    // change panel settings
    void panelSettings(const RenderingScenePanelSettings& settings);

    // change camera settings
    void cameraSettings(const CameraControllerSettings& settings);

    //--

    // modify camera to show given bounds
    void focusOnBounds(const base::Box& bounds, float distanceFactor /*= 1.0f*/, const base::Angles* newRotation/*=nullptr*/);

    //--

    /// compute bounds of the content in the view
    virtual bool computeContentBounds(base::Box& outBox) const;

    //---

    // query selectable objects from given area
    // TODO: object filters (debug, transparent, etc)
    base::RefPtr<RenderingPanelSelectionQuery> querySelection(const base::Rect* captureArea = nullptr);

    // query depth buffer from given area
    // TODO: object filters (debug, transparent, etc)
    base::RefPtr<RenderingPanelDepthBufferQuery> queryDepth(const base::Rect* captureArea = nullptr);

    //--

protected:
    virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt) override;
    virtual bool handleMouseMovement(const base::input::MouseMovementEvent& evt) override;
    virtual bool handleMouseWheel(const base::input::MouseMovementEvent& evt, float delta) override;
    virtual void handleFocusLost() override;

    virtual void handleUpdate(float dt);
    virtual void handleCamera(rendering::scene::CameraSetup& outCamera);
    virtual void handleRender(rendering::scene::FrameParams& frame);
    virtual void handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition);
    virtual void handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect);
    virtual bool handleContextMenu(const ElementArea& area, const Position& absolutePosition, base::input::KeyMask controlKeys) override;

    virtual void handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables);
    virtual void handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables);
    virtual void handleContextMenu(bool ctrl, bool shift, const Position& absolutePosition, const base::Point& clientPosition, const rendering::scene::Selectable& objectUnderCursor, const base::AbsolutePosition* positionUnderCursor);

    virtual void buildRenderModePopup(MenuButtonContainer* menu);
    virtual void buildFilterPopup(MenuButtonContainer* menu);
    virtual void buildCameraPopup(MenuButtonContainer* menu);

	virtual void renderContent(const ViewportParams& viewport, rendering::scene::Camera* outCameraUsedToRender = nullptr) override;

    void drawViewAxes(uint32_t width, uint32_t height, rendering::scene::FrameParams& frame);
    void drawCameraInfo(uint32_t width, uint32_t height, rendering::scene::FrameParams& frame);
    void drawGrid(rendering::scene::FrameParams& frame);

    void rotateGlobalLight(float deltaPitch, float deltaYaw);

    bool queryWorldPositionUnderCursor(const base::Point& localPoint, base::AbsolutePosition& outPosition);

    //-
       
protected:
    RenderingScenePanelSettings m_panelSettings;

private:
    Timer m_updateTimer;
    base::NativeTimePoint m_lastUpdateTime;

    CameraController m_cameraController;

    double m_engineTimeCounter = 0.0;
    double m_gameTimeCounter = 0.0;

    uint32_t m_frameIndex = 0;

    rendering::scene::Camera m_cachedCamera;

    rendering::scene::CameraContextPtr m_cameraContext;

    base::RefWeakPtr<ui::IInputAction> m_renderInputAction;

    ToolBarPtr m_toolbar;
    ToolBarPtr m_bottomToolbar;

    //--

    void createFilterItem(base::StringView prefix, const rendering::scene::FilterBitInfo* bitInfo, MenuButtonContainer* menu);
    void createToolbarItems();

    InputActionPtr createLeftMouseButtonCameraAction(const ElementArea& area, const base::input::MouseClickEvent& evt, bool allowSelection);
    InputActionPtr createRightMouseButtonCameraAction(const ElementArea& area, const base::input::MouseClickEvent& evt);
    InputActionPtr createMiddleMouseButtonCameraAction(const ElementArea& area, const base::input::MouseClickEvent& evt);
};

//--

/// ui widget with a full 3D rendering scene (but not world, just rendering scene)
class RENDERING_UI_VIEWPORT_API RenderingSimpleScenePanel : public RenderingScenePanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingSimpleScenePanel, RenderingScenePanel);

public:
    RenderingSimpleScenePanel();
    virtual ~RenderingSimpleScenePanel();

    //--

    INLINE rendering::scene::Scene* scene() const { return m_scene; }

    //--

protected:
    virtual void handleRender(rendering::scene::FrameParams& frame) override;

    rendering::scene::ScenePtr m_scene;
};

//---

END_BOOMER_NAMESPACE(ui)