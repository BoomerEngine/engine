/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\shaders #]
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

			Shaders::Shaders(Thread* drv, const ShaderLibraryData* data, PipelineIndex index)
				: IBaseShaders(drv, data, index)
			{}

			Shaders::~Shaders()
			{}

			IBaseGraphicsPipeline* Shaders::createGraphicsPipeline_ClientApi(const IBaseGraphicsPassLayout* passLayout, const IBaseGraphicsRenderStates* renderStates)
			{
				DEBUG_CHECK_RETURN_V(passLayout != nullptr, nullptr);
				DEBUG_CHECK_RETURN_V(renderStates != nullptr, nullptr);
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderType::Vertex), "Shader bundle has no vertex shader", nullptr);
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderType::Pixel), "Shader bundle has no pixel shader", nullptr);

				auto* localPassLayout = static_cast<const GraphicsPassLayout*>(passLayout);
				auto* localRenderStates = static_cast<const GraphicsRenderStates*>(renderStates);
				return new GraphicsPipeline(owner(), this, localPassLayout, localRenderStates);
			}

			IBaseComputePipeline* Shaders::createComputePipeline_ClientApi()
			{
				DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderType::Compute), "Shader bundle has no compute shader", nullptr);
				return new ComputePipeline(owner(), this);
			}

			//--

		} // nul
    } // api
} // rendering
