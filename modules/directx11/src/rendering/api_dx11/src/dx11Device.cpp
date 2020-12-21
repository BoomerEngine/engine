/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "dx11Device.h"
#include "dx11Thread.h"
#include "base/app/include/commandline.h"

#pragma comment(lib, "D3D11.lib")

namespace rendering
{
	namespace api
	{
		namespace dx11
		{
			//--

			RTTI_BEGIN_TYPE_CLASS(Device);
				RTTI_METADATA(DeviceNameMetadata).name("DX11");
			RTTI_END_TYPE();

			Device::Device()
			{
			}

			Device::~Device()
			{
				ASSERT_EX(m_dxgi == nullptr, "Improper shutdown");
			}
        
			base::StringBuf Device::name() const
			{
				return "DX11";
			}

			bool Device::initialize(const base::app::CommandLine& cmdLine, DeviceCaps& outCaps)
			{
				m_dxgi = new DXGIHelper();
				if (!m_dxgi->initialize(cmdLine))
				{
					TRACE_ERROR("Failed to initialize DXGI");
					return false;
				}

				return TBaseClass::initialize(cmdLine, outCaps);
			}

			void Device::shutdown()
			{
				TBaseClass::shutdown();

				delete m_dxgi;
				m_dxgi = nullptr;
			}

			IBaseThread* Device::createOptimalThread(const base::app::CommandLine& cmdLine)
			{
				return new Thread(this, windows(), m_dxgi);
			}

			//---

		} // dx11
	} // api
} // rendering
