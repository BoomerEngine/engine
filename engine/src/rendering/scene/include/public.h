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
        class FrameSurfaces;

        struct FilterFlags;

        //--

        enum class FrameViewType : uint8_t
        {
            MainColor, // main color view (or derivatives)
            GlobalCascades, // view for global cascades - only shadow casting fragments should be collected

            MAX,
        };

        enum class ProxyType : uint8_t
        {
            None = 0, // just here to make 0 indicate invalid proxy type
            Mesh = 1,

            MAX,
        };

        enum class FragmentHandlerType : uint8_t
        {
            None = 0, // just here to make 0 indicate invalid fragment type
            Mesh = 1,

            MAX,
        };

        enum class FragmentDrawBucket : uint8_t
        {
            OpaqueNotMoving, // true static geometry, no masking
            Opaque, // opaque that might be moving
            OpaqueMasked,

            DebugSolid,

            ShadowDepth0,
            ShadowDepth1,
            ShadowDepth2,
            ShadowDepth3,

            Transparent,
            SelectionOutline,

            MAX,
        };

    } // scene
} // rendering

