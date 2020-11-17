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
#include "rendering/driver/include/renderingConstantsView.h"
#include "rendering/driver/include/renderingBufferView.h"
#include "rendering/driver/include/renderingCommandBuffer.h"
#include "rendering/driver/include/renderingCommandWriter.h"
#include "rendering/driver/include/renderingDeviceService.h"
#include "rendering/driver/include/renderingDriver.h"
#include "rendering/driver/include/renderingShaderLibrary.h"

namespace ed
{
    //---

    base::ConfigProperty<int> cvNumImageHistogramBuckets("Editor.Image", "NumHistogramBuckets", 256);

    //---

    ImageHistogramPendingData::ImageHistogramPendingData(const rendering::DownloadBufferPtr& data, uint32_t totalPixelCount, uint32_t numBuckets, uint8_t channel)
        : m_totalPixelCount(totalPixelCount)
        , m_downloadBuffer(data)
        , m_numBuckets(numBuckets)
        , m_channel(channel)
    {
    }

    ImageHistogramPendingData::~ImageHistogramPendingData()
    {
    }

    const base::RefPtr<ImageHistogramData> ImageHistogramPendingData::fetchDataIfReady()
    {
        auto lock = CreateLock(m_lock);

        if (!m_data)
        {
            if (m_downloadBuffer && m_downloadBuffer->isReady())
            {
                if (auto data = m_downloadBuffer->data())
                {
                    const auto expectedSize = (2 + m_numBuckets) * sizeof(uint32_t);
                    auto dataSize = data.size();
                    DEBUG_CHECK_EX(dataSize == expectedSize, "Downloaded histogram data does not have proper size");
                    if (expectedSize == dataSize)
                    {
                        auto* dataPtr = (const uint32_t*)data.data();

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
            }
        }

        return m_data;
    }
    
    //--

    //base::res::StaticResource<rendering::ShaderLibrary> resComputeImagePreviewHistogram("/editor/shaders/canvas_rendering_panel_integration.csl");

    //--

    struct ComputeHistogramConstants
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
    };

    base::RefPtr<ImageHistogramPendingData> ComputeHistogram(const rendering::ImageView& view, const ImageComputationSettings& settings)
    {
        if (view.empty())
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
        return base::RefNew<ImageHistogramPendingData>(downloadBuffer, totalPixels, bucketCount, settings.channel);
    }

    //--

} // ed
