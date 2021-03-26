/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: camera #]
***/

#pragma once

#include "core/reflection/include/variantTable.h"

BEGIN_BOOMER_NAMESPACE()

//---

// HDR adaptation data 
struct CameraContext_Adaptation
{
	// 1x1, RGBA32F
	gpu::ImageObjectPtr currentFrameLuminance;
	gpu::ImageSampledViewPtr currentFrameLuminanceSRV;
	gpu::ImageWritableViewPtr currentFrameLuminanceUAV;

	// 1x1, RGBA32F
	gpu::ImageObjectPtr prevFrameLuminance;
	gpu::ImageSampledViewPtr prevFrameLuminanceSRV;
	gpu::ImageWritableViewPtr prevFrameLuminanceUAV;

	//--

	CameraContext_Adaptation(gpu::IDevice* dev);
};

// Temporal AA data 
struct CameraContext_TemporalAA
{
	struct HistoryCameraSetup
	{
		Matrix prevWorldToScreen;
	};

	struct HistoryFrame
	{
		HistoryCameraSetup camera;

		bool valid = false;
		NativeTimePoint timestamp;

		gpu::ImageObjectPtr historyBuffer;
		gpu::ImageSampledViewPtr historyBufferSRV;
		gpu::ImageWritableViewPtr historyBufferUAV;
	};

	CameraContext_TemporalAA(gpu::IDevice* dev, uint32_t frameCount);

	void adjustForResolution(gpu::IDevice* dev, uint32_t width, uint32_t height);

	void pushHistory(const HistoryCameraSetup& camera);

	inline uint32_t size() const { return frames.size(); }
	inline HistoryFrame& operator[](int index) { return frames[index]; }

private:
	uint32_t supprotedWidth = 0;
	uint32_t supprotedHeight = 0;

	Array<HistoryFrame> frames;

	void createFrames(gpu::IDevice* dev, uint32_t width, uint32_t height);
};

//---

// data holder for frame 2 frame stuff, cached engine side
class ENGINE_RENDERING_API CameraContext : public IReferencable
{
public:
	CameraContext();
	~CameraContext();

	//--

	CameraContext_Adaptation adaptation;
	CameraContext_TemporalAA taa;
};

//---

END_BOOMER_NAMESPACE()
