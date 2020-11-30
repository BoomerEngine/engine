/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "base/containers/include/inplaceArray.h"
#include "base/system/include/staticCRC.h"
#include "renderingGraphicsPassLayout.h"

namespace rendering
{
    //--

    struct FrameBufferAttachmentInfo
    {
		const RenderTargetView* viewPtr = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
        uint8_t slices = 0;
        uint8_t samples = 0;
        LoadOp loadOp = LoadOp::Keep;
        StoreOp storeOp = StoreOp::Store;
		bool swapchain = false;

		ObjectID viewID; // resolved when copied to command buffer

        INLINE bool empty() const { return viewPtr == nullptr && viewID.empty(); }
        INLINE operator bool() const { return !empty(); }
    };

    struct RENDERING_DEVICE_API FrameBufferColorAttachmentInfo : public FrameBufferAttachmentInfo
    {
        float clearColorValues[4] = { 1.0f };

        FrameBufferColorAttachmentInfo& view(const RenderTargetView* rtv);

        INLINE FrameBufferColorAttachmentInfo& dontCare()
        {
            loadOp = LoadOp::DontCare;
            return *this;
        }

        INLINE FrameBufferColorAttachmentInfo& discard()
        {
            storeOp = StoreOp::DontCare;
            return *this;
        }

        INLINE FrameBufferColorAttachmentInfo& clear(base::Color color)
        {
            clearColorValues[0] = color.r / 255.0f;
            clearColorValues[1] = color.g / 255.0f;
            clearColorValues[2] = color.b / 255.0f;
            clearColorValues[3] = color.a / 255.0f;
            loadOp = LoadOp::Clear;
            return *this;
        }

        INLINE FrameBufferColorAttachmentInfo& clear(const base::Vector4& color)
        {
            clearColorValues[0] = color.x;
            clearColorValues[1] = color.y;
            clearColorValues[2] = color.z;
            clearColorValues[3] = color.w;
            loadOp = LoadOp::Clear;
            return *this;
        }

        INLINE FrameBufferColorAttachmentInfo& clear(float r=0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f)
        {
            clearColorValues[0] = r;
            clearColorValues[1] = g;
            clearColorValues[2] = b;
            clearColorValues[3] = a;
            loadOp = LoadOp::Clear;
            return *this;
        }

        void print(base::IFormatStream& f) const;
    };

    //---

    struct RENDERING_DEVICE_API FrameBufferDepthAttachmentInfo : public FrameBufferAttachmentInfo
    {
        float clearDepthValue = 1.0f;
        uint8_t clearStencilValue = 0;

        FrameBufferDepthAttachmentInfo& view(const RenderTargetView* view);

        INLINE FrameBufferDepthAttachmentInfo& dontCare()
        {
            loadOp = LoadOp::DontCare;
            return *this;
        }

        INLINE FrameBufferDepthAttachmentInfo& discard()
        {
            storeOp = StoreOp::DontCare;
            return *this;
        }

        INLINE FrameBufferDepthAttachmentInfo& clearDepth(float val = 1.0f)
        {
            clearDepthValue = val;
            loadOp = LoadOp::Clear;
            return *this;
        }

        INLINE FrameBufferDepthAttachmentInfo& clearStencil(uint8_t val = 0)
        {
            clearStencilValue = val;
            loadOp = LoadOp::Clear;
            return *this;
        }

        void print(base::IFormatStream& f) const;
    };

    //---

    /// frame buffer, configuration of render targets, used when entering render pass
    class RENDERING_DEVICE_API FrameBuffer
    {
    public:
        static const uint32_t MAX_COLOR_TARGETS = 8;

        INLINE FrameBuffer() {};
        INLINE FrameBuffer(const FrameBuffer& other) = default;
        INLINE FrameBuffer(FrameBuffer&& other) = default;
        INLINE FrameBuffer& operator=(const FrameBuffer& other) = default;
        INLINE FrameBuffer& operator=(FrameBuffer&& other) = default;

        FrameBufferColorAttachmentInfo color[MAX_COLOR_TARGETS];
        FrameBufferDepthAttachmentInfo depth;

        //--

        void print(base::IFormatStream& f) const;
        bool validate() const;

        uint8_t validColorSurfaces() const;
		uint8_t samples() const;
    };

    //---

    // viewport state for pass
    struct RENDERING_DEVICE_API FrameBufferViewportState
    {
        base::Rect viewportRect; // if empty than RT bounds are used
        base::Rect scissorRect; // if non-empty the scissor is set and automatically enabled 
        float minDepthRange = 0.0f;
        float maxDepthRange = 1.0f;

        INLINE FrameBufferViewportState() {};
        INLINE FrameBufferViewportState(const base::Rect& r) : viewportRect(r), scissorRect(r) {};
    };

    //---

} // render