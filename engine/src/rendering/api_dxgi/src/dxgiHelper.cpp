/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: dxgi #]
***/

#include "build.h"
#include "dxgiHelper.h"
#include "base/app/include/commandline.h"

#pragma comment (lib, "DXGI.lib")

namespace rendering
{
	namespace api
	{

		//---

		DXGIHelper::DXGIHelper()
		{}

		DXGIHelper::~DXGIHelper()
		{
			for (auto& display : m_cachedDisplays)
				DX_RELEASE(display.dxOutput);

			DX_RELEASE(m_dxAdapter);
			DX_RELEASE(m_dxFactory);

			TRACE_INFO("DXGI shut down");
		}

		bool DXGIHelper::initialize(const base::app::CommandLine& cmdLine)
		{
			// create factory
			IDXGIFactory1* pFactory = nullptr;
			DXGI_PROTECT(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&pFactory)));
			DEBUG_CHECK_RETURN_EX_V(pFactory != nullptr, "Failed to create DXGI factory", false);
			m_dxFactory = pFactory;

			// initialize selected adapter
			if (!initializeAdapter(cmdLine))
				return false;

			// enumerate adapters
			enumerateAdapters();

			// done
			TRACE_INFO("DXGI initialized");
			return true;
		}

		void DXGIHelper::enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const
		{
			outMonitorAreas = m_cachedMonitors;
		}

		void DXGIHelper::enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const
		{
			outDisplayInfos.reserve(m_cachedDisplays.size());
			for (const auto& info : m_cachedDisplays)
				outDisplayInfos.pushBack(info.displayInfo);
		}

		void DXGIHelper::enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const
		{
			if (displayIndex < m_cachedDisplays.size())
			{
				const auto& display = m_cachedDisplays[displayIndex];
				outResolutions = display.resolutions;
			}
		}
		
		///--

		bool DXGIHelper::initializeAdapter(const base::app::CommandLine& cmdLine)
		{
			auto adapterIndex = cmdLine.singleValueInt("dxAdapter", 0);

			IDXGIAdapter1* adapter = nullptr;
			HRESULT hRet = m_dxFactory->EnumAdapters1(adapterIndex, &adapter);
			if (hRet != S_OK)
			{
				TRACE_ERROR("Unknown adapter {}", adapterIndex);
				return false;
			}

			DXGI_ADAPTER_DESC1 desc;
			memzero(&desc, sizeof(desc));
			DXGI_PROTECT(adapter->GetDesc1(&desc));

			TRACE_INFO("Found DXGI adapter {} at index {}", desc.Description, adapterIndex);
			TRACE_INFO("Adapter VendorID: {}", Hex(desc.VendorId));
			TRACE_INFO("Adapter DeviceID: {}", Hex(desc.DeviceId));
			TRACE_INFO("Adapter SubSysId: {}", Hex(desc.SubSysId));
			TRACE_INFO("Adapter Revision: {}", Hex(desc.Revision));

			TRACE_INFO("Dedicated video memory size: {}", MemSize(desc.DedicatedVideoMemory));
			TRACE_INFO("Dedicated system memory size: {}", MemSize(desc.DedicatedSystemMemory));
			TRACE_INFO("Shader system memory size: {}", MemSize(desc.SharedSystemMemory));

			m_dxAdapter = adapter;
			return true;
		}

		void DXGIHelper::enumerateAdapters()
		{
			uint32_t displayIndex = 0;
			for (;;)
			{
				IDXGIOutput* output = nullptr;
				HRESULT hRet = m_dxAdapter->EnumOutputs(displayIndex, &output);
				DEBUG_CHECK_EX(hRet == DXGI_ERROR_NOT_FOUND || hRet == S_OK, base::TempString("Unexpected result from EnumAdapters: {} {}", hRet, TranslateDXGIError(hRet)));
				if (hRet == DXGI_ERROR_NOT_FOUND)
					break;

				DXGI_OUTPUT_DESC desc;
				memzero(&desc, sizeof(desc));
				DXGI_PROTECT(output->GetDesc(&desc));

				TRACE_INFO("  Found DXGI output {}: {}", displayIndex, desc.DeviceName);
				TRACE_INFO("  Desktop coordinates: [{},{}] - [{},{}]", desc.DesktopCoordinates.left, desc.DesktopCoordinates.top, desc.DesktopCoordinates.right, desc.DesktopCoordinates.bottom);
				TRACE_INFO("  Attached: {}", desc.AttachedToDesktop);
				TRACE_INFO("  Rotation: {}", (int)desc.Rotation);

				MONITORINFO info;
				memzero(&info, sizeof(info));
				info.cbSize = sizeof(MONITORINFO);
				GetMonitorInfoA(desc.Monitor, &info);

				auto& displayInfo = m_cachedDisplays.emplaceBack();
				displayInfo.dxOutput = output;
				displayInfo.displayInfo.active = true;
				displayInfo.displayInfo.attached = desc.AttachedToDesktop;
				displayInfo.displayInfo.desktopWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
				displayInfo.displayInfo.desktopWidth = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
				displayInfo.displayInfo.primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;

				auto& rect = m_cachedMonitors.emplaceBack();
				rect.min.x = desc.DesktopCoordinates.left;
				rect.min.y = desc.DesktopCoordinates.top;
				rect.max.x = desc.DesktopCoordinates.right;
				rect.max.y = desc.DesktopCoordinates.bottom;

				enumerateDisplayModes(displayInfo);
			}
		}

		void DXGIHelper::enumerateDisplayModes(CachedDisplay& displayInfo)
		{
			const auto format = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO: proper format list

			UINT numModes = 0;
			DXGI_PROTECT(displayInfo.dxOutput->GetDisplayModeList(format, 0, &numModes, NULL));
			TRACE_INFO("  Found {} display modes for format {}", numModes, TranslateDXGIFormatName(format));

			base::Array<DXGI_MODE_DESC> modes;
			modes.resize(numModes);
			DXGI_PROTECT(displayInfo.dxOutput->GetDisplayModeList(format, 0, &numModes, modes.typedData()));

			for (uint32_t modeIndex : modes.indexRange())
			{
				const auto& modeDesc = modes[modeIndex];
				TRACE_INFO("  Found DXGI render mode {}: [{}x{}], refresh rate {}/{}", modeIndex, modeDesc.Width, modeDesc.Height, modeDesc.RefreshRate.Numerator, modeDesc.RefreshRate.Denominator);

				bool newResolution = true;

				for (auto& existingMode : displayInfo.resolutions)
				{
					if (existingMode.width == modeDesc.Width && existingMode.height == modeDesc.Height)
					{
						auto& rate = existingMode.refreshRates.emplaceBack();
						rate.num = modeDesc.RefreshRate.Numerator;
						rate.denom = modeDesc.RefreshRate.Denominator;
						newResolution = false;
						break;
					}
				}

				if (newResolution)
				{
					auto& mode = displayInfo.resolutions.emplaceBack();
					mode.width = modeDesc.Width;
					mode.height = modeDesc.Height;

					auto& rate = mode.refreshRates.emplaceBack();
					rate.num = modeDesc.RefreshRate.Numerator;
					rate.denom = modeDesc.RefreshRate.Denominator;
				}
			}
		}

		//---

	} // api
} // rendering