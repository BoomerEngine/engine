/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: dxgi #]
***/

#pragma once
#include "../../device/include/renderingDeviceApi.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api)

//--

/// helper class that manages outputs for DirectX
class GPU_API_DXGI_API DXGIHelper : public NoCopy
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
	bool initialize(const app::CommandLine& cmdLine);

	/// device output/display discovery interface
	void enumMonitorAreas(Array<Rect>& outMonitorAreas) const;
	void enumDisplays(Array<DisplayInfo>& outDisplayInfos) const;
	void enumResolutions(uint32_t displayIndex, Array<ResolutionInfo>& outResolutions) const;

private:
	IDXGIFactory1* m_dxFactory = nullptr;
	IDXGIAdapter1* m_dxAdapter = nullptr;

	//--

	struct CachedDisplay
	{
		IDXGIOutput* dxOutput = nullptr;

		DisplayInfo displayInfo;
		Array<ResolutionInfo> resolutions;
	};
			
	Array<CachedDisplay> m_cachedDisplays;
	Array<Rect> m_cachedMonitors;

	bool initializeAdapter(const app::CommandLine& cmdLine);
	void enumerateAdapters();
	void enumerateDisplayModes(CachedDisplay& display);

	//--
};

END_BOOMER_NAMESPACE_EX(gpu::api)
