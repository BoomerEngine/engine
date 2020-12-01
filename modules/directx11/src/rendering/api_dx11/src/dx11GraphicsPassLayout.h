/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiGraphicsPassLayout.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{
			///---

			class GraphicsPassLayout : public IBaseGraphicsPassLayout
			{
			public:
				GraphicsPassLayout(Thread* owner, const rendering::GraphicsPassLayoutSetup& setup);
				virtual ~GraphicsPassLayout();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

				//--

			private:
			};

			//--

		} // dx11
    } // api
} // rendering

