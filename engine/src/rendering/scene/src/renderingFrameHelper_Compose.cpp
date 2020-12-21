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
#include "renderingFrameHelper_Compose.h"

#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingShaderFile.h"
#include "rendering/device/include/renderingImage.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingShader.h"
#include "rendering/device/include/renderingShaderData.h"

#include "base/containers/include/stringBuilder.h"
#include "base/resource/include/resourceStaticResource.h"

namespace rendering
{
    namespace scene
    {
		//---

		static base::res::StaticResource<ShaderFile> resBlitShader("/engine/shaders/screen/final_copy.fx");

        //---

		FrameHelperCompose::FrameHelperCompose(IDevice* api)
			: m_device(api)
		{
			m_blitShaders = resBlitShader.loadAndGet()->rootShader()->deviceShader();
		}

		FrameHelperCompose::~FrameHelperCompose()
		{

		}

		void FrameHelperCompose::finalCompose(command::CommandWriter& cmd, const Setup& setup) const
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

			if (setup.gameView->image()->flippedY())
			{
                consts.sourceOffsetV = gameSourceScaleY;
                consts.sourceExtentsV = -gameSourceScaleY;
			}
			else
			{
                consts.sourceOffsetV = 0.0f;
                consts.sourceExtentsV = gameSourceScaleY;
			}

			consts.gamma = setup.gamma;

			//--

			DescriptorEntry desc[2];
			desc[0].constants(consts);
			desc[1] = setup.gameView;
			cmd.opBindDescriptor("BlitParams"_id, desc);

			//--

			if (const auto* technique = fetchTechnique(setup.presentTarget->format()))
			{
				FrameBuffer fb;
				fb.color[0].view(setup.presentTarget);

				cmd.opBeingPass(technique->layout, fb, 1, setup.presentRect);
				cmd.opDraw(technique->pso, 0, 4);
				cmd.opEndPass();
			}
		}

        //---

		const FrameHelperCompose::Technique* FrameHelperCompose::fetchTechnique(ImageFormat colorFormat) const
		{
			for (const auto& info : m_techniques)
				if (info.format == colorFormat)
					return &info;

			auto& info = m_techniques.emplaceBack();
			info.format = colorFormat;

			GraphicsPassLayoutSetup setup;
			setup.color[0].format = colorFormat;
			setup.color[0].loadOp = LoadOp::Keep;
			setup.color[0].storeOp = StoreOp::Store;
			info.layout = m_device->createGraphicsPassLayout(setup);

			info.pso = m_blitShaders->createGraphicsPipeline(info.layout);

			return &info;
		}

    } // scene
} // rendering

