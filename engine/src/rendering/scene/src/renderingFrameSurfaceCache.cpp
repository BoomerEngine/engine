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

#include "rendering/device/include/renderingDeviceApi.h"
#include "renderingSelectable.h"
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

        FrameSurfaceCache::FrameSurfaceCache(IDevice* api)
            : m_device(api)
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
                if (auto obj = m_device->createImage(colorInfo))
                {
                    m_globalImages.pushBack(obj);
                    m_cascadesShadowDepthRT = obj->view();
                }
            }

            // create size dependent crap
            const auto maxWidth = m_device->maxRenderTargetSize().x;
            const auto maxHeight = m_device->maxRenderTargetSize().y;
            createViewportSurfaces(maxWidth, maxHeight);

            // create the selection capture buffer
            {
                m_maxSelectables = maxWidth * maxHeight * 2;

                BufferCreationInfo bufferInfo;
                bufferInfo.label = "SelectableCaptureBuffer";
                bufferInfo.stride = sizeof(EncodedSelectable);
                bufferInfo.size = bufferInfo.stride * m_maxSelectables;
                bufferInfo.allowShaderReads = true;
                bufferInfo.allowUAV = true;
                bufferInfo.allowCopies = true;

                if (auto obj = m_device->createBuffer(bufferInfo))
                {
                    m_viewportBuffers.pushBack(obj);
                    m_selectables = obj->view();
                }
            }

            m_debugBuffers = new DebugFragmentBuffers();
        }

        FrameSurfaceCache::~FrameSurfaceCache()
        {
            destroyViewportSurfaces();
            m_cascadesShadowDepthRT = ImageView();

            delete m_debugBuffers;
            m_debugBuffers = nullptr;
        }

        void FrameSurfaceCache::destroyViewportSurfaces()
        {
            m_viewportImages.reset();
            m_viewportBuffers.reset();

            m_globalAOShadowMaskRT = ImageView();
            m_sceneResolvedColor = ImageView();
            m_sceneResolvedDepth = ImageView();
            m_sceneFullColorRT = ImageView();
            m_sceneFullDepthRT = ImageView();
            m_linarizedDepthRT = ImageView();
            m_velocityBufferRT = ImageView();
            m_viewNormalRT = ImageView();

            m_selectables = BufferView();
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
                if (auto obj = m_device->createImage(info))
                {
                    m_viewportImages.pushBack(obj);
                    m_sceneFullColorRT = obj->view();
                }
                else
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // create depth color
            {
                info.label = "MainViewDepth";
                info.format = DepthFormat;

                if (auto obj = m_device->createImage(info))
                {
                    m_viewportImages.pushBack(obj);
                    m_sceneFullDepthRT = obj->view();
                }
                else
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // create selection depth
            {
                info.label = "SelectionDepth";
                info.format = DepthFormat;

                if (auto obj = m_device->createImage(info))
                {
                    m_viewportImages.pushBack(obj);
                    m_sceneSelectionDepthRT = obj->view();
                }
                else
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // create resolved color
            {
                info.label = "MainViewResolvedColor";
                info.format = HdrFormat;

                if (auto obj = m_device->createImage(info))
                {
                    m_viewportImages.pushBack(obj);
                    m_sceneResolvedColor = obj->view();
                }
                else
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // create resolved depth
            {
                info.label = "MainViewResolvedDepth";
                info.format = DepthFormat;

                if (auto obj = m_device->createImage(info))
                {
                    m_viewportImages.pushBack(obj);
                    m_sceneResolvedDepth = obj->view();
                }
                else
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // shadow mask/AO buffer
            {
                info.label = "ShadowMaskAO";
                info.format = ImageFormat::RGBA8_UNORM;

                if (auto obj = m_device->createImage(info))
                {
                    m_viewportImages.pushBack(obj);
                    m_globalAOShadowMaskRT = obj->view();
                }
                else
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // linearized depth buffer
            {
                info.label = "LinearizedDepth";
                info.format = ImageFormat::R32F;

                if (auto obj = m_device->createImage(info))
                {
                    m_viewportImages.pushBack(obj);
                    m_linarizedDepthRT = obj->view();
                }
                else
                {
                    destroyViewportSurfaces();
                    return false;
                }                
            }

            // velocity buffer
            {
                info.label = "VelocityBuffer";
                info.format = ImageFormat::RG16F;

                if (auto obj = m_device->createImage(info))
                {
                    m_viewportImages.pushBack(obj);
                    m_velocityBufferRT = obj->view();
                }
                else
                {
                    destroyViewportSurfaces();
                    return false;
                }
            }

            // view space reconstructed normal
            {
                info.label = "ReconstructedViewNormal";
                info.format = ImageFormat::RGBA8_UNORM;

                if (auto obj = m_device->createImage(info))
                {
                    m_viewportImages.pushBack(obj);
                    m_viewNormalRT = obj->view();
                }
                else
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

        ///--

    } // scene
} // rendering