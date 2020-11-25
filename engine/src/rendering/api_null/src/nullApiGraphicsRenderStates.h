/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiGraphicsRenderStates.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{
			///---

			class GraphicsRenderStates : public IBaseGraphicsRenderStates
			{
			public:
				GraphicsRenderStates(Thread* owner, const rendering::StaticRenderStatesSetup& setup);
				virtual ~GraphicsRenderStates();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

				//--

			private:
			};

			//--

		} // nul
    } // api
} // rendering

