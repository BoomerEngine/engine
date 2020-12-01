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

		/// compute pipeline setup - all you need to dispatch
		class RENDERING_API_COMMON_API IBaseComputePipeline : public IBaseObject
		{
		public:
			IBaseComputePipeline(IBaseThread* owner, const IBaseShaders* shaders);
			virtual ~IBaseComputePipeline();

			//--

			static const auto STATIC_TYPE = ObjectType::ComputePipelineObject;

			//--

			// internal unique key (can be used to find compiled data in cache)
			INLINE uint64_t key() const { return m_key; }

			// shaders used to build the pipeline
			INLINE const IBaseShaders* shaders() const { return m_shaders; }

			//--

		private:
			uint64_t m_key = 0;
			
			const IBaseShaders* m_shaders = nullptr;
		};

		//---

	} // api
} // rendering