/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_scene_glue.inl"

namespace rendering
{
    namespace scene
    {
		///---

		static const uint32_t MAX_CASCADES = 4;

        ///---

        class Camera;
        struct CameraSetup;

        class FlyCamera;

        ///---

        class IFrameInspector;

        //--
 
		struct FrameCompositionTarget;

        struct FrameParams;
        struct FrameParams_Capture;

        class FrameSurfaceCache;
        
        class CameraContext;
        typedef base::RefPtr<CameraContext> CameraContextPtr;

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

        class MaterialCachedTemplate;
        typedef base::RefPtr<MaterialCachedTemplate> MaterialCachedTemplatePtr;

        class Scene;
        typedef base::RefPtr<Scene> ScenePtr;

        struct Command;

        class Selectable;

        class MaterialCache;
        struct MaterialCacheEntry;

        struct Fragment;
        class IFragmentHandler;
        class FragmentDrawList;
        
        class FrameRenderer;
        class FrameView;
        class FrameSurfaces;

        struct FilterFlags;

		//--

		class FrameRenderingService;

		class FrameViewMain;
		class FrameViewCascades;
		class FrameViewCaptureSelection;
        class FrameViewWireframe;
		
        struct FrameViewMainRecorder;
        struct FrameViewCascadesRecorder;
        struct FrameViewWireframeRecorder;
        struct FrameViewCaptureSelectionRecorder;

		//--

		class IObjectManager;

		enum class ObjectType : uint8_t
		{
			Mesh,
		};
		
		class IObjectProxy;
		typedef base::RefPtr<IObjectProxy> ObjectProxyPtr;

		class ObjectProxyMesh;
		typedef base::RefPtr<ObjectProxyMesh> ObjectProxyMeshPtr;

		//--

    } // scene
} // rendering

