/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiShaders.h"
#include "nullApiComputePipeline.h"
#include "nullApiGraphicsPipeline.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{

			//--

			Shaders::Shaders(Thread* drv, const ShaderData* data)
				: IBaseShaders(drv, data)
			{}

			Shaders::~Shaders()
			{}

			IBaseGraphicsPipeline* Shaders::createGraphicsPipeline_ClientApi(const GraphicsRenderStatesSetup& mergedRenderStates)
			{
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Vertex), "Shader bundle has no vertex shader", nullptr);
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Pixel), "Shader bundle has no pixel shader", nullptr);

				return new GraphicsPipeline(owner(), this, mergedRenderStates);
			}

			IBaseComputePipeline* Shaders::createComputePipeline_ClientApi()
			{
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Compute), "Shader bundle has no compute shader", nullptr);
				return new ComputePipeline(owner(), this);
			}

			//--

		} // nul
    } // api
} // rendering
