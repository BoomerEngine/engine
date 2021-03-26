/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

#include "debugGeometry.h"
#include "filters.h"

BEGIN_BOOMER_NAMESPACE()

///---

// rendering resolution settings
struct ENGINE_RENDERING_API FrameParams_Resolution
{
    uint32_t width = 0; // width of the internal rendering targets, may be different than composition size
    uint32_t height = 0; // height of the internal rendering targets, may be different than composition size

    uint8_t msaaLevel = 1; // disabled

    FrameParams_Resolution();

    float aspect() const; // width/height
};

///---

// viewport camera settings
struct ENGINE_RENDERING_API FrameParams_Camera
{
    Camera camera;
    CameraContext* cameraContext = nullptr;

    FrameParams_Camera();
};

///---

// clear color
struct ENGINE_RENDERING_API FrameParams_Clear
{
    bool clear = true;
    Vector4 clearColor;

    FrameParams_Clear();
};

///---

// time parameters
struct ENGINE_RENDERING_API FrameParams_Time
{
    double gameTime = 0.0; // game time, affected by so-mo, pause, etc, resets after game reload, full screen UI and very raerly when error grows to large
    double engineRealTime = 0.0; // engine time, not effected by pause, may reset from time to time to zero
    float timeOfDay = 12.0f; // 0-24 value representing time of day
    float dayNightFrac = 1.0f; // number representing transition from night (0) to day (1) regimes, blends only during the switch, normally solid 0 or 1

    FrameParams_Time();
};

///---

/// what to capture from a frame
enum class FrameCaptureMode : uint8_t
{
    Disabled, // do not capture anything
    SelectionRect, // capture selection fragments in given area
    DepthRect, // capture depth data in given area
};

/// Capture settings for frame rendering, allows to easily extract rendered data back to a CPU side
/// Used in editor mostly to get the selection and depth data of the scene, used also for thumbnails and such
struct ENGINE_RENDERING_API FrameParams_Capture
{
    FrameCaptureMode mode; // capture mode
    Rect region; // area to capture, can be used with both the image and buffer capture

    gpu::DownloadDataSinkPtr sink; // output sink for data download
            
    FrameParams_Capture(); // assigns global (config) defaults
};

//---

/// global lighting parameters
struct ENGINE_RENDERING_API FrameParams_GlobalLighting
{
    // simplest (directional) settings
    Vector3 globalLightDirection; // normal vector towards the global light
    Vector3 globalLightColor; // color of the global light, LINEAR
    Vector3 globalAmbientColorZenith; // color of the global ambient light, LINEAR
    Vector3 globalAmbientColorHorizon; // color of the global light, LINEAR

    FrameParams_GlobalLighting(); // assigns global (config) defaults
};

//---

/// global shadow cascades
struct ENGINE_RENDERING_API FrameParams_ShadowCascades
{
    int numCascades = 0; // number of cascades, 0 to disable
    float baseRange = 2.0f; // base cascade range - this is the range of the 1st cascade
    float baseEdgeFade = 0.05f; // size of fade between cascades
    float baseFilterSize = 16.0f; // in texels of the image

    float baseDepthBiasConstant = 0.0f;
    float baseDepthBiasSlope = 0.0f;

    float filterSizeTexelSizeMul = 0.0f;
    float depthBiasSlopeTexelSizeMul = 0.0f;

    float rangeMul1 = 5.0f; // scale factor between cascade 0 and 1
    float rangeMul2 = 5.0f; // scale factor between cascade 1 and 2
    float rangeMul3 = 5.0f; // scale factor between cascade 2 and 3

    FrameParams_ShadowCascades();
};

//---

/// ambient occlusion params
struct ENGINE_RENDERING_API FrameParams_AmbientOcclusion
{
    bool enabled = true;
    bool blur = true;
    float intensity = 1.5f;
    float bias = 0.1f;
    float radius = 2.0f;
    float blurSharpness = 40.0f;

    FrameParams_AmbientOcclusion();
};

//---

/// tone mapping type
enum class FrameToneMappingType : uint8_t
{
    None, // no tone mapping, data is output directly
    Linear, // simple 1/2.2 gamma
    SimpleReinhard,
    LumabasedReinhard,
    WhitePreservingLumabasedReinhard,
    RomBinDaHouse,
    Filmic,
    Uncharted2,
};

/// tone mapping parameters
struct ENGINE_RENDERING_API FrameParams_ToneMapping
{
    FrameToneMappingType type = FrameToneMappingType::Uncharted2; // best game ever

    FrameParams_ToneMapping();
};

//---

// auto exposure adaptation mode
struct ENGINE_RENDERING_API FrameParams_ExposureAdaptation
{
    float keyValue = 0.18f;
    float exposureCompensationEV = 0.0f;
    float minLuminanceEV = -2.0f;
    float maxLuminanceEV = 5.0f;
    float adaptationSpeed = 0.1f; // 0-instant adaptation

    FrameParams_ExposureAdaptation();
};

//---

/// color grading parameters
struct ENGINE_RENDERING_API FrameParams_ColorGrading
{
    float temperatureShift = 0.0f; // no shift (-1 to 1)
    float temeratureTint = 0.0f; // no tint (-1 to 1)

    float contrast = 1.0f;
    float vibrance = 1.0f;
    float saturation = 1.0f;

    Vector3 mixerRed = Vector3(1, 0, 0);
    Vector3 mixerGreen = Vector3(0, 1, 0);
    Vector3 mixerBlue = Vector3(0, 0, 1);

    Vector3 toneShadows = Vector3(1, 1, 1);
    Vector3 toneMidtones = Vector3(1, 1, 1);
    Vector3 toneHighlights = Vector3(1, 1, 1);
    Vector2 toneShadowsRange = Vector2(0.0f, 0.333f);
    Vector2 toneHighlightRange = Vector2(0.55f, 1.0f);

    Vector3 colorSlope = Vector3(1, 1, 1);
    Vector3 colorOffset = Vector3(0, 0, 0);
    Vector3 colorPower = Vector3(1, 1, 1);

    Vector3 curveShadowGamma = Vector3(1, 1, 1);
    Vector3 curveMidPoint = Vector3(1, 1, 1);
    Vector3 curveHighlightScale = Vector3(1, 1, 1);

    FrameParams_ColorGrading();
};

//----

/// selection outline parameters
struct ENGINE_RENDERING_API FrameParams_SelectionOutline
{
    Color colorFront;
    Color colorBack;
    float outlineWidth = 4.0f; // DPI invariant (get's thicker with higher DPI)
    float centerOpacity = 0.5f; // inner highlight opacity

    FrameParams_SelectionOutline();
};

//----

/// collected debug geometry to render
struct ENGINE_RENDERING_API FrameParams_DebugData
{
    StringID materialDebugMode;

    Point mouseHoverPixel = Point(-1,-1); // viewport coordinates of pixel in the active viewport that is currently under the cursor, set to -1,-1 if no pixel is under the cursor
    Point mouseClickedPixel = Point(-1, -1); // coordinates of pixel that was clicked
    uint32_t mouseButtons = 0; // current state of mouse buttons
};

//---

/// frame rendering mode
enum class FrameRenderMode : uint8_t
{
    Default, // default, full rendering
    WireframeSolid, // render a solid wireframe, objects are filled in with their colors and an triangles/quads have edges rendered in black
    WireframePassThrough, // render a classical "wired" wireframe

    DebugDepth, // visualize frame depth
    DebugLuminance, // visualize frame luminance
    DebugShadowMask, // visualize shadow mask buffer
    DebugReconstructedViewNormals, // visualize the reconstructed view-space normals
    DebugAmbientOcclusion, // visualize AO buffer
    DebugMaterial, // debug material channel - outputs specific material output instead of calculating whole material
};

//---

/// structure that hold all frame rendering parameters for configuring rendering of this frame
struct ENGINE_RENDERING_API FrameParams : public NoCopy
{
    FrameRenderMode mode = FrameRenderMode::Default;
    FrameFilterFlags filters = FrameFilterFlags::DefaultEditor();

    uint32_t index = 0; // frame index, should be increased every frame by the caller

    FrameParams_Time time;
    FrameParams_Camera camera;
    FrameParams_Clear clear;
    FrameParams_Resolution resolution;
    FrameParams_Capture capture;
    FrameParams_GlobalLighting globalLighting;
    FrameParams_DebugData debug;
    FrameParams_ToneMapping toneMapping;
    FrameParams_ExposureAdaptation exposureAdaptation;
    FrameParams_ColorGrading colorGrading;
    FrameParams_ShadowCascades cascades;
    FrameParams_AmbientOcclusion ao;
    FrameParams_SelectionOutline selectionOutline;

    //--

    // calculate hack "scaling factor" to keep objects constant size on screen but to render them in world space
    float screenSpaceScalingFactor(const Vector3& pos) const;

    // calculate position on screen of given world space position
    bool screenPosition(const Vector3& center, Vector3& outScreenPos, int margin = 0) const;

    //--

    FrameParams();
};

//---

END_BOOMER_NAMESPACE()
