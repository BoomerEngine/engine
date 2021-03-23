/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_rendering_glue.inl"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

///---

static const uint32_t MAX_SHADOW_CASCADES = 4;

//--

class DebugGeometry;

enum class DebugFont : uint8_t
{
    Normal,
    Bold,
    Italic,
    Big,
    Small,
};

//--

class CameraContext;
typedef RefPtr<CameraContext> CameraContextPtr;

//--
 
struct FrameParams;
struct FrameParams_Capture;

struct FrameStats;

struct FrameFilterFlags;

class FrameRenderer;
class FrameView;
class FrameSurfaces;

//--

class RenderingScene;
typedef RefPtr<RenderingScene> RenderingScenePtr;


//--

class FrameRenderingService;

class FrameViewMain;
class FrameViewCascades;
class FrameViewCaptureSelection;
class FrameViewCaptureDepth;
class FrameViewWireframe;
class FrameViewSingleCamera;

struct FrameViewMainRecorder;
struct FrameViewCascadesRecorder;
struct FrameViewWireframeRecorder;
struct FrameViewCaptureSelectionRecorder;
struct FrameViewCaptureDepthRecorder;

//--

class IRenderingObjectManager;
		
class IRenderingObject;
typedef RefPtr<IRenderingObject> RenderingObjectPtr;

class RenderingMesh;
typedef RefPtr<RenderingMesh> RenderingMeshPtr;

//--

END_BOOMER_NAMESPACE_EX(rendering)
