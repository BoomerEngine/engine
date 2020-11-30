/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\shaders #]
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

			IBaseGraphicsPipeline* Shaders::createGraphicsPipeline_ClientApi(const IBaseGraphicsPassLayout* passLayout, const IBaseGraphicsRenderStates* renderStates)
			{
				DEBUG_CHECK_RETURN_V(passLayout != nullptr, nullptr);
				DEBUG_CHECK_RETURN_V(renderStates != nullptr, nullptr);
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Vertex), "Shader bundle has no vertex shader", nullptr);
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Pixel), "Shader bundle has no pixel shader", nullptr);

				auto* localPassLayout = static_cast<const GraphicsPassLayout*>(passLayout);
				auto* localRenderStates = static_cast<const GraphicsRenderStates*>(renderStates);
				return new GraphicsPipeline(owner(), this, localPassLayout, localRenderStates);
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
