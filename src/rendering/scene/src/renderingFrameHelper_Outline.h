/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\helpers #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(rendering::scene)

///---

class RENDERING_SCENE_API FrameHelperOutline : public base::NoCopy
{
public:
	FrameHelperOutline(IDevice* api); // initialized to the max resolution of the device
	~FrameHelperOutline();

	struct Setup
	{
		uint32_t sceneWidth = 0;
        uint32_t sceneHeight = 0;
		ImageSampledView* sceneDepth = nullptr;
		ImageSampledView* outlineDepth = nullptr;

		base::Rect presentRect; // in target viewport that we assume is bound

		float width = 4.0f;
		float innerOpacity = 0.2f;
		base::Color primaryColor = base::Color::YELLOW;
		base::Color backgroundColor = base::Color::BLUE;
	};

	// NOTE: requires active pass
	void drawOutlineEffect(GPUCommandWriter& cmd, const Setup& setup) const;

private:
    GraphicsPipelineObjectPtr m_outlineShaderPSO;

	//--

	IDevice* m_device = nullptr;
};		
	
///---

END_BOOMER_NAMESPACE(rendering::scene)