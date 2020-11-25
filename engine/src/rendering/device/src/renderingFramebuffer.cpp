/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "renderingFramebuffer.h"
#include "renderingImage.h"

namespace rendering
{
    //--

    FrameBufferColorAttachmentInfo& FrameBufferColorAttachmentInfo::view(const RenderTargetView* rtv)
    {
        DEBUG_CHECK_RETURN_V(rtv, *this);
        DEBUG_CHECK_RETURN_V(!rtv->depth(), *this);
		viewPtr = rtv;
        viewID = rtv->viewId();
        width = rtv->width();
        height = rtv->height();
        slices = rtv->slices();
        samples = rtv->swapchain();
		swapchain = rtv->swapchain();
        return *this;
    }

    //--

    FrameBufferDepthAttachmentInfo& FrameBufferDepthAttachmentInfo::view(const RenderTargetView* rtv)
    {
        DEBUG_CHECK_RETURN_V(rtv, *this);
        DEBUG_CHECK_RETURN_V(rtv->depth(), *this);
		viewPtr = rtv;
        viewID = rtv->viewId();
        width = rtv->width();
        height = rtv->height();
        slices = rtv->slices();
        samples = rtv->swapchain();
        return *this;
    }

    //--

    void FrameBufferColorAttachmentInfo::print(base::IFormatStream& f) const
    {
        if (empty())
        {
            f << "empty";
        }
        else
        {
            f.appendf("[{}x{}] ", width, height);

            if (samples)
                f.append("{} samples", samples);

            if (slices)
                f.append(", {} slices", slices);

            f << ", " << loadOp;
            f << ", " << storeOp;

            if (loadOp == LoadOp::Clear)
            {
                f.appendf(", Clear: {},{},{},{}", clearColorValues[0], clearColorValues[1], clearColorValues[2], clearColorValues[3]);
            }
        }
    }

    //--

    void FrameBufferDepthAttachmentInfo::print(base::IFormatStream& f) const
    {
        if (empty())
        {
            f << "empty";
        }
        else
        {
            f.appendf("[{}x{}] ", width, height);

            if (samples)
                f.append("{} samples", samples);

            if (slices)
                f.append(", {} slices", slices);

            f << ", " << loadOp;
            f << ", " << storeOp;

            if (loadOp == LoadOp::Clear)
            {
                f.appendf(", ClearDepth: {}, ClearStencil: {}", clearDepthValue, clearStencilValue);
            }
        }
    }

    //--

    void FrameBuffer::print(base::IFormatStream& f) const
    {
        bool hasRenderTargets = false;

        if (depth)
        {
            hasRenderTargets = true;
            f << "Depth: " << depth << "\n";
        }

        for (uint32_t i = 0; i < MAX_COLOR_TARGETS; ++i)
        {
            if (color[i])
            {
                hasRenderTargets = true;
                f << "Color" << i << ": " << color[i] << "\n";
            }
        }
    }

    uint8_t FrameBuffer::validColorSurfaces() const
    {
        uint8_t ret = 0;

        for (uint8_t i = 0; i < MAX_COLOR_TARGETS; ++i)
        {
            if (!color[i])
                break;
            ret += 1;
        }

        return ret;
    }

	uint8_t FrameBuffer::samples() const
	{
		if (depth.samples)
			return depth.samples;
		for (uint8_t i = 0; i < MAX_COLOR_TARGETS; ++i)
			if (color[i].samples)
				return color[i].samples;

		return 1;
	}

    bool FrameBuffer::validate() const
    {
        int w = -1;
        int h = -1;
        int s = -1;
        int l = -1;

        bool first = true;

        for (const auto& entry : color)
        {
            if (!entry)
                break;

            if (first)
            {
                w = entry.width;
                h = entry.height;
                s = entry.samples;
                l = entry.slices;
                first = false;
            }
            else
            {
                DEBUG_CHECK_RETURN_V(w == entry.width, false);
                DEBUG_CHECK_RETURN_V(h == entry.height, false);
                DEBUG_CHECK_RETURN_V(s == entry.samples, false);
                DEBUG_CHECK_RETURN_V(l == entry.slices, false);
            }
        }

        if (depth && !first)
        {
            DEBUG_CHECK_RETURN_V(w == depth.width, false);
            DEBUG_CHECK_RETURN_V(h == depth.height, false);
            DEBUG_CHECK_RETURN_V(s == depth.samples, false);
            DEBUG_CHECK_RETURN_V(l == depth.slices, false);
        }

        return true;
    }

    //--

 } // rendering

