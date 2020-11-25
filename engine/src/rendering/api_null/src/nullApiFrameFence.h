/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiFrame.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{
			//---

			class FrameFence : public IBaseFrameFence
			{
			public:
				FrameFence(float timeout);
				virtual ~FrameFence();

				virtual FenceResult check() override final;

			private:
				base::NativeTimePoint m_expirationTime;
				bool m_signaled = false;
			};

			//---

		} // nul
    } // api
} // rendering