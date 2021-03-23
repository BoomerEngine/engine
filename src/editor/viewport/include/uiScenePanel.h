/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#pragma once

#include "viewportCameraController.h"

#include "engine/rendering/include/filters.h"
#include "engine/rendering/include/params.h"
#include "engine/rendering/include/stats.h"
#include "engine/ui/include/uiRenderingPanel.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

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
class EDITOR_VIEWPORT_API RenderingPanelSelectionQuery : public IReferencable
{
public:
    RenderingPanelSelectionQuery(const Rect& area, Array<EncodedSelectable> entries, const Camera& camera);

    /// get camera data used during rendering
    INLINE const Camera& camera() const { return m_camera; }

    /// get the selection area
    INLINE const Rect& area() const { return m_area; }

    /// get the captured selection entries
    INLINE const Array<EncodedSelectable>& entries() const { return m_entries; }

    /// extract selection list for given point, selects best selectable at given position
    bool extractPointSelection(PointSelectionMode mode, const Point& point, Selectable& outSelectable) const;

    /// extract selection for given area
    bool extractAreaSelection(AreaSelectionMode mode, const Rect& rect, HashSet<Selectable>& outSelectables) const;

private:
    friend class RenderingPanel;

    Rect m_area; // the selection area to extract
    Array<EncodedSelectable> m_entries;

    Camera m_camera;
};

///---

/// depth buffer query
class EDITOR_VIEWPORT_API RenderingPanelDepthBufferQuery : public IReferencable
{
public:
    RenderingPanelDepthBufferQuery(uint32_t width, uint32_t height, const Camera& camera, const Rect& validDepthValues, Array<float>&& depthValues, bool flipped);

    /// get captured resolution
    INLINE uint32_t width() const { return m_width; }
    INLINE uint32_t height() const { return m_height; }

    /// get the camera used to capture the depth buffer
    INLINE const Camera& camera() const { return m_camera; }

    /// get the range of client space pixels for which we have depth values
    INLINE const Rect& depthValuesRect() const { return m_depthValuesRect; }

    /// get projected depth value at pixel, returns 1.0f for values outside captured area
    float projectedDepthValueAtPixel(int x, int y, bool* withinRect = nullptr) const;

    /// get linear depth value at pixel, returns camera's far plane for values outside captured area
    float linearDepthValueAtPixel(int x, int y, bool* withinRect = nullptr) const;

    /// calculate world position for given pixel
    bool calcWorldPosition(int x, int y, ExactPosition& outPos) const;

private:
    uint32_t m_width;
    uint32_t m_height;
    bool m_flipped = false;

    Camera m_camera;
    Rect m_depthValuesRect;
    Array<float> m_depthValues;
};

//--

/// base panel settings
struct EDITOR_VIEWPORT_API RenderingScenePanelSettings
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(RenderingScenePanelSettings);

public:
    rendering::FrameRenderMode renderMode = rendering::FrameRenderMode::Default;
    StringID renderMaterialDebugChannelName;

    rendering::FrameFilterFlags filters;

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
class EDITOR_VIEWPORT_API RenderingScenePanel : public RenderingPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingScenePanel, RenderingPanel);

public:
    RenderingScenePanel();
    virtual ~RenderingScenePanel();

    //--

    // get the scene to render :)
    virtual rendering::RenderingScene* scene() const { return nullptr; }

    //--

    // get the overlay toolbar
    INLINE const ToolBarPtr& toolbar() const { return m_toolbar; }

    // get the bottom overlay toolbar
    INLINE const ToolBarPtr& bottomToolbar() const { return m_bottomToolbar; }

    // center area (stats windows)
    INLINE const ElementPtr& centerArea() const { return m_centerArea; }

    /// get last computed camera
    INLINE const Camera& cachedCamera() const { return m_cachedCamera; }

    // get the panel settings
    INLINE const RenderingScenePanelSettings& panelSettings() const { return m_panelSettings; }

    // get the camera settings
    INLINE const ViewportCameraControllerSettings& cameraSettings() const { return m_cameraController.settings(); }

    // get last frame stats
    INLINE const rendering::FrameStats& stats() const { return m_frameStats; }

    // change panel settings
    void panelSettings(const RenderingScenePanelSettings& settings);

    // change camera settings
    void cameraSettings(const ViewportCameraControllerSettings& settings);

    //--

    // modify camera to show given bounds
    void focusOnBounds(const Box& bounds, float distanceFactor /*= 1.0f*/, const Angles* newRotation/*=nullptr*/);

    //--

    /// compute bounds of the content in the view
    virtual bool computeContentBounds(Box& outBox) const;

    //---

    // query selectable objects from given area
    // TODO: object filters (debug, transparent, etc)
    RefPtr<RenderingPanelSelectionQuery> querySelection(const Rect* captureArea = nullptr);

    // query depth buffer from given area
    // TODO: object filters (debug, transparent, etc)
    RefPtr<RenderingPanelDepthBufferQuery> queryDepth(const Rect* captureArea = nullptr);

    //--

protected:
    virtual bool handleKeyEvent(const InputKeyEvent& evt) override;
    virtual InputActionPtr handleMouseClick(const ElementArea& area, const InputMouseClickEvent& evt) override;
    virtual bool handleMouseMovement(const InputMouseMovementEvent& evt) override;
    virtual bool handleMouseWheel(const InputMouseMovementEvent& evt, float delta) override;
    virtual void handleFocusLost() override;

    virtual void handleUpdate(float dt);
    virtual void handleCamera(CameraSetup& outCamera) const override;
    virtual void handleFrame(rendering::FrameParams& frame);
    virtual void handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, const CameraSetup& camera, const rendering::FrameParams_Capture* capture) override;
    virtual void handlePointSelection(bool ctrl, bool shift, const Point& clientPosition);
    virtual void handleAreaSelection(bool ctrl, bool shift, const Rect& clientRect);
    virtual bool handleContextMenu(const ElementArea& area, const Position& absolutePosition, InputKeyMask controlKeys) override;
    virtual void handlePostRenderContent();

    virtual void handlePointSelection(bool ctrl, bool shift, const Point& clientPosition, const Array<Selectable>& selectables);
    virtual void handleAreaSelection(bool ctrl, bool shift, const Rect& clientRect, const Array<Selectable>& selectables);
    virtual void handleContextMenu(bool ctrl, bool shift, const Position& absolutePosition, const Point& clientPosition, const Selectable& objectUnderCursor, const ExactPosition* positionUnderCursor);

    virtual void buildRenderModePopup(MenuButtonContainer* menu);
    virtual void buildFilterPopup(MenuButtonContainer* menu);
    virtual void buildCameraPopup(MenuButtonContainer* menu);

	void drawViewAxes(uint32_t width, uint32_t height, rendering::FrameParams& frame);
    void drawCameraInfo(uint32_t width, uint32_t height, rendering::FrameParams& frame);
    void drawGrid(rendering::FrameParams& frame);

    void rotateGlobalLight(float deltaPitch, float deltaYaw);

    bool queryWorldPositionUnderCursor(const Point& localPoint, ExactPosition& outPosition);

    //-
       
protected:
    RenderingScenePanelSettings m_panelSettings;

    rendering::FrameStats m_frameStats;

private:
    Timer m_updateTimer;
    NativeTimePoint m_lastUpdateTime;

    ViewportCameraController m_cameraController;

    double m_engineTimeCounter = 0.0;
    double m_gameTimeCounter = 0.0;

    uint32_t m_frameIndex = 0;

    mutable Camera m_cachedCamera;

    rendering::CameraContextPtr m_cameraContext;

    RefWeakPtr<ui::IInputAction> m_renderInputAction;

    ToolBarPtr m_toolbar;
    ToolBarPtr m_bottomToolbar;
    ElementPtr m_centerArea;

    //--

    void createFilterItem(StringView prefix, const rendering::FrameFilterBitInfo* bitInfo, MenuButtonContainer* menu);
    void createToolbarItems();

    InputActionPtr createLeftMouseButtonCameraAction(const ElementArea& area, const InputMouseClickEvent& evt, bool allowSelection);
    InputActionPtr createRightMouseButtonCameraAction(const ElementArea& area, const InputMouseClickEvent& evt);
    InputActionPtr createMiddleMouseButtonCameraAction(const ElementArea& area, const InputMouseClickEvent& evt);
};

//--

/// ui widget with a full 3D rendering scene (but not world, just rendering scene)
class EDITOR_VIEWPORT_API RenderingSimpleScenePanel : public RenderingScenePanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingSimpleScenePanel, RenderingScenePanel);

public:
    RenderingSimpleScenePanel();
    virtual ~RenderingSimpleScenePanel();

    //--

    virtual rendering::RenderingScene* scene() const override final { return m_scene; }

    //--

protected:
    rendering::RenderingScenePtr m_scene;
};

//---

END_BOOMER_NAMESPACE_EX(ui)
