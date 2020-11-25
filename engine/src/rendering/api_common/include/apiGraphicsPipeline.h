/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "apiObject.h"

namespace rendering
{
	namespace api
	{

		//---

		/// graphics pipeline setup - all you need to draw (besides dynamic states...)
		class RENDERING_API_COMMON_API IBaseGraphicsPipeline : public IBaseObject
		{
		public:
			IBaseGraphicsPipeline(IBaseThread* owner, const IBaseShaders* shaders, const IBaseGraphicsPassLayout* passLayout, const IBaseGraphicsRenderStates* states);
			virtual ~IBaseGraphicsPipeline();

			//--

			// internal unique key (can be used to find compiled data in cache)
			INLINE uint64_t key() const { return m_key; }

			// shaders used to build the pipeline
			INLINE const IBaseShaders* shaders() const { return m_shaders; }

			// pass layout (render target formats)
			INLINE const IBaseGraphicsPassLayout* passLayout() const { return m_passLayout; }

			// rendering states
			INLINE const IBaseGraphicsRenderStates* renderStates() const { return m_renderStates; }

			//--

		private:
			uint64_t m_key = 0;
			
			const IBaseShaders* m_shaders = nullptr;
			const IBaseGraphicsPassLayout* m_passLayout = nullptr;
			const IBaseGraphicsRenderStates* m_renderStates = nullptr;
		};

		//---

	} // api
} // rendering