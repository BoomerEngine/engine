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

        base::ConfigProperty<uint32_t> cvCascadeShadowmapBufferSize("Rendering.Cascades", "Resolution", 2048);
        base::ConfigProperty<uint32_t> cvCascadeShadowmapMaxCount("Rendering.Cascades", "MaxCount", 4);

        ///--

        // BIG TODO: MSAA

        FrameSurfaceCache::FrameSurfaceCache()
        {
            // cascade shadowmap buffer
            {
                const auto count = std::clamp<uint32_t>(cvCascadeShadowmapMaxCount.get(), 1, 4);
                const auto resolution = std::clamp<uint32_t>(cvCascadeShadowmapBufferSize.get(), 256, 8192);

                ImageCreationInfo colorInfo;
                colorInfo.label = "CascadeShadowDepthMap";
                colorInfo.view = ImageViewType::View2DArray;
                colorInfo.width = resolution;
                colorInfo.height = resolution;
                colorInfo.numSlices = count;
                colorInfo.allowRenderTarget = true;
                colorInfo.allowShaderReads = true;
                colorInfo.allowUAV = true;
                colorInfo.format = ShadowDepthFormat;
                m_cascadesShadowDepthRT = device()->createImage(colorInfo);
            }

            // create size dependent crap
            const auto maxWidth = device()->maxRenderTargetSize().x;
            const auto maxHeight = device()->maxRenderTargetSize().y;
            createViewportSurfaces(maxWidth, maxHeight);
        }

        FrameSurfaceCache::~FrameSurfaceCache()
        {
            m_cascadesShadowDepthRT.destroy();
            destroyViewportSurfaces();
        }

        void FrameSurfaceCache::destroyViewportSurfaces()
        {
            m_globalAOShadowMaskRT.destroy();
            m_sceneResolvedColor.destroy();
            m_sceneResolvedDepth.destroy();
            m_sceneFullColorRT.destroy();
            m_sceneFullDepthRT.destroy();
            m_linarizedDepthRT.destroy();
            m_velocityBufferRT.destroy();
            m_viewNormalRT.destroy();
        }

        bool FrameSurfaceCache::createViewportSurfaces(uint32_t width, uint32_t height)
        {
            ImageCreationInfo info;
            info.view = ImageViewType::View2D; // TODO: array for VR ?
            info.width = width;
            info.height = height;
            info.allowRenderTarget = true;
            info.allowShaderReads = true;
            info.allowUAV = true;

            // create scene color
            {
                info.label = "MainViewColor";
                info.format = HdrFormat;
                m_sceneFullColorRT = device()->createImage(info);
                if (!m_sceneFullColorRT)
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // create depth color
            {
                info.label = "MainViewDepth";
                info.format = DepthFormat;
                m_sceneFullDepthRT = device()->createImage(info);
                if (!m_sceneFullDepthRT)
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // create resolved color
            {
                info.label = "MainViewResolvedColor";
                info.format = HdrFormat;
                m_sceneResolvedColor = device()->createImage(info);
                if (!m_sceneResolvedColor)
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // create resolved depth
            {
                info.label = "MainViewResolvedDepth";
                info.format = DepthFormat;
                m_sceneResolvedDepth = device()->createImage(info);
                if (!m_sceneResolvedDepth)
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // shadow mask/AO buffer
            {
                info.label = "ShadowMaskAO";
                info.format = ImageFormat::RGBA8_UNORM;
                m_globalAOShadowMaskRT = device()->createImage(info);
                if (!m_globalAOShadowMaskRT)
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // linearized depth buffer
            {
                info.label = "LinearizedDepth";
                info.format = ImageFormat::R32F;
                m_linarizedDepthRT = device()->createImage(info);
                if (!m_linarizedDepthRT)
                {
                    destroyViewportSurfaces();
                    return false;
                }                
            }

            // velocity buffer
            {
                info.label = "VelocityBuffer";
                info.format = ImageFormat::RG16F;
                m_velocityBufferRT = device()->createImage(info);
                if (!m_velocityBufferRT)
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // view space reconstructed normal
            {
                info.label = "ReconstructedViewNormal";
                info.format = ImageFormat::RGBA8_UNORM;
                m_viewNormalRT = device()->createImage(info);
                if (!m_viewNormalRT)
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // yay, now we can support such resolution
            m_maxSupportedWidth = width;
            m_maxSupportedHeight = height;
            return true;
        }

        bool FrameSurfaceCache::adjust(uint32_t requiredWidth, uint32_t requiredHeight)
        {
            auto newWidth = std::max<uint32_t>(requiredWidth, m_maxSupportedWidth);
            auto newHeight = std::max<uint32_t>(requiredHeight, m_maxSupportedHeight);

            // TODO: validate memory usage

            if (!m_sceneFullColorRT || newWidth > m_maxSupportedWidth || newHeight > m_maxSupportedHeight)
            {
                destroyViewportSurfaces();

                if (!createViewportSurfaces(newWidth, newHeight))
                    return false;
            }

            return true;
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
            //m_supportedMSAALevel = 0;
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