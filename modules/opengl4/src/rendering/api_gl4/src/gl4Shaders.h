/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiShaders.h"
#include "gl4Thread.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{
			///--

			class ShaderCompilationJob;

			///--

			/// loaded shaders, this object mainly servers as caching interface to object cache
			class Shaders : public IBaseShaders
			{
			public:
				Shaders(Thread* drv, const ShaderData* data);
				virtual ~Shaders();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

				INLINE VertexBindingLayout* vertexLayout() const { return (VertexBindingLayout*)IBaseShaders::vertexLayout();  }
				INLINE DescriptorBindingLayout* descriptorLayout() const { return (DescriptorBindingLayout*)IBaseShaders::descriptorLayout(); }

				//--

				virtual IBaseGraphicsPipeline* createGraphicsPipeline_ClientApi(const IBaseGraphicsPassLayout* passLayout, const GraphicsRenderStatesSetup& setup) override;
				virtual IBaseComputePipeline* createComputePipeline_ClientApi() override;

				//--

				GLuint object();

			private:
				GLuint m_glProgram = 0;

				base::RefPtr<ShaderCompilationJob> m_compilationJob;
			};

			///--

		} // null
    } // api
} // rendering
        
