/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "dx11FrameFence.h"

namespace rendering
{
	namespace api
	{
		namespace dx11
		{

			//---

			FrameFence::FrameFence(ID3D11DeviceContext* dxContext, ID3D11Query* dxQuery)
				: m_dxQuery(dxQuery)
				, m_dxContext(dxContext)
			{
			}

			FrameFence::~FrameFence()
			{
				DEBUG_CHECK_EX(m_signaled, "Deleting unsignalled frame fence");
				DX_RELEASE(m_dxQuery);
			}

			FrameFence::FenceResult FrameFence::check()
			{
				DEBUG_CHECK_RETURN_EX_V(!m_signaled, "Checking already signalled fence", FenceResult::Completed);

				BOOL data = FALSE;
				HRESULT hRet = m_dxContext->GetData(m_dxQuery, &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH);
				if (hRet == S_OK)
				{
					m_signaled = true;
					return FrameFence::FenceResult::Completed;
				}
				else if (hRet == S_FALSE)
				{
					return FrameFence::FenceResult::Pending;
				}

				m_signaled = true;
				return FrameFence::FenceResult::Failed;
			}

			//---

		} // dx11
	} // api
} // rendering
