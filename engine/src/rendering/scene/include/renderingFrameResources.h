/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

namespace rendering
{
    namespace scene
    {

        ///---

        /// collection of rendering resources (mostly render targets and buffers) shared between frames
        class RENDERING_SCENE_API FrameResources : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_RENDERING_FRAME)

        public:
			FrameResources(IDevice* api); // initialized to the max resolution of the device
            virtual ~FrameResources();

            //--

            // adjust to support given resolution, may fail
            void adjust(uint32_t requiredWidth, uint32_t requiredHeight);

			//--

			ImageFormat sceneColorFormat = ImageFormat::RGBA16F;
			ImageFormat sceneDepthFormat = ImageFormat::D24S8;
			ImageFormat shadowMapFormat = ImageFormat::D32;
			ImageFormat velocityBufferFormat = ImageFormat::RG16F;
			ImageFormat globalAOFormat = ImageFormat::RGBA8_UNORM;
			ImageFormat resolvedColorFormat = ImageFormat::RGBA16F;
			ImageFormat resolvedDepthFormat = ImageFormat::D24S8;

            //--

            ImageObjectPtr sceneFullColor; // main scene render target, can be multisampled
			RenderTargetViewPtr sceneFullColorRTV;
			ImageReadOnlyViewPtr sceneFullColorUAV;

			ImageObjectPtr sceneFullDepth; // main scene depth buffer, can be multisampled
			RenderTargetViewPtr sceneFullDepthRTV;
			ImageReadOnlyViewPtr sceneFullDepthUAV;

			GraphicsPassLayoutObjectPtr sceneFullDepthPrePassLayout;
			GraphicsPassLayoutObjectPtr sceneFullDepthPrePassWithVelocityBufferLayout;
			GraphicsPassLayoutObjectPtr sceneFullColorPassLayout;
			GraphicsPassLayoutObjectPtr sceneFullColorOverlayLayout;

			//--

			ImageObjectPtr sceneResolvedColor; // resolved copy of scene color buffer, not multisampled
			ImageSampledViewPtr sceneResolvedColorSRV;

			ImageObjectPtr sceneResolvedDepth; // resolved copy of scene depth buffer, not multisampled
			ImageSampledViewPtr sceneResolvedDepthSRV; // R32F

			//--

			ImageObjectPtr sceneOutlineDepth; // selection outline rendering depth RT
			RenderTargetViewPtr sceneOutlineDepthRTV;
			ImageSampledViewPtr sceneOutlineDepthSRV; // R32F

			//--

			ImageObjectPtr cascadesShadowDepth; // array of shadow maps for global cascade shadows, usually texture array
			RenderTargetViewPtr cascadesShadowDepthRTVArray; // rtv with all slices
			ImageSampledViewPtr cascadesShadowDepthSRVArray; // R32F
			RenderTargetViewPtr cascadesShadowDepthRTV[MAX_CASCADES]; // rtv for each slice

			//--

			ImageObjectPtr globalAOShadowMask; // screen size AO/shadow mask buffer
			ImageWritableViewPtr globalAOShadowMaskUAV; // RGBA
			ImageSampledViewPtr globalAOShadowMaskSRV; // RGBA

			//--

			ImageObjectPtr velocityBuffer; // full scale screen space XY velocity buffer
			RenderTargetViewPtr velocityBufferRTV;
			ImageWritableViewPtr velocityBufferUAV; // RG16F
			ImageSampledViewPtr velocityBufferSRV; // RG16F

			//--

			ImageObjectPtr linarizedDepth; // full scale linearized depth (AO)
			ImageWritableViewPtr linarizedDepthUAV; // R32F
			ImageSampledViewPtr linarizedDepthSRV; // R32F

			ImageObjectPtr viewNormal; // full scale linearized reconstructed normal (AO)
			ImageWritableViewPtr viewNormalUAV; // RG16F
			ImageSampledViewPtr viewNormalSRV; // RG16F

			//-

            BufferObjectPtr m_selectablesBuffer; // capture buffer for selection
			BufferWritableStructuredViewPtr m_selectablesBufferUAV; // export buffer for selection

            uint32_t m_maxSelectables = 0; // maximum number of selectable fragments we can capture

			//--

			//BufferObjectPtr debugVertexBuffer; // debug rendering storage vertex buffer
			//BufferObjectPtr debugindexBuffer; // debug rendering storage index buffer

            //--

        private:
            IDevice* m_device = nullptr;

            uint32_t m_maxSupportedWidth = 0;
            uint32_t m_maxSupportedHeight = 0;

            void createViewportSurfaces(uint32_t width, uint32_t height);
            void destroyViewportSurfaces();

			void createGlobalResources();
			void createPassLayouts();
        };

        ///---

    } // scene
} // rendering