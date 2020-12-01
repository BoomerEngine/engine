/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4Shaders.h"
#include "gl4ComputePipeline.h"
#include "gl4GraphicsPipeline.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			Shaders::Shaders(Thread* drv, const ShaderData* data)
				: IBaseShaders(drv, data)
			{}

			Shaders::~Shaders()
			{}

			IBaseGraphicsPipeline* Shaders::createGraphicsPipeline_ClientApi(const IBaseGraphicsPassLayout* passLayout, const GraphicsRenderStatesSetup& setup)
			{
				DEBUG_CHECK_RETURN_V(passLayout != nullptr, nullptr);
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Vertex), "Shader bundle has no vertex shader", nullptr);
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Pixel), "Shader bundle has no pixel shader", nullptr);

				auto* localPassLayout = static_cast<const GraphicsPassLayout*>(passLayout);
				return new GraphicsPipeline(owner(), this, localPassLayout, setup);
			}

			IBaseComputePipeline* Shaders::createComputePipeline_ClientApi()
			{
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Compute), "Shader bundle has no compute shader", nullptr);
				return new ComputePipeline(owner(), this);
			}

			//--

		} // gl4
    } // api
} // rendering
