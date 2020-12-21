/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "imageHistogramWidget.h"
#include "imageHistogramCalculation.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingDeviceApi.h"

namespace ed
{
    //---

    base::ConfigProperty<int> cvNumImageHistogramBuckets("Editor.Image", "NumHistogramBuckets", 256);

    //---

    ImageHistogramPendingData::ImageHistogramPendingData(uint32_t totalPixelCount, uint32_t numBuckets, uint8_t channel)
        : m_totalPixelCount(totalPixelCount)
        , m_numBuckets(numBuckets)
        , m_channel(channel)
    {
    }

    ImageHistogramPendingData::~ImageHistogramPendingData()
    {
    }

	void ImageHistogramPendingData::processRetreivedData(rendering::IDownloadAreaObject* area, const void* untypedDataPtr, uint32_t dataSize, const rendering::ResourceCopyRange& info)
	{
		const auto expectedSize = (2 + m_numBuckets) * sizeof(uint32_t);
		if (expectedSize == dataSize)
		{
			const auto* dataPtr = (const uint32_t*)untypedDataPtr;

			m_data = base::RefNew<ImageHistogramData>();
			m_data->channel = m_channel;
			m_data->totalPixelCount = m_totalPixelCount;
			m_data->minValue = reinterpret_cast<const float&>(dataPtr[0]);
			m_data->maxValue = reinterpret_cast<const float&>(dataPtr[1]);
			m_data->maxBucketValue = 0;
			memcpy(m_data->buckets.allocate(m_numBuckets), dataPtr + 2, sizeof(uint32_t) * m_numBuckets);

			for (auto val : m_data->buckets)
				m_data->maxBucketValue = std::max<uint32_t>(val, m_data->maxBucketValue);
		}
	}

    const base::RefPtr<ImageHistogramData> ImageHistogramPendingData::fetchDataIfReady()
    {
        auto lock = CreateLock(m_lock);
        return m_data;
    }
    
    //--

    //base::res::StaticResource<rendering::ShaderLibrary> resComputeImagePreviewHistogram("/editor/shaders/canvas_rendering_panel_integration.csl");

    //--

 /*  struct ComputeHistogramConstants
    {
        uint32_t imageWidth = 0;
        uint32_t imageHeight = 0;
        uint32_t componentType = 0;
        uint32_t numberOfBuckets = 0;
    };

    struct ComputeHistogramParams
    {
        rendering::ConstantsView Params;
        rendering::ImageView InputImage;
        rendering::BufferView MinMax;
        rendering::BufferView Buckets;
    };*/

    base::RefPtr<ImageHistogramPendingData> ComputeHistogram(const rendering::ImageSampledView* view, const ImageComputationSettings& settings)
    {
        /*if (view.empty())
            return nullptr;

        if (settings.mipIndex < 0 || settings.mipIndex >= view.numMips())
            return nullptr;

        if (settings.sliceIndex < 0 || settings.sliceIndex >= view.numArraySlices())
            return nullptr;

        if (view.viewType() != rendering::ImageViewType::View2D)
            return nullptr;

        auto commandBuffer = rendering::command::CommandBuffer::Alloc();
        if (!commandBuffer)
            return nullptr;

        const auto bucketCount = 256;

        auto downloadDataSize = (2 + bucketCount) * sizeof(uint32_t);
        auto downloadBuffer = base::RefNew<rendering::DownloadBuffer>();

        {
            rendering::command::CommandWriter cmd("ComputeHistogram");

            // create the buffer for computations
            rendering::TransientBufferView histogramBuffer(rendering::BufferViewFlag::ShaderReadable, rendering::TransientBufferAccess::ShaderReadWrite, downloadDataSize, 0, true);
            cmd.opAllocTransientBuffer(histogramBuffer);

            // download buffer
            cmd.opDownloadBuffer(histogramBuffer, downloadBuffer);

            // issue to rendering
            // TODO: idea of background work would be nice
            base::GetService<DeviceService>()->device()->submitWork(cmd.release());
        }

        // create wrapper
        const auto totalPixels = view.width() * view.height();
        return base::RefNew<ImageHistogramPendingData>(downloadBuffer, totalPixels, bucketCount, settings.channel);*/
        return nullptr;
    }

    //--

} // ed
