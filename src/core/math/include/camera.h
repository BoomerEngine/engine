/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: camera #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

/// input camera setup, used so the data can be gathered and set once since updating the camera is not cheap
/// also this structure is exposed to script while the camera is not
struct CORE_MATH_API CameraSetup
{
public:
    Vector3 position; // position of the camera
    Quat rotation; // orientation of the camera

    float fov; // in degrees, 0 for isometric view
    float zoom; // zoom factor (crop)
    float aspect; // vertical/horizontal
    float nearPlane; // near clip plane
    float farPlane; // far clip plane (drawing distance)

    float subPixelOffsetX; // sub pixel offset (jitter) 1.0f = one pixel
    float subPixelOffsetY; // sub pixel offset (jitter) 1.0f = one pixel

	uint32_t discontinuityIndex; // something to bump every time camera teleports or changes, let's the renderer detect cases when frame to frame stuff like TAA, Adaptation, etc must be reset

    //--

    CameraSetup();

    /// is this a perspective setup ?
    INLINE bool perspective() const { return fov > 0.0f; }

    // calculate the camera to world (basic placement)
    void calcWorldToCamera(Matrix& outWorldToCamera) const;

    // calculate the world to camera (projection)
    void calcCameraToWorld(Matrix& outCameraToWorld) const;

    // calculate the view to screen matrix
    void calcViewToScreen(Matrix& outViewToScreen) const;

    // calculate the camera axes
    void calcAxes(Vector3* outForward, Vector3* outRight, Vector3* outUp) const;

    ///--

    static Matrix CameraToView;
    static Matrix ViewToCamera;
};

/// camera setup, free of viewport related setup
class CORE_MATH_API Camera
{
public:
    Camera();
    Camera(const CameraSetup& setup);
    Camera(const Camera& other);
    Camera(Camera&& other);
    Camera& operator=(const Camera& other);
    Camera& operator=(Camera&& other);

    /// get the original setup of the camera
    INLINE const CameraSetup& setup() const { return m_setup; }

    /// get camera position as vector 3
    INLINE const Vector3& position() const { return m_setup.position; }

    /// get frame forward vector
    INLINE const Vector3& directionForward() const { return m_frameForward; }

    /// get frame up vector
    INLINE const Vector3& directionUp() const { return m_frameUp; }

    /// get frame right vector
    INLINE const Vector3& directionRight() const { return m_frameRight; }

    /// get world to camera matrix, depends only on the camera placement
    INLINE const Matrix& worldToCamera() const { return m_worldToCamera; }

    /// get camera to world matrix, depends only on the camera placement
    INLINE const Matrix& cameraToWorld() const { return m_cameraToWorld; }

    /// get view to screen matrix, depends only on the projection settings
    INLINE const Matrix& viewToScreen() const { return m_viewToScreen; }

    /// get screen to view matrix, depends only on the projection settings
    INLINE const Matrix& screenToView() const { return m_screenToView; }

    /// get merged world to screen matrix for the camera, this is a full transform
    INLINE const Matrix& worldToScreen() const { return m_worldToScreen; }

    /// get merged screen to world matrix for the camera, this is a full transform
    INLINE const Matrix& screenToWorld() const { return m_screenToWorld; }

    /// get camera rotation
    INLINE const Quat& rotation() const { return m_setup.rotation; }

    /// get camera field of view in degrees
    /// NOTE: returns zero for isometric camera
    INLINE float fOV() const { return m_setup.fov; }

    /// get camera aspect ratio
    INLINE float aspect() const { return m_setup.aspect; }

    /// is this a perspective camera ?
    INLINE bool isPerspective() const { return m_setup.fov > 0.0f; }

    /// is this an isometric camera ?
    INLINE bool isIsometric() const { return m_setup.fov <= 0.0f; }

    /// get camera near plane
    INLINE float nearPlane() const { return m_setup.nearPlane; }

    /// get camera far plane
    INLINE float farPlane() const { return m_setup.farPlane; }

    /// get camera zoom factor
    INLINE float zoom() const { return m_setup.zoom; }

    //---

    /// set new camera placement and projection
    /// NOTE: costly
    /// if the recreate token flag is set this effectively tells the renderer "I'm a new camera that has nothing to do with the old one"
    /// this will ensure that all frame to frame dependencies like HDR adaptation, LOD transitions are reset for the next frame
    void setup(const CameraSetup& setupParams);

    //---

    /// calculate scaling factor for screen space adjusted geometry
    /// this scaling factor will keep objects the same size
    /// NOTE: this factor depends on the resolution
    float calcScreenSpaceScalingFactor(const Vector3& pos, uint32_t width, uint32_t height) const;

    /// calculate ray for given normalized coordinates
    bool calcWorldSpaceRay(const Vector3& screenPosition, Vector3& outRayStart, Vector3& outRayDirection) const;

    /// project point in 3D onto screen, returns false if point is behind the camera
    /// NOTE: the XY are in the screen space [0, 1], X is going left to right, Y is going bottom to top
    /// NOTE: the Z encodes the view space depth, not the perspective depth so it's a linear distance from the camera plane
    bool projectWorldToScreen(const Vector3& worldPosition, Vector3& outScreenPosition) const;

    /// project point from 3D screen space into the world space
    /// NOTE: the XY are in the screen space [0, 1], X is going left to right, Y is going bottom to top
    /// NOTE: the Z encodes the view space depth, not the perspective depth so it's a linear distance from the camera plane
    bool projectScreenToWorld(Vector3& outWorldPosition, const Vector3& screenPosition) const;

    /// calculate corners of the frustum
    void calcFrustumCorners(float zPlane, Vector3* outCorners) const;

    /// calculate 0-1 depth from a linear Z
    float linearZToProjectedZ(float linearZ) const;

    /// calculate linear Z from 0-1 projected depth
    float projectedZToLinearZ(float projectedZ) const;

private:
    CameraSetup m_setup;

    Vector3 m_frameForward;
    Vector3 m_frameUp;
    Vector3 m_frameRight;

    Matrix m_worldToCamera;
    Matrix m_cameraToWorld;

    Matrix m_viewToScreen;
    Matrix m_screenToView;

    Matrix m_cameraToScreen;
    Matrix m_screenToCamera;
    Matrix m_worldToScreen;
    Matrix m_screenToWorld;

    void updateMatrices();
};
        
//--

/// visibility frustum, optimized for fast tests
struct CORE_MATH_API VisibilityFrustum
{
    static const uint8_t MAX_PLANES = 6;

    static const uint8_t PLANE_NEAR = 0;
    static const uint8_t PLANE_FAR = 1;
    static const uint8_t PLANE_LEFT = 2;
    static const uint8_t PLANE_RIGHT = 3;
    static const uint8_t PLANE_TOP = 4;
    static const uint8_t PLANE_BOTTOM = 5;

    SIMDQuad origin; // camera reference point
    SIMDQuad planes[6]; // XYZ - normal, W - distance (dot(p,XYZ)+W = 0 for points on plane)
    SIMDQuad planeMask[6]; // X<0, Y<0, Z<0 for each plane

    // setup from normal rendering camera
    void setup(const Camera& sourceCamera);
};

/// visibility bounding box
struct CORE_MATH_API VisibilityBox
{
    SIMDQuad min;
    SIMDQuad max;

    // setup from normal bounding box
    void setup(const Box& box);

    // are we inside a given frustum
    bool isInFrustum(const VisibilityFrustum& f) const;
};

//--

END_BOOMER_NAMESPACE()
