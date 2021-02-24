/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\helpers #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameResources.h"
#include "renderingFrameParams.h"
#include "renderingFrameHelper_Outline.h"

#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingImage.h"
#include "rendering/device/include/renderingShader.h"
#include "rendering/device/include/renderingDescriptor.h"

#include "base/containers/include/stringBuilder.h"
#include "base/resource/include/resourceStaticResource.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

        //---

		FrameHelperOutline::FrameHelperOutline(IDevice* api)
			: m_device(api)
		{
			const auto shader = LoadStaticShaderDeviceObject("screen/selection_outline.fx");
			m_outlineShaderPSO = shader->createGraphicsPipeline();
		}

		FrameHelperOutline::~FrameHelperOutline()
		{

		}

		void FrameHelperOutline::drawOutlineEffect(GPUCommandWriter& cmd, const Setup& setup) const
		{
			struct
			{
                float sourceOffsetU = 0.0f;
                float sourceOffsetV = 0.0f;
				float sourceExtentsU = 1.0f;
				float sourceExtentsV = 1.0f;

                base::Vector4 colorFront;
                base::Vector4 colorBack;
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

			DescriptorEntry desc[3];
			desc[0].constants(consts);
			desc[1] = setup.sceneDepth;
			desc[2] = setup.outlineDepth;
			cmd.opBindDescriptor("SelectionOutlineParams"_id, desc);

			cmd.opDraw(m_outlineShaderPSO, 0, 4);
	
			//--
		}

        //---


    } // scene
} // rendering

