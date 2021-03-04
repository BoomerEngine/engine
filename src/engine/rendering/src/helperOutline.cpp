/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\helpers #]
***/

#include "build.h"
#include "scene.h"
#include "renderer.h"
#include "cameraContext.h"
#include "resources.h"
#include "params.h"
#include "helperOutline.h"

#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/commandBuffer.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/image.h"
#include "gpu/device/include/shader.h"
#include "gpu/device/include/descriptor.h"

#include "core/containers/include/stringBuilder.h"
#include "core/resource/include/staticResource.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//---

FrameHelperOutline::FrameHelperOutline(gpu::IDevice* api)
	: m_device(api)
{
	const auto shader = gpu::LoadStaticShaderDeviceObject("screen/selection_outline.fx");
	m_outlineShaderPSO = shader->createGraphicsPipeline();
}

FrameHelperOutline::~FrameHelperOutline()
{

}

void FrameHelperOutline::drawOutlineEffect(gpu::CommandWriter& cmd, const Setup& setup) const
{
	struct
	{
        float sourceOffsetU = 0.0f;
        float sourceOffsetV = 0.0f;
		float sourceExtentsU = 1.0f;
		float sourceExtentsV = 1.0f;

        Vector4 colorFront;
        Vector4 colorBack;
        float outlineSize = 4.0f;
        float centerOpacity = 0.5f;
	} consts;

	consts.sourceOffsetU = 0.0f;
    consts.sourceOffsetV = 0.0f;
	consts.sourceExtentsU = setup.sceneWidth / (float)setup.presentRect.width();
	consts.sourceExtentsV = setup.sceneHeight / (float)setup.presentRect.height();

	consts.colorFront = setup.primaryColor.toVectorLinear();
	consts.colorBack = setup.backgroundColor.toVectorLinear();
	consts.outlineSize = std::max<float>(1.0f, setup.width);
	consts.centerOpacity = setup.innerOpacity;

	//--

	gpu::DescriptorEntry desc[3];
	desc[0].constants(consts);
	desc[1] = setup.sceneDepth;
	desc[2] = setup.outlineDepth;
	cmd.opBindDescriptor("SelectionOutlineParams"_id, desc);

	cmd.opDraw(m_outlineShaderPSO, 0, 4);
	
	//--
}

//---

END_BOOMER_NAMESPACE_EX(rendering)