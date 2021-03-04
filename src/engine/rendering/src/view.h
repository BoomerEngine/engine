/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(rendering)

#pragma pack(push)
#pragma pack(4)

///----
/// Packed GPU data
/// TODO: move to common header, for now keep in sync

/// global frame parameters, set once
struct GPUFrameParameters
{
    Point ViewportSize; // size of the frame viewport
    Point ViewportRTSize; // size of the frame render target (may be bigger)
    Vector2 InvViewportSize;
    Vector2 InvViewportRTSize;

    /*Point TargetSize; // size of the target (composition) viewport - may be totally different size
    Point TargetRTSize; // size of the composition render target (may be bigger)
    Vector2 InvTargetSize;
    Vector2 InvTargetRTSize;*/

    uint32_t FrameIndex; // running frame index, every frame new number
    uint32_t MSAASamples;
    uint32_t MaterialFlags;
    uint32_t __PaddingC;

    int32_t ScreenTopY;
    int32_t ScreenBottomY;
    int32_t ScreenDeltaY;
    int32_t __PaddingD;

    uint32_t PseudoRandom[4]; // pseudo random numbers - new every frame

    Point DebugMousePos; // position (in Frame pixel coords) of mouse cursor, can be used for debugging shaders
    Point DebugMouseClickPos; // position (in Frame pixel coords) of last mouse click

    float GameTime = 0.0f;
    float EngineTime = 0.0f;
    float TimeOfDay = 12.0f;
    float DayNightFraction = 1.0f;
};

// camera parameters, as seen by shaders
struct GPUCameraInfo
{
    Vector4 CameraPosition;
    Vector4 CameraForward;
    Vector4 CameraUp;
    Vector4 CameraRight;

    Matrix WorldToScreen;
    Matrix ScreenToWorld;
    Matrix WorldToScreenNoJitter;
    Matrix ScreenToWorldNoJitter;
    Matrix WorldToPixelCoord;
    Matrix PixelCoordToWorld;

    Vector4 LinearizeZ;
    Vector4 NearFarPlane;

    Vector4 PrevCameraPosition;
    Matrix PrevWorldToScreen;
    Matrix PrevScreenToWorld;
    Matrix PrevWorldToScreenNoJitter;
    Matrix PrevScreenToWorldNoJitter;
};

//--

// single cascade slice info
struct GPUCascadeInfo
{
    Matrix ShadowTransform;
    Vector4 ShadowOffsetsX;
    Vector4 ShadowOffsetsY;
    Vector4 ShadowHalfSizes;
    Vector4 ShadowParams[4];
    Vector4 ShadowPoissonOffsetAndBias;
    Vector4 ShadowTextureSize;
    Vector4 ShadowFadeScales;
    Vector4 ShadowDepthRanges;
    uint32_t ShadowQuality;
};

//--

// global lighting information
struct GPULightingInfo
{
    Vector4 globalLightDirection;
    Vector4 globalLightColor;
    Vector4 globalAmbientColorZenith;
    Vector4 globalAmbientColorHorizon;
};

#pragma pack(pop)

//--

// setup of single global shadow cascade
struct ENGINE_RENDERING_API CascadeInfo
{
    uint8_t cascadeIndex = 0;
    float pixelSize = 1.0f;
    float invPixelSize = 1.0f;
    float edgeFade = 0.0f;
    float filterScale = 0.0f;
    float filterTexelSize = 0.0f;
    float worldSpaceTexelSize = 0.0f;

    float depthBiasConstant = 0.0f;
    float depthBiasSlope = 0.0f;

    //ImageView m_dephtBuffer;

    Camera camera; // culling camera
    Camera jitterCamera; // rendering camera (with jitter)
};

//--

// general setup for all cascades
struct ENGINE_RENDERING_API CascadeData
{
    uint8_t numCascades = 0;
    CascadeInfo cascades[MAX_SHADOW_CASCADES];
};

//--

// calculate cascade settings that best match given camera 
// NOTE: texture must be bound first (we need size internally for some computations)
extern ENGINE_RENDERING_API void CalculateCascadeSettings(const Vector3& lightDirection, uint32_t resolution, const Camera& viewCamera, const FrameParams_ShadowCascades& setup, CascadeData& outData);

//--

// pack frame parameters
extern ENGINE_RENDERING_API void PackFrameParams(GPUFrameParameters& outParams, const FrameRenderer& frame, const FrameCompositionTarget& targets);

// pack camera setup for GPU
extern ENGINE_RENDERING_API void PackSingleCameraParams(GPUCameraInfo& outInfo, const Camera& camera, const Camera* prevCamera = nullptr);

// pack lighting setup
extern ENGINE_RENDERING_API void PackLightingParams(GPULightingInfo& outInfo, const FrameParams_GlobalLighting& lighting);

// pack cascade shadows params
extern ENGINE_RENDERING_API void PackCascadeParams(GPUCascadeInfo& outInfo, const CascadeData& cascades);

//--

/// single camera view
class ENGINE_RENDERING_API FrameViewSingleCamera : public NoCopy
{
public:
    FrameViewSingleCamera(const FrameRenderer& frame, const Camera& camera, const Rect& viewport);
    virtual ~FrameViewSingleCamera();

    //--

    INLINE const FrameRenderer& frame() const { return m_frame; }

    INLINE const Rect& viewport() const { return m_viewport; }

    INLINE const Camera& visibilityCamera() const { return m_camera; }

    INLINE const Vector3 lodReferencePoint() const { return m_lodReferencePoint; }

    //--

    void bindCamera(gpu::CommandWriter& cmd);

    //--

protected:
    const FrameRenderer& m_frame;

    Vector3 m_lodReferencePoint;

    Rect m_viewport;
    Camera m_camera;
};

//--

END_BOOMER_NAMESPACE_EX(rendering)
