/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\helpers #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(rendering)

///---

class ENGINE_RENDERING_API FrameHelperCompose : public NoCopy
{
public:
	FrameHelperCompose(gpu::IDevice* api); // initialized to the max resolution of the device
	~FrameHelperCompose();

	struct Setup
	{
		// game viewport
		uint32_t gameWidth = 0;
		uint32_t gameHeight = 0;
		gpu::ImageSampledView* gameView = nullptr;

		// output presentation area
		Rect presentRect;
		const gpu::RenderTargetView* presentTarget = nullptr;

		float gamma = 1.0f;
	};

	void finalCompose(gpu::CommandWriter& cmd, const Setup& setup) const;

private:
	gpu::GraphicsPipelineObjectPtr m_blitShadersPSO;

	//--

	gpu::IDevice* m_device = nullptr;
};		
	
///---

END_BOOMER_NAMESPACE_EX(rendering)
