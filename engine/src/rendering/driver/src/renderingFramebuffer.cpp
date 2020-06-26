/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "renderingFramebuffer.h"

namespace rendering
{
    //--

    void FrameBufferColorAttachmentInfo::print(base::IFormatStream& f) const
    {
        if (empty())
        {
            f << "empty";
        }
        else
        {
            f << rt;

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
            f << rt;

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

    static bool CheckRT(const ImageView& view, bool depth, int& w, int& h, int& s)
    {
        if (!view.empty())
        {
            if (!view.renderTarget())
                return false;
            if (view.viewType() != ImageViewType::View2D && view.viewType() != ImageViewType::View2DArray)
                return false;
            if (view.format() == ImageFormat::UNKNOWN)
                return false;
            if (view.numMips() != 1)
                return false;

            const auto isDepthFormat = (GetImageFormatInfo(view.format()).formatClass == ImageFormatClass::DEPTH);
            if (depth != isDepthFormat)
                return false;
            if (depth != view.renderTargetDepth())
                return false;

            /*int realW = std::max<int>(1, view.width() >> view.firstMip());
            int realH = std::max<int>(1, view.width() >> view.firstMip());*/

            if (w == -1 || h == -1 || s == -1)
            {
                w = view.width();
                h = view.height();
                s = view.numSamples();
            }
            else
            {
                if (w != (int)view.width())
                    return false;
                if (h != (int)view.height())
                    return false;
                if (s != (int)view.numSamples())
                    return false;
            }
        }

        return true;
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

    bool FrameBuffer::validate() const
    {
        int w = -1;
        int h = -1;
        int s = -1;

        if (!CheckRT(depth.rt, true, w, h, s))
            return false;

        for (uint32_t i = 0; i < MAX_COLOR_TARGETS; ++i)
        {
            if (!CheckRT(color[i].rt, false, w, h, s))
                return false;
        }

        if (w == -1 || h == -1 || s == -1)
            return false;

        return true;
    }

 } // rendering

