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

class ENGINE_RENDERING_API FrameHelperOutline : public NoCopy
{
public:
	FrameHelperOutline(gpu::IDevice* api); // initialized to the max resolution of the device
	~FrameHelperOutline();

	struct Setup
	{
		uint32_t sceneWidth = 0;
        uint32_t sceneHeight = 0;
		gpu::ImageSampledView* sceneDepth = nullptr;
		gpu::ImageSampledView* outlineDepth = nullptr;

		Rect presentRect; // in target viewport that we assume is bound

		float width = 4.0f;
		float innerOpacity = 0.2f;
		Color primaryColor = Color::YELLOW;
		Color backgroundColor = Color::BLUE;
	};

	// NOTE: requires active pass
	void drawOutlineEffect(gpu::CommandWriter& cmd, const Setup& setup) const;

private:
	gpu::GraphicsPipelineObjectPtr m_outlineShaderPSO;

	//--

	gpu::IDevice* m_device = nullptr;
};		
	
///---

END_BOOMER_NAMESPACE_EX(rendering)
