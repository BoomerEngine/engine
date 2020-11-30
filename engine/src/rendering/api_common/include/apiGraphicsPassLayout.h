/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "apiObject.h"

#include "rendering/device/include/renderingGraphicsPassLayout.h"

namespace rendering
{
	namespace api
	{

		//---

		/// general object for pass layout
		class RENDERING_API_COMMON_API IBaseGraphicsPassLayout : public IBaseObject
		{
		public:
			IBaseGraphicsPassLayout(IBaseThread* owner, const rendering::GraphicsPassLayoutSetup& setup);
			virtual ~IBaseGraphicsPassLayout();

			static const auto STATIC_TYPE = ObjectType::GraphicsPassLayout;

			//--

			INLINE const uint64_t key() const { return m_key; }
			INLINE const GraphicsPassLayoutSetup& setup() const { return m_setup; }

			//--

		private:
			uint64_t m_key = 0;
			GraphicsPassLayoutSetup m_setup;
		};

		//---

	} // api
} // rendering