/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

///---

/// collection of rendering resources (mostly render targets and buffers) shared between frames
class ENGINE_RENDERING_API FrameResources : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_RENDERING_FRAME)

public:
	FrameResources(gpu::IDevice* api); // initialized to the max resolution of the device
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

    gpu::ImageObjectPtr sceneFullColor; // main scene render target, can be multisampled
	gpu::RenderTargetViewPtr sceneFullColorRTV;
	gpu::ImageReadOnlyViewPtr sceneFullColorUAV;

	gpu::ImageObjectPtr sceneFullDepth; // main scene depth buffer, can be multisampled
	gpu::RenderTargetViewPtr sceneFullDepthRTV;
	gpu::ImageReadOnlyViewPtr sceneFullDepthUAV;

	//--

	gpu::ImageObjectPtr sceneResolvedColor; // resolved copy of scene color buffer, not multisampled
	gpu::ImageSampledViewPtr sceneResolvedColorSRV;

	gpu::ImageObjectPtr sceneResolvedDepth; // resolved copy of scene depth buffer, not multisampled
	gpu::ImageSampledViewPtr sceneResolvedDepthSRV; // R32F

	//--

	gpu::ImageObjectPtr sceneOutlineDepth; // selection outline rendering depth RT
	gpu::RenderTargetViewPtr sceneOutlineDepthRTV;
	gpu::ImageSampledViewPtr sceneOutlineDepthSRV; // R32F

	//--

	gpu::ImageObjectPtr cascadesShadowDepth; // array of shadow maps for global cascade shadows, usually texture array
	gpu::RenderTargetViewPtr cascadesShadowDepthRTVArray; // rtv with all slices
	gpu::ImageSampledViewPtr cascadesShadowDepthSRVArray; // R32F
	gpu::RenderTargetViewPtr cascadesShadowDepthRTV[MAX_SHADOW_CASCADES]; // rtv for each slice

	//--

	gpu::ImageObjectPtr globalAOShadowMask; // screen size AO/shadow mask buffer
	gpu::ImageWritableViewPtr globalAOShadowMaskUAV; // RGBA
	gpu::ImageReadOnlyViewPtr globalAOShadowMaskUAV_RO; // RGBA
	//ImageSampledViewPtr globalAOShadowMaskSRV; // RGBA

	//--

	gpu::ImageObjectPtr velocityBuffer; // full scale screen space XY velocity buffer
	gpu::RenderTargetViewPtr velocityBufferRTV;
	gpu::ImageWritableViewPtr velocityBufferUAV; // RG16F
	gpu::ImageSampledViewPtr velocityBufferSRV; // RG16F

	//--

	gpu::ImageObjectPtr linarizedDepth; // full scale linearized depth (AO)
	gpu::ImageWritableViewPtr linarizedDepthUAV; // R32F
	gpu::ImageSampledViewPtr linarizedDepthSRV; // R32F

	gpu::ImageObjectPtr viewNormal; // full scale linearized reconstructed normal (AO)
	gpu::ImageWritableViewPtr viewNormalUAV; // RG16F
	gpu::ImageSampledViewPtr viewNormalSRV; // RG16F

	//-

	gpu::BufferObjectPtr selectablesBuffer; // capture buffer for selection
	gpu::BufferWritableStructuredViewPtr selectablesBufferUAV; // export buffer for selection

    uint32_t maxSelectables = 0; // maximum number of selectable fragments we can capture

	//--

	//BufferObjectPtr debugVertexBuffer; // debug rendering storage vertex buffer
	//BufferObjectPtr debugindexBuffer; // debug rendering storage index buffer

    //--

private:
	gpu::IDevice* m_device = nullptr;

    uint32_t m_maxSupportedWidth = 0;
    uint32_t m_maxSupportedHeight = 0;

    void createViewportSurfaces(uint32_t width, uint32_t height);
    void destroyViewportSurfaces();

	void createGlobalResources();
};

///---

END_BOOMER_NAMESPACE()
