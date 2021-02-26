/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: camera #]
***/

#include "build.h"
#include "renderingFrameCameraContext.h"
#include "gpu/device/include/renderingResources.h"
#include "gpu/device/include/renderingDeviceApi.h"
#include "gpu/device/include/renderingDeviceService.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

CameraContext_Adaptation::CameraContext_Adaptation(gpu::IDevice* dev)
{
	gpu::ImageCreationInfo info;
	info.width = 1;
	info.height = 1;
	info.allowShaderReads = true;
	info.allowUAV = true;
	info.format = ImageFormat::RGBA32F;
	info.label = "CurrentFrameLum";

	currentFrameLuminance = dev->createImage(info);
	currentFrameLuminanceSRV = currentFrameLuminance->createSampledView();
	currentFrameLuminanceUAV = currentFrameLuminance->createWritableView();

	info.label = "PrevFrameLum";

	prevFrameLuminance = dev->createImage(info);
	prevFrameLuminanceSRV = prevFrameLuminance->createSampledView();
	prevFrameLuminanceUAV = prevFrameLuminance->createWritableView();
}

//--

CameraContext_TemporalAA::CameraContext_TemporalAA(gpu::IDevice* dev, uint32_t frameCount)
{
	for (auto i = 0; i < frameCount; ++i)
	{
		auto& entry = frames.emplaceBack();
		entry.valid = false;
	}
}

void CameraContext_TemporalAA::createFrames(gpu::IDevice* dev, uint32_t width, uint32_t height)
{
	supprotedWidth = Align<uint32_t>(width, 256);
	supprotedHeight = Align<uint32_t>(height, 256);

	gpu::ImageCreationInfo info;
	info.width = supprotedWidth;
	info.height = supprotedHeight;
	info.allowShaderReads = true;
	info.allowUAV = true;
	info.format = ImageFormat::RGBA16F;

	for (auto i : frames.indexRange())
	{
		auto& frame = frames[i];
							
		info.label = TempString("FrameHistory{}", i);

		frame.historyBuffer = dev->createImage(info);
		frame.historyBufferSRV = frame.historyBuffer->createSampledView();
		frame.historyBufferUAV = frame.historyBuffer->createWritableView();

		frame.valid = false;
		frame.camera = HistoryCameraSetup();
		frame.timestamp = NativeTimePoint();
	}
}

void CameraContext_TemporalAA::adjustForResolution(gpu::IDevice* dev, uint32_t width, uint32_t height)
{
	if (width > supprotedWidth || height > supprotedHeight)
		createFrames(dev, width, height);
}

void CameraContext_TemporalAA::pushHistory(const HistoryCameraSetup& camera)
{
	auto temp = std::move(frames[0]);

	for (uint32_t i = 1; i < frames.size(); ++i)
		frames[i - 1] = std::move(frames[i]);

	frames.back() = std::move(temp);

	frames[0].camera = camera;
	frames[0].valid = true;
	frames[0].timestamp.resetToNow();
}

//--

CameraContext::CameraContext()
	: adaptation(GetService<DeviceService>()->device())
	, taa(GetService<DeviceService>()->device(), 4)
{}

CameraContext::~CameraContext()
{}

//--

END_BOOMER_NAMESPACE_EX(rendering)
