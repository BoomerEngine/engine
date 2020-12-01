/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: dxgi #]
***/

#pragma once
#include "../../device/include/renderingDeviceApi.h"

namespace rendering
{
	namespace api
	{

		//--

		/// helper class that manages outputs for DirectX
		class RENDERING_API_DXGI_API DXGIHelper : public base::NoCopy
		{
		public:
			DXGIHelper();
			~DXGIHelper();

			///--

			// get the DXGI factory (to create swapchains)
			INLINE IDXGIFactory1* dxFactory() const { return m_dxFactory; }

			// get the selected DXGI adapter (we don't currently support multi gpu rendering)
			INLINE IDXGIAdapter1* dxAdapter() const { return m_dxAdapter; }

			///--

			/// initialize, may fail for some crazy reason
			bool initialize(const base::app::CommandLine& cmdLine);

			/// device output/display discovery interface
			void enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const;
			void enumDisplays(base::Array<DisplayInfo>& outDisplayInfos) const;
			void enumResolutions(uint32_t displayIndex, base::Array<ResolutionInfo>& outResolutions) const;

		private:
			IDXGIFactory1* m_dxFactory = nullptr;
			IDXGIAdapter1* m_dxAdapter = nullptr;

			//--

			struct CachedDisplay
			{
				IDXGIOutput* dxOutput = nullptr;

				DisplayInfo displayInfo;
				base::Array<ResolutionInfo> resolutions;
			};
			
			base::Array<CachedDisplay> m_cachedDisplays;
			base::Array<base::Rect> m_cachedMonitors;		

			bool initializeAdapter(const base::app::CommandLine& cmdLine);
			void enumerateAdapters();
			void enumerateDisplayModes(CachedDisplay& display);

			//--
		};

		//--

	} // api
} // rendering

