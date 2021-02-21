/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#include "build.h"
#include "renderingFrameParams.h"
#include "renderingFrameResources.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "renderingSelectable.h"

namespace rendering
{
    namespace scene
    {
        ///--

        base::ConfigProperty<uint32_t> cvCascadeShadowmapBufferSize("Rendering.Cascades", "Resolution", 2048);
        base::ConfigProperty<uint32_t> cvCascadeShadowmapMaxCount("Rendering.Cascades", "MaxCount", 4);

        ///--

        // BIG TODO: MSAA

        FrameResources::FrameResources(IDevice* api)
            : m_device(api)
        {
			// create the resources
			createGlobalResources();
			
			// create size dependent crap
			// TODO: in full screen mode this may be an overkill
			const auto maxWidth = m_device->maxRenderTargetSize().x;
			const auto maxHeight = m_device->maxRenderTargetSize().y;
			createViewportSurfaces(maxWidth, maxHeight);
        }

        FrameResources::~FrameResources()
        {
            destroyViewportSurfaces();
        }

		void FrameResources::createGlobalResources()
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
				colorInfo.format = shadowMapFormat;
				cascadesShadowDepth = m_device->createImage(colorInfo);
				cascadesShadowDepthRTVArray = cascadesShadowDepth->createRenderTargetView(0, 0, count);
				cascadesShadowDepthSRVArray = cascadesShadowDepth->createSampledView();

				for (uint32_t i = 0; i < count; ++i)
					cascadesShadowDepthRTV[i] = cascadesShadowDepth->createRenderTargetView(0, i, 1);
			}
		}

        void FrameResources::destroyViewportSurfaces()
        {
			sceneFullColor.reset();
			sceneFullColorRTV.reset();
			sceneFullColorUAV.reset();

			sceneFullDepth.reset();
			sceneFullDepthRTV.reset();
			sceneFullDepthUAV.reset();

			sceneResolvedColor.reset();
//			sceneResolvedColorUAV.reset();
			sceneResolvedColorSRV.reset();

			sceneResolvedDepth.reset();
			sceneResolvedDepthSRV.reset();

			sceneOutlineDepth.reset();
			sceneOutlineDepthRTV.reset();
			sceneOutlineDepthSRV.reset();

			globalAOShadowMask.reset();
			globalAOShadowMaskUAV.reset();
			//globalAOShadowMaskSRV.reset();
			globalAOShadowMaskUAV_RO.reset();

			velocityBuffer.reset();
			velocityBufferRTV.reset();
			velocityBufferUAV.reset();
			velocityBufferSRV.reset();

			linarizedDepth.reset();
			linarizedDepthUAV.reset();
			linarizedDepthSRV.reset();

			viewNormal.reset();
			viewNormalUAV.reset();
			viewNormalSRV.reset();
        }
		
		void FrameResources::createViewportSurfaces(uint32_t width, uint32_t height)
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
                info.format = sceneColorFormat;
				info.allowRenderTarget = true;

				sceneFullColor = m_device->createImage(info);
				sceneFullColorRTV = sceneFullColor->createRenderTargetView();
				sceneFullColorUAV = sceneFullColor->createReadOnlyView();
            }

            // create depth color
            {
                info.label = "MainViewDepth";
                info.format = sceneDepthFormat;
				info.allowRenderTarget = true;

				sceneFullDepth = m_device->createImage(info);
				sceneFullDepthRTV = sceneFullDepth->createRenderTargetView();
				sceneFullDepthUAV = sceneFullDepth->createReadOnlyView();
            }

            // create depth buffer for outline rendering
			{
				info.label = "OutlineDepth";
				info.format = sceneDepthFormat;
				info.allowRenderTarget = true;

				sceneOutlineDepth = m_device->createImage(info);
				sceneOutlineDepthRTV = sceneOutlineDepth->createRenderTargetView();
				sceneOutlineDepthSRV = sceneOutlineDepth->createSampledView();
			}

            // create resolved color
            {
                info.label = "MainViewResolvedColor";
                info.format = resolvedColorFormat;
				info.allowRenderTarget = false;

				sceneResolvedColor = m_device->createImage(info);
				sceneResolvedColorSRV = sceneResolvedColor->createSampledView();
            }

            // create resolved depth
            {
                info.label = "MainViewResolvedDepth";
                info.format = resolvedDepthFormat;
				info.allowRenderTarget = false;

				sceneResolvedDepth = m_device->createImage(info);
				sceneResolvedDepthSRV = sceneResolvedDepth->createSampledView();
            }

            // shadow mask/AO buffer
            {
                info.label = "ShadowMaskAO";
                info.format = globalAOFormat;
				info.allowRenderTarget = false;

				globalAOShadowMask = m_device->createImage(info);
				globalAOShadowMaskUAV = globalAOShadowMask->createWritableView();
				globalAOShadowMaskUAV_RO = globalAOShadowMask->createReadOnlyView();
				//globalAOShadowMaskSRV = globalAOShadowMask->createSampledView();
            }

            // linearized depth buffer
            {
                info.label = "LinearizedDepth";
                info.format = ImageFormat::R32F;
				info.allowRenderTarget = false;

				linarizedDepth = m_device->createImage(info);
				linarizedDepthUAV = linarizedDepth->createWritableView();
				linarizedDepthSRV = linarizedDepth->createSampledView();
            }

            // velocity buffer
            {
                info.label = "VelocityBuffer";
                info.format = velocityBufferFormat;
				info.allowRenderTarget = true;

				velocityBuffer = m_device->createImage(info);
				velocityBufferSRV = velocityBuffer->createSampledView();
				velocityBufferUAV = velocityBuffer->createWritableView();
				velocityBufferRTV = velocityBuffer->createRenderTargetView();
            }

            // view space reconstructed normal
            {
                info.label = "ReconstructedViewNormal";
                info.format = ImageFormat::RGBA8_UNORM;
				info.allowRenderTarget = false;

				viewNormal = m_device->createImage(info);
				viewNormalSRV = viewNormal->createSampledView();
				viewNormalUAV = viewNormal->createWritableView();
            }

			// create the selection capture buffer
			{
				const auto maxWidth = m_device->maxRenderTargetSize().x;
				const auto maxHeight = m_device->maxRenderTargetSize().y;
				maxSelectables = maxWidth * maxHeight * 2;

				BufferCreationInfo bufferInfo;
				bufferInfo.label = "SelectableCaptureBuffer";
				bufferInfo.stride = sizeof(EncodedSelectable);
				bufferInfo.size = bufferInfo.stride * maxSelectables;
				bufferInfo.allowShaderReads = true;
				bufferInfo.allowUAV = true;
				bufferInfo.allowCopies = true;

				selectablesBuffer = m_device->createBuffer(bufferInfo);
				selectablesBufferUAV = selectablesBuffer->createWritableStructuredView();
			}

            // yay, now we can support such resolution
            m_maxSupportedWidth = width;
            m_maxSupportedHeight = height;
        }

        void FrameResources::adjust(uint32_t requiredWidth, uint32_t requiredHeight)
        {
            auto newWidth = std::max<uint32_t>(requiredWidth, m_maxSupportedWidth);
            auto newHeight = std::max<uint32_t>(requiredHeight, m_maxSupportedHeight);

            // TODO: validate memory usage

            if (newWidth > m_maxSupportedWidth || newHeight > m_maxSupportedHeight)
            {
                destroyViewportSurfaces();
				createViewportSurfaces(newWidth, newHeight);
            }
        }

        ///--

    } // scene
} // rendering