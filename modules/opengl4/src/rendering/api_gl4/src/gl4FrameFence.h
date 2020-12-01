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
		namespace gl4
		{
			//---

			class FrameFence : public IBaseFrameFence
			{
			public:
				FrameFence();
				virtual ~FrameFence();

				virtual FenceResult check() override final;

			private:
				base::NativeTimePoint m_issueTime;
				bool m_signaled = false;

				GLsync m_glFence = 0;
			};

			//---

		} // gl4
    } // api
} // rendering