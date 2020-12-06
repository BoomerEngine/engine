/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "gl4Shaders.h"
#include "gl4GraphicsPassLayout.h"
#include "gl4StateCache.h"

#include "rendering/api_common/include/apiGraphicsPipeline.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{
			///--

			class GraphicsPipeline : public IBaseGraphicsPipeline
			{
			public:
				GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsPassLayout* passLayout, const GraphicsRenderStatesSetup& mergedRenderStates);
				virtual ~GraphicsPipeline();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }
				INLINE Shaders* shaders() const { return const_cast<Shaders*>(static_cast<const Shaders*>(IBaseGraphicsPipeline::shaders())); }
				INLINE const GraphicsPassLayout* passLayout() const { return static_cast<const GraphicsPassLayout*>(IBaseGraphicsPipeline::passLayout()); }

				INLINE const StateValues& staticRenderState() const { return m_staticRenderStates; }
				INLINE StateMask staticRenderStateMask() const { return m_staticRenderStateMask; }
				
				//--				

				bool apply(GLuint& glActiveProgram);

				//--

			private:
				StateValues m_staticRenderStates;
				StateMask m_staticRenderStateMask;
			};

			//--

		} // gl4
    } // api
} // rendering

