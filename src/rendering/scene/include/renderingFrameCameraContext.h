/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: camera #]
***/

#pragma once

#include "base/reflection/include/variantTable.h"

namespace rendering
{
    namespace scene
    {
		//---

		// HDR adaptation data 
		struct CameraContext_Adaptation
		{
			// 1x1, RGBA32F
			ImageObjectPtr currentFrameLuminance;
			ImageSampledViewPtr currentFrameLuminanceSRV;
			ImageWritableViewPtr currentFrameLuminanceUAV;

			// 1x1, RGBA32F
			ImageObjectPtr prevFrameLuminance;
			ImageSampledViewPtr prevFrameLuminanceSRV;
			ImageWritableViewPtr prevFrameLuminanceUAV;

			//--

			CameraContext_Adaptation(IDevice* dev);
		};

		// Temporal AA data 
		struct CameraContext_TemporalAA
		{
			struct HistoryCameraSetup
			{
				base::Matrix prevWorldToScreen;
			};

			struct HistoryFrame
			{
				HistoryCameraSetup camera;

				bool valid = false;
				base::NativeTimePoint timestamp;

				ImageObjectPtr historyBuffer;
				ImageSampledViewPtr historyBufferSRV;
				ImageWritableViewPtr historyBufferUAV;
			};

			CameraContext_TemporalAA(IDevice* dev, uint32_t frameCount);

			void adjustForResolution(IDevice* dev, uint32_t width, uint32_t height);

			void pushHistory(const HistoryCameraSetup& camera);

			inline uint32_t size() const { return frames.size(); }
			inline HistoryFrame& operator[](int index) { return frames[index]; }

		private:
			uint32_t supprotedWidth = 0;
			uint32_t supprotedHeight = 0;

			base::Array<HistoryFrame> frames;

			void createFrames(IDevice* dev, uint32_t width, uint32_t height);
		};

        //---

		// data holder for frame 2 frame stuff, cached engine side
		class RENDERING_SCENE_API CameraContext : public base::IReferencable
		{
		public:
			CameraContext();
			~CameraContext();

			//--

			CameraContext_Adaptation adaptation;
			CameraContext_TemporalAA taa;
		};

		//---

	} // scene
} // rendering

