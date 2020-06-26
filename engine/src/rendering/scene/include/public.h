/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_scene_glue.inl"

#include "rendering/driver/include/renderingImageView.h"

namespace rendering
{
    namespace scene
    {

        ///---

        class Camera;
        struct CameraSetup;

        ///---

        class IFrameInspector;

        //--

        /// render target "wrapper"
        struct RenderTarget
        {
            ImageView view; // render target, may be way bigger than what we need
            base::Rect rect; // our space in that render target

            INLINE RenderTarget() {}

            INLINE RenderTarget(ImageView view_, uint32_t width, uint32_t height)
                : view(view_)
                , rect(0, 0, width, height)
            {}

            INLINE RenderTarget(ImageView view_, const base::Rect& rect_)
                : view(view_)
                , rect(rect_)
            {}
        };

        //--
 
        struct FrameParams;
        struct FrameParams_Capture;

        class FrameSurfaceCache;
        
        class CameraContext;
        typedef base::RefPtr<CameraContext> CameraContextPtr;

        class DebugGeometry;
        class DebugGeometryScreen;

        enum class DebugFont : uint8_t
        {
            Normal,
            Bold,
            Italic,
            Big,
            Small,
        };

        //--

        typedef uint32_t ObjectRenderID;
        //typedef uint16_t MaterialTemplateID;

        class MaterialCachedTemplate;
        typedef base::RefPtr<MaterialCachedTemplate> MaterialCachedTemplatePtr;

        struct ProxyBaseDesc;

        class Scene;
        typedef base::RefPtr<Scene> ScenePtr;

        class SceneObjectRegistry;
        struct SceneObjectCullingEntry;

        struct Command;

        struct IProxy;
        class IProxyHandler;

        struct ProxyHandle
        {
            uint32_t index = 0;
            uint32_t generation = 0;

            INLINE operator bool() const { return generation != 0; }
            INLINE void reset() { generation = 0; }
        };

        class MaterialCache;
        struct MaterialCacheEntry;

        struct Fragment;
        class IFragmentHandler;
        class FragmentDrawList;
        
        class FrameRenderer;
        class FrameView;
        class FrameViewCamera;
        class FrameSurfaces;

        struct FilterFlags;

        //--

        enum class FrameResource : uint8_t
        {
            HDRLinearMainColorRT, // main render target - HDR linear, MSAA if needed
            HDRLinearMainDepthRT, // depth buffer matching the size and format of FullColr

            HDRResolvedColor, // non-MSAA resolved color
            HDRResolvedDepth, // non-MSAA resolved depth

            CameraPrevLum, // previous scene luminance (1x1 texture)
        };

        //--

        enum class ProxyType : uint8_t
        {
            None = 0,
            Mesh = 1,

            MAX,
        };

        enum class FragmentHandlerType : uint8_t
        {
            None = 0,
            Mesh = 1,

            MAX,
        };

        enum class FragmentDrawBucket : uint8_t
        {
            OpaqueNotMoving, // true static geometry, no masking
            Opaque, // opaque that might be moving
            OpaqueMasked,

            DebugSolid,

            Transparent,
            SelectionOutline,

            MAX,
        };

    } // scene
} // rendering

