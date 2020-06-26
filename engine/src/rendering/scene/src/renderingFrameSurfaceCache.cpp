/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#include "build.h"
#include "renderingFrameParams.h"
#include "renderingFrameSurfaceCache.h"

#include "rendering/driver/include/renderingDriver.h"
#
namespace rendering
{
    namespace scene
    {
        ///--

        FrameSurfaceCache::FrameSurfaceCache(const FrameParams_Resolution& res)
            : m_supportedMSAALevel(res.msaaLevel)
        {
            // determine resolution
            m_maxSupportedWidth = std::max<uint32_t>(device()->maxRenderTargetSize().x, base::Align<uint32_t>(res.width, 16));
            m_maxSupportedHeight = std::max<uint32_t>(device()->maxRenderTargetSize().y, base::Align<uint32_t>(res.height, 16));

            // use the max supported res for render target sizing
            auto mainWidth = m_maxSupportedWidth;
            auto mainHeight = m_maxSupportedHeight;

            // select format
            const auto vr = false;
            const auto hdrFormat = ImageFormat::RGBA16F;
            const auto depthFormat = ImageFormat::D24S8;

            // TODO: automate ?

            // create scene color
            {
                ImageCreationInfo colorInfo;
                colorInfo.label = "MainViewColor";
                colorInfo.view = ImageViewType::View2D; // TODO: array for VR ?
                colorInfo.width = mainWidth;
                colorInfo.height = mainHeight;
                colorInfo.allowRenderTarget = true;
                colorInfo.allowShaderReads = true;
                colorInfo.allowUAV = true;
                colorInfo.format = hdrFormat;
                colorInfo.numSamples = m_supportedMSAALevel;
                m_sceneFullColorRT = device()->createImage(colorInfo);
            }

            // create depth color
            {
                ImageCreationInfo colorInfo;
                colorInfo.label = "MainViewDepth";
                colorInfo.view = ImageViewType::View2D; // TODO: array for VR ?
                colorInfo.width = mainWidth;
                colorInfo.height = mainHeight;
                colorInfo.allowRenderTarget = true;
                colorInfo.allowShaderReads = true;
                colorInfo.allowUAV = false;
                colorInfo.format = depthFormat;
                colorInfo.numSamples = m_supportedMSAALevel;
                m_sceneFullDepthRT = device()->createImage(colorInfo);
            }

            // create resolved color
            {
                ImageCreationInfo colorInfo;
                colorInfo.label = "MainViewResolvedColor";
                colorInfo.view = ImageViewType::View2D; // TODO: array for VR ?
                colorInfo.width = mainWidth; // same resolution just non MSAA
                colorInfo.height = mainHeight;
                colorInfo.allowRenderTarget = true;
                colorInfo.allowShaderReads = true;
                colorInfo.allowUAV = true;
                colorInfo.format = hdrFormat;
                colorInfo.numSamples = 1;
                m_sceneResolvedColor = device()->createImage(colorInfo);
            }

            // create resolved color
            {
                ImageCreationInfo colorInfo;
                colorInfo.label = "MainViewResolvedDepth";
                colorInfo.view = ImageViewType::View2D; // TODO: array for VR ?
                colorInfo.width = mainWidth; // same resolution just non MSAA
                colorInfo.height = mainHeight;
                colorInfo.allowRenderTarget = true;
                colorInfo.allowShaderReads = true;
                colorInfo.allowUAV = true;
                colorInfo.format = depthFormat;
                colorInfo.numSamples = 1;
                m_sceneResolvedDepth = device()->createImage(colorInfo);
            }
        }

        FrameSurfaceCache::~FrameSurfaceCache()
        {
            m_sceneFullColorRT.destroy();
            m_sceneFullDepthRT.destroy();
            m_sceneResolvedColor.destroy();
            m_sceneResolvedDepth.destroy();
        }

        bool FrameSurfaceCache::supports(const FrameParams_Resolution& res) const
        {
            if (res.msaaLevel != m_supportedMSAALevel)
                return false;

            if (res.width > m_maxSupportedWidth || res.height > m_maxSupportedHeight)
                return false;

            return true;
        }

        const BufferView* FrameSurfaceCache::fetchBuffer(FrameResource resourceType) const
        {
            return nullptr;
        }

        const ImageView* FrameSurfaceCache::fetchImage(FrameResource resourceType) const
        {
            switch (resourceType)
            {
                case FrameResource::HDRLinearMainColorRT: return &m_sceneFullColorRT;
                case FrameResource::HDRLinearMainDepthRT: return &m_sceneFullDepthRT;
                case FrameResource::HDRResolvedColor: return &m_sceneResolvedColor;
                case FrameResource::HDRResolvedDepth: return &m_sceneResolvedDepth;
            }

            return nullptr;
        }

        //--

        base::StringBuf FrameSurfaceCache::describe() const
        {
            return base::StringBuf("FrameSurfaceCache");
        }

        void FrameSurfaceCache::handleDeviceReset()
        {
            m_maxSupportedWidth = 0;
            m_maxSupportedHeight = 0;
            m_supportedMSAALevel = 0;
        }

        void FrameSurfaceCache::handleDeviceRelease()
        {
            m_sceneFullColorRT.destroy();
            m_sceneFullDepthRT.destroy();
            m_sceneResolvedColor.destroy();
        }

        ///--

    } // scene
} // rendering