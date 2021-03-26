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
#include "helperCompose.h"

#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/commandBuffer.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/image.h"
#include "gpu/device/include/descriptor.h"
#include "gpu/device/include/shader.h"

#include "core/containers/include/stringBuilder.h"
#include "core/resource/include/staticResource.h"

BEGIN_BOOMER_NAMESPACE()

//---

FrameHelperCompose::FrameHelperCompose(gpu::IDevice* api)
	: m_device(api)
{
	const auto shader = gpu::LoadStaticShaderDeviceObject("screen/final_copy.fx");
	m_blitShadersPSO = shader->createGraphicsPipeline();
}

FrameHelperCompose::~FrameHelperCompose()
{

}

void FrameHelperCompose::finalCompose(gpu::CommandWriter& cmd, const Setup& setup) const
{
	struct
	{
		int targetOffsetX = 0;
		int targetOffsetY = 0;
		float targetInvSizeX = 0;
		float targetInvSizeY = 0;

        float sourceOffsetU = 0.0f;
        float sourceOffsetV = 0.0f;
		float sourceExtentsU = 1.0f;
		float sourceExtentsV = 1.0f;

		float gamma = 1.0f;

		float padding = 0.0f;

	} consts;

	consts.targetOffsetX = setup.presentRect.min.x;
	consts.targetOffsetY = setup.presentRect.min.y;
	consts.targetInvSizeX = 1.0f / setup.presentRect.width();
	consts.targetInvSizeY = 1.0f / setup.presentRect.height();

	const auto sourceInvWidth = 1.0f / (float)setup.gameView->image()->width();
	const auto sourceInvHeight = 1.0f / (float)setup.gameView->image()->height();

	const auto gameSourceScaleX = setup.gameWidth * sourceInvWidth;
	const auto gameSourceScaleY = setup.gameHeight * sourceInvHeight;
	consts.sourceOffsetU = 0.0f;
	consts.sourceExtentsU = gameSourceScaleX;

	if (setup.presentTarget->flipped())
	{
        //consts.sourceOffsetV = 1.0f - gameSourceScaleY;
        //consts.sourceExtentsV = gameSourceScaleY;
        consts.sourceOffsetV = 0.0f;
        consts.sourceExtentsV = gameSourceScaleY;
	}
	else
	{
        consts.sourceOffsetV = 0.0f;
        consts.sourceExtentsV = gameSourceScaleY;
	}

	consts.gamma = setup.gamma;

	//--

	gpu::DescriptorEntry desc[2];
	desc[0].constants(consts);
	desc[1] = setup.gameView;
	cmd.opBindDescriptor("BlitParams"_id, desc);

	//--

	{
		//FrameBuffer fb;
		//fb.color[0].view(setup.presentTarget);

		//cmd.opBeingPass(fb, 1, setup.presentRect);
		cmd.opDraw(m_blitShadersPSO, 0, 4);
		//cmd.opEndPass();
	}

	//--
}

//---

END_BOOMER_NAMESPACE()
