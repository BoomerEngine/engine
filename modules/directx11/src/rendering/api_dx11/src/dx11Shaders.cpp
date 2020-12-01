/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\shaders #]
***/

#include "build.h"
#include "dx11Shaders.h"
#include "dx11ComputePipeline.h"
#include "dx11GraphicsPipeline.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			//--

			Shaders::Shaders(Thread* drv, const ShaderData* data)
				: IBaseShaders(drv, data)
			{}

			Shaders::~Shaders()
			{}

			IBaseGraphicsPipeline* Shaders::createGraphicsPipeline_ClientApi(const IBaseGraphicsPassLayout* passLayout, const GraphicsRenderStatesSetup& mergedRenderStates)
			{
				DEBUG_CHECK_RETURN_V(passLayout != nullptr, nullptr);
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Vertex), "Shader bundle has no vertex shader", nullptr);
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Pixel), "Shader bundle has no pixel shader", nullptr);

				auto* localPassLayout = static_cast<const GraphicsPassLayout*>(passLayout);
				return new GraphicsPipeline(owner(), this, localPassLayout, mergedRenderStates);
			}

			IBaseComputePipeline* Shaders::createComputePipeline_ClientApi()
			{
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Compute), "Shader bundle has no compute shader", nullptr);
				return new ComputePipeline(owner(), this);
			}

			//--

		} // dx11
    } // api
} // rendering
