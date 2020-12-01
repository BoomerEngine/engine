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
		namespace dx11
		{
			//---

			class FrameFence : public IBaseFrameFence
			{
			public:
				FrameFence(ID3D11DeviceContext* dxContext, ID3D11Query* dxQuery);
				virtual ~FrameFence();

				virtual FenceResult check() override final;

			private:
				ID3D11Query* m_dxQuery = nullptr;
				ID3D11DeviceContext* m_dxContext = nullptr;

				bool m_signaled = false;
			};

			//---

		} // dx11
    } // api
} // rendering