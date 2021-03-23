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

//--

class Scene;
typedef RefPtr<Scene> ScenePtr;

class FrameRenderer;
class FrameView;
class FrameSurfaces;

struct FilterFlags;

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

class IObjectManager;
		
class IObjectProxy;
typedef RefPtr<IObjectProxy> ObjectProxyPtr;

class ObjectProxyMesh;
typedef RefPtr<ObjectProxyMesh> ObjectProxyMeshPtr;

//--

END_BOOMER_NAMESPACE_EX(rendering)
