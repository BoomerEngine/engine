/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\shaders #]
***/

#pragma once

#include "rendering/api_common/include/apiShaders.h"
#include "dx11Thread.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			/// loaded shaders, this object mainly servers as caching interface to object cache
			class Shaders : public IBaseShaders
			{
			public:
				Shaders(Thread* drv, const ShaderData* data);
				virtual ~Shaders();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

				//--

				virtual IBaseGraphicsPipeline* createGraphicsPipeline_ClientApi(const IBaseGraphicsPassLayout* passLayout, const GraphicsRenderStatesSetup& mergedRenderStates) override;
				virtual IBaseComputePipeline* createComputePipeline_ClientApi() override;

				//--            

			private:
			};

		} // null
    } // api
} // rendering
        
